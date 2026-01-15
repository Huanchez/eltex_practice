#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PAYLOAD 1024
#define CLOSE_MSG "__CLOSE__"

static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static unsigned short ip_checksum(const void *vdata, size_t length) {
    const unsigned char *data = (const unsigned char *)vdata;
    unsigned long acc = 0xffff;

    for (size_t i = 0; i + 1 < length; i += 2) {
        unsigned short word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) acc = (acc & 0xffff) + (acc >> 16);
    }

    if (length & 1) {
        unsigned short word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) acc = (acc & 0xffff) + (acc >> 16);
    }

    acc = ~acc;
    return htons((unsigned short)acc);
}

typedef struct {
    uint32_t ip;
    uint16_t port;
    unsigned counter;
} ClientEntry;

typedef struct {
    ClientEntry *a;
    size_t n;
    size_t cap;
} ClientTable;

static void tbl_init(ClientTable *t) {
    t->a = NULL;
    t->n = 0;
    t->cap = 0;
}

static int tbl_find(const ClientTable *t, uint32_t ip, uint16_t port) {
    for (size_t i = 0; i < t->n; i++) {
        if (t->a[i].ip == ip && t->a[i].port == port) return (int)i;
    }
    return -1;
}

static int tbl_add(ClientTable *t, uint32_t ip, uint16_t port) {
    if (t->n == t->cap) {
        size_t newcap = (t->cap == 0) ? 8 : t->cap * 2;
        ClientEntry *na = (ClientEntry *)realloc(t->a, newcap * sizeof(*na));
        if (!na) return -1;
        t->a = na;
        t->cap = newcap;
    }
    t->a[t->n].ip = ip;
    t->a[t->n].port = port;
    t->a[t->n].counter = 0;
    t->n++;
    return (int)(t->n - 1);
}

static void tbl_remove(ClientTable *t, size_t idx) {
    if (idx >= t->n) return;
    t->a[idx] = t->a[t->n - 1];
    t->n--;
}


static void send_udp_raw(int rsock, uint32_t src_ip, uint16_t src_port,
                         uint32_t dst_ip, uint16_t dst_port,
                         const unsigned char *payload, size_t payload_len) {
    unsigned char pkt[sizeof(struct iphdr) + sizeof(struct udphdr) + MAX_PAYLOAD];
    size_t ip_len = sizeof(struct iphdr);
    size_t udp_len = sizeof(struct udphdr);
    size_t total = ip_len + udp_len + payload_len;
    if (total > sizeof(pkt)) return;

    struct iphdr *ip = (struct iphdr *)pkt;
    struct udphdr *udp = (struct udphdr *)(pkt + ip_len);
    unsigned char *pl = pkt + ip_len + udp_len;

    memcpy(pl, payload, payload_len);

    memset(ip, 0, ip_len);
    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = src_ip;
    ip->daddr = dst_ip;
    ip->tot_len = htons((unsigned short)total);
    ip->check = 0;
    ip->check = ip_checksum(ip, ip_len);

    memset(udp, 0, udp_len);
    udp->source = src_port;
    udp->dest = dst_port;
    udp->len = htons((unsigned short)(udp_len + payload_len));
    udp->check = 0;

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = dst_ip;

    sendto(rsock, pkt, total, 0, (struct sockaddr *)&to, sizeof(to));
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Использование: %s <server_port>\n", argv[0]);
        return 1;
    }

    int server_port = atoi(argv[1]);
    if (server_port <= 0 || server_port > 65535) return 1;

    signal(SIGINT, on_sigint);

    int rsock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (rsock < 0) {
        perror("socket");
        return 1;
    }

    int on = 1;
    setsockopt(rsock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

    ClientTable tbl;
    tbl_init(&tbl);

    printf("Сервер запущен. UDP port=%d\n", server_port);

    unsigned char buf[65536];
    long total_msgs = 0;

    while (running) {
        ssize_t n = recvfrom(rsock, buf, sizeof(buf), 0, NULL, NULL);
        if (n <= 0) continue;
        if ((size_t)n < sizeof(struct iphdr)) continue;

        struct iphdr *ip = (struct iphdr *)buf;
        if (ip->version != 4) continue;
        if (ip->protocol != IPPROTO_UDP) continue;

        size_t ip_hl = (size_t)ip->ihl * 4;
        if ((size_t)n < ip_hl + sizeof(struct udphdr)) continue;

        struct udphdr *udp = (struct udphdr *)(buf + ip_hl);

        int dport = ntohs(udp->dest);
        if (dport != server_port) continue;

        uint32_t cli_ip = ip->saddr;
        uint16_t cli_port = udp->source;

        size_t udp_total = ntohs(udp->len);
        if (udp_total < sizeof(struct udphdr)) continue;

        size_t payload_len = udp_total - sizeof(struct udphdr);
        const unsigned char *payload = buf + ip_hl + sizeof(struct udphdr);
        if ((size_t)n < ip_hl + sizeof(struct udphdr) + payload_len) continue;

        char text[MAX_PAYLOAD + 1];
        size_t copy = payload_len;
        if (copy > MAX_PAYLOAD) copy = MAX_PAYLOAD;
        memcpy(text, payload, copy);
        text[copy] = '\0';

        int idx = tbl_find(&tbl, cli_ip, cli_port);

        if (strcmp(text, CLOSE_MSG) == 0) {
            if (idx >= 0) tbl_remove(&tbl, (size_t)idx);

            struct in_addr a;
            a.s_addr = cli_ip;
            printf("CLOSE от %s:%d -> счетчик сброшен\n",
                   inet_ntoa(a), ntohs(cli_port));
            continue;
        }

        if (idx < 0) idx = tbl_add(&tbl, cli_ip, cli_port);
        if (idx < 0) continue;

        tbl.a[idx].counter++;
        total_msgs++;

        char out[MAX_PAYLOAD + 64];
        snprintf(out, sizeof(out), "%s %u", text, tbl.a[idx].counter);

        uint32_t srv_ip = ip->daddr;
        uint16_t srv_port = udp->dest;

        send_udp_raw(rsock, srv_ip, srv_port, cli_ip, cli_port,
                     (const unsigned char *)out, strlen(out) + 1);

        struct in_addr a;
        a.s_addr = cli_ip;
        printf("[%ld] %s:%d -> \"%s\" => \"%s\"\n",
               total_msgs, inet_ntoa(a), ntohs(cli_port), text, out);
    }

    printf("\nОстановлено. Клиентов в таблице: %zu\n", tbl.n);

    free(tbl.a);
    close(rsock);
    return 0;
}

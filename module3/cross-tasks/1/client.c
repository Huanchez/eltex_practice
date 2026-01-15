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

static int rsock = -1;

static uint32_t server_ip = 0;
static uint16_t server_port = 0;
static uint32_t my_ip = 0;
static uint16_t my_port = 0;

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

static void send_udp_raw(const unsigned char *payload, size_t payload_len) {
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
    ip->saddr = my_ip;
    ip->daddr = server_ip;
    ip->tot_len = htons((unsigned short)total);
    ip->check = 0;
    ip->check = ip_checksum(ip, ip_len);

    memset(udp, 0, udp_len);
    udp->source = my_port;
    udp->dest = server_port;
    udp->len = htons((unsigned short)(udp_len + payload_len));
    udp->check = 0;

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = server_ip;

    sendto(rsock, pkt, total, 0, (struct sockaddr *)&to, sizeof(to));
}

static int recv_reply(char *out, size_t cap) {
    unsigned char buf[65536];

    while (running) {
        ssize_t n = recvfrom(rsock, buf, sizeof(buf), 0, NULL, NULL);
        if (n <= 0) return 0;
        if ((size_t)n < sizeof(struct iphdr)) continue;

        struct iphdr *ip = (struct iphdr *)buf;
        if (ip->version != 4) continue;
        if (ip->protocol != IPPROTO_UDP) continue;

        size_t ip_hl = (size_t)ip->ihl * 4;
        if ((size_t)n < ip_hl + sizeof(struct udphdr)) continue;

        struct udphdr *udp = (struct udphdr *)(buf + ip_hl);

        if (udp->dest != my_port) continue;
        if (udp->source != server_port) continue;

        size_t udp_total = ntohs(udp->len);
        if (udp_total < sizeof(struct udphdr)) continue;

        size_t payload_len = udp_total - sizeof(struct udphdr);
        const unsigned char *payload = buf + ip_hl + sizeof(struct udphdr);
        if ((size_t)n < ip_hl + sizeof(struct udphdr) + payload_len) continue;

        size_t copy = payload_len;
        if (copy + 1 > cap) copy = cap - 1;
        memcpy(out, payload, copy);
        out[copy] = '\0';
        return 1;
    }

    return 0;
}

static void send_close(void) {
    const char *s = CLOSE_MSG;
    send_udp_raw((const unsigned char *)s, strlen(s) + 1);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Использование: %s <server_ip> <server_port> <my_port>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, on_sigint);

    struct in_addr a;
    if (inet_pton(AF_INET, argv[1], &a) != 1) return 1;
    server_ip = a.s_addr;

    int sp = atoi(argv[2]);
    int mp = atoi(argv[3]);
    if (sp <= 0 || sp > 65535) return 1;
    if (mp <= 0 || mp > 65535) return 1;

    server_port = htons((uint16_t)sp);
    my_port = htons((uint16_t)mp);

    my_ip = server_ip;

    rsock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (rsock < 0) {
        perror("socket");
        return 1;
    }

    int on = 1;
    setsockopt(rsock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

    printf("Клиент: сервер=%s:%d, me=%d\n", argv[1], sp, mp);

    char line[MAX_PAYLOAD];

    while (running) {
        printf("you> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            running = 0;
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        if (strcmp(line, "exit") == 0) {
            running = 0;
            break;
        }

        send_udp_raw((const unsigned char *)line, strlen(line) + 1);

        char reply[MAX_PAYLOAD + 64];
        if (recv_reply(reply, sizeof(reply))) {
            printf("srv> %s\n", reply);
        }
    }

    send_close();

    close(rsock);
    printf("\nКлиент остановлен\n");
    return 0;
}

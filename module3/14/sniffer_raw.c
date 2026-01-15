#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if_packet.h>

#define PORT_A 5000
#define PORT_B 5001

#define BUF_SZ 65536

static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static int is_printable_line(const unsigned char *p, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c = p[i];
        if (c == 0) break;
        if (c == '\n' || c == '\r' || c == '\t') continue;
        if (c < 32 || c > 126) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *iface = "lo";
    const char *dumpname = "udp_dump.bin";

    if (argc >= 2) iface = argv[1];
    if (argc >= 3) dumpname = argv[2];

    signal(SIGINT, on_sigint);

    int rsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (rsock < 0) {
        perror("socket");
        return 1;
    }

    int ifindex = if_nametoindex(iface);
    if (ifindex == 0) {
        perror("if_nametoindex");
        return 1;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifindex;

    if (bind(rsock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        return 1;
    }

    FILE *dump = fopen(dumpname, "ab");
    if (!dump) {
        perror("fopen dump");
        return 1;
    }

    printf("Снифер запущен. iface=%s, dump=%s\n", iface, dumpname);
    printf("Ловлю UDP dst-port %d или %d\n\n", PORT_A, PORT_B);

    unsigned char buf[BUF_SZ];
    long captured = 0;

    while (running) {
        struct sockaddr_ll ll;
        socklen_t lllen = sizeof(ll);

        ssize_t n = recvfrom(rsock, buf, sizeof(buf), 0, (struct sockaddr *)&ll, &lllen);
        if (n <= 0) continue;

        if (ll.sll_pkttype == PACKET_OUTGOING) continue;
        size_t off = 0;

        if ((size_t)n >= sizeof(struct ethhdr)) {
            struct ethhdr *eth = (struct ethhdr *)buf;
            if (ntohs(eth->h_proto) == ETH_P_IP) {
                off = sizeof(struct ethhdr);
            }
        }

        if ((size_t)n < off + sizeof(struct iphdr)) continue;

        struct iphdr *ip = (struct iphdr *)(buf + off);
        if (ip->version != 4) continue;
        if (ip->protocol != IPPROTO_UDP) continue;

        size_t ip_hl = (size_t)ip->ihl * 4;
        if ((size_t)n < off + ip_hl + sizeof(struct udphdr)) continue;

        struct udphdr *udp = (struct udphdr *)(buf + off + ip_hl);

        int dport = ntohs(udp->dest);
        int sport = ntohs(udp->source);

        if (!(dport == PORT_A || dport == PORT_B)) continue;

        size_t udp_hl = sizeof(struct udphdr);
        size_t payload_off = off + ip_hl + udp_hl;
        if ((size_t)n < payload_off) continue;

        size_t payload_len = (size_t)n - payload_off;
        unsigned char *payload = buf + payload_off;

        fwrite(payload, 1, payload_len, dump);
        fflush(dump);

        captured++;

        struct in_addr sa, da;
        sa.s_addr = ip->saddr;
        da.s_addr = ip->daddr;

        char s_ip[INET_ADDRSTRLEN];
        char d_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa, s_ip, sizeof(s_ip));
        inet_ntop(AF_INET, &da, d_ip, sizeof(d_ip));

        printf("[%ld] %s:%d -> %s:%d | %zu bytes", captured, s_ip, sport, d_ip, dport, payload_len);

        if (payload_len > 0 && is_printable_line(payload, payload_len)) {
            printf(" | text: ");
            printf("%s", (char *)payload);
        }

        printf("\n");
    }

    printf("\nОстановлено. Поймано пакетов: %ld\n", captured);

    fclose(dump);
    close(rsock);
    return 0;
}

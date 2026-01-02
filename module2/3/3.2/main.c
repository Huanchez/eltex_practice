#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void print_usage(const char *prog)
{
    fprintf(stderr, "Использование: %s <IP_шлюза> <маска_подсети> <N>\n", prog);
    fprintf(stderr, "Пример: %s 192.168.1.1 255.255.255.0 1000\n", prog);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t gateway_ip = ip_parse_ipv4(argv[1]);
    if (gateway_ip == IP_PARSE_ERROR) {
        fprintf(stderr, "Ошибка: некорректный IP-адрес шлюза '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    uint32_t subnet_mask = ip_parse_ipv4(argv[2]);
    if (subnet_mask == IP_PARSE_ERROR || !ip_is_valid_mask(subnet_mask)) {
        fprintf(stderr, "Ошибка: некорректная маска подсети '%s'\n", argv[2]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    unsigned long long N = strtoull(argv[3], &endptr, 10);
    if (endptr == argv[3] || *endptr != '\0' || N == 0ULL) {
        fprintf(stderr, "Ошибка: N должно быть положительным целым числом\n");
        return EXIT_FAILURE;
    }

    uint32_t local_network = gateway_ip & subnet_mask;

    srand((unsigned)time(NULL));

    unsigned long long local_count = 0ULL;

    for (unsigned long long i = 0; i < N; i++) {
        uint32_t dest_ip = ip_random_ipv4();
        uint32_t dest_network = dest_ip & subnet_mask;

        if (dest_network == local_network) {
            local_count++;
        }
    }

    unsigned long long foreign_count = N - local_count;
    double local_percent = 100.0 * (double)local_count / (double)N;
    double foreign_percent = 100.0 * (double)foreign_count / (double)N;

    printf("Обработано пакетов: %llu\n", N);
    printf("В своей подсети: %llu (%.2f%%)\n", local_count, local_percent);
    printf("В чужих сетях: %llu (%.2f%%)\n", foreign_count, foreign_percent);

    return EXIT_SUCCESS;
}

#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t ip_parse_ipv4(const char *ip_str)
{
    if (!ip_str) return IP_PARSE_ERROR;

    char buffer[64];
    strncpy(buffer, ip_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    uint32_t result = 0;

    char *token = strtok(buffer, ".");
    for (int i = 0; i < 4; i++) {
        if (token == NULL) return IP_PARSE_ERROR;

        char *endptr = NULL;
        long octet = strtol(token, &endptr, 10);
        if (endptr == token || *endptr != '\0') return IP_PARSE_ERROR;
        if (octet < 0 || octet > 255) return IP_PARSE_ERROR;

        result = (result << 8) | (uint32_t)octet;
        token = strtok(NULL, ".");
    }

    if (token != NULL) return IP_PARSE_ERROR;

    return result;
}


int ip_is_valid_mask(uint32_t mask)
{
    if (mask == 0U) return 0;

    uint32_t x = ~mask;
    return (x & (x + 1U)) == 0U;
}

uint32_t ip_random_ipv4(void)
{
    uint32_t ip = 0;
    for (int i = 0; i < 4; i++) {
        ip = (ip << 8) | (uint32_t)(rand() % 256);
    }
    return ip;
}

void ip_to_string(uint32_t ip, char *out, size_t out_size)
{
    if (!out || out_size == 0) return;

    unsigned a = (ip >> 24) & 0xFF;
    unsigned b = (ip >> 16) & 0xFF;
    unsigned c = (ip >> 8)  & 0xFF;
    unsigned d = (ip)       & 0xFF;

    snprintf(out, out_size, "%u.%u.%u.%u", a, b, c, d);
}

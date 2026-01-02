#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <stddef.h>

#define IP_PARSE_ERROR 0U

uint32_t ip_parse_ipv4(const char *ip_str);

int ip_is_valid_mask(uint32_t mask);

uint32_t ip_random_ipv4(void);

void ip_to_string(uint32_t ip, char *out, size_t out_size);

#endif

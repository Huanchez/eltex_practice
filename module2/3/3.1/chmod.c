#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "chmod.h"

void letterToBit(char* c, unsigned int* permission) {
    for (int i = 0; i < 9; i++) {
        permission[i] = (c[i] == '-') ? 0 : 1;
    }
}

void numericToBit(char* c, unsigned int* permission) {
    static const unsigned int DIG[8] = {0,0x049,0x092,0x0DB,0x124,0x16D,0x1B6,0x1FF};
    unsigned int mode = 0u, mask = 0x1C0;
    for (int i = 0; i < 3; i++) {
        int d = c[i] - '0';
        if (d < 0 || d > 7) d = 0;
        mode |= mask & DIG[d];
        mask >>= 3;
    }
    for (int i = 0; i < 9; i++) permission[8 - i] = (mode >> i) & 1u;
}

void letterPermissions(unsigned int mode, char *permission) {
    permission[0] = (mode & S_IRUSR) ? 'r' : '-';
    permission[1] = (mode & S_IWUSR) ? 'w' : '-';
    permission[2] = (mode & S_IXUSR) ? 'x' : '-';
    permission[3] = (mode & S_IRGRP) ? 'r' : '-';
    permission[4] = (mode & S_IWGRP) ? 'w' : '-';
    permission[5] = (mode & S_IXGRP) ? 'x' : '-';
    permission[6] = (mode & S_IROTH) ? 'r' : '-';
    permission[7] = (mode & S_IWOTH) ? 'w' : '-';
    permission[8] = (mode & S_IXOTH) ? 'x' : '-';
}

void numericPermissions(unsigned int mode, unsigned int *permission) {
    unsigned int mask = 0x007;
    for (int i = 2; i >= 0; i--) {
        permission[i] = mode & mask;
        mode >>= 3;
    }
}

void bitPermissions(unsigned int mode, unsigned int *permission) {
    unsigned int mask = 0x100;
    for (int i = 0; i < 9; i++) {
        permission[i] = (mode & mask) ? 1 : 0;
        mask >>= 1;
    }
}

unsigned int badchmod(char *c, const char *filename) {
    struct stat file_stat;
    unsigned int mode = 0x000;

    if (stat(filename, &file_stat) == 0)
        mode = file_stat.st_mode & 0x1FF;
    else
        return ERROR;

    if (matchPattern(c, "^[ugoar]+[+=-](r|w|x){1,3}$"))
        return letterchmod(c, mode);
    else if (matchPattern(c, "^[0-7]{3}$"))
        return numericchmod(c, mode);
    else
        return ERROR;
}

unsigned int letterchmod(char *c, unsigned int mode) {
    unsigned int classes = 0u, actions = 0u;
    char op = 0;

    for (size_t i = 0; i < strlen(c); i++) {
        switch (c[i]) {
            case '+': case '-': case '=': op = c[i]; break;
            case 'u': classes |= USER; break;
            case 'g': classes |= GROUP; break;
            case 'o': classes |= OTHER; break;
            case 'a': classes |= ALL; break;
            case 'r': actions |= READ; break;
            case 'w': actions |= WRITE; break;
            case 'x': actions |= EXEC; break;
            default: return ERROR;
        }
    }
    if (classes == 0) classes = ALL;
    unsigned int mask = classes & actions;

    switch (op) {
        case '+': mode |= mask; break;
        case '-': mode &= ~mask; break;
        case '=': mode = (mode & ~classes) | mask; break;
        default: return ERROR;
    }
    return mode & 0x1FF;
}

unsigned int numericchmod(char* c, unsigned int mode_ignored) {
    (void)mode_ignored;  // Не используется
    static const unsigned int DIG[8] = {
        0u,
        0x049,
        0x092,
        0x092 | 0x049,
        0x124,
        0x124 | 0x049,
        0x124 | 0x092,
        0x124 | 0x092 | 0x049
    };
    unsigned int mode = 0u;
    unsigned int mask = 0x1C0;

    for (int i = 0; i < 3; i++) {
        int d = c[i] - '0';
        if (d < 0 || d > 7) return ERROR;
        mode |= mask & DIG[d];
        mask >>= 3;
    }
    return mode & 0x1FF;
}

unsigned int matchPattern(char *input, const char *pattern) {
    regex_t regex;
    int reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti) return 0;

    reti = regexec(&regex, input, 0, NULL, 0);
    regfree(&regex);

    return (reti == 0) ? 1 : 0;
}
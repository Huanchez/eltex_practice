#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s str1 [str2 ...]\n", argv[0]);
        return 1;
    }

    int best = 1;
    for (int i = 2; i < argc; i++) {
        if (strlen(argv[i]) > strlen(argv[best])) best = i;
    }

    printf("Самая длинная строка: \"%s\" (длина %zu)\n", argv[best], strlen(argv[best]));
    return 0;
}

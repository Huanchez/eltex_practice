#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

static int parse_number(const char *arg, double *value, int *is_int)
{
    // Пропуск ведущих пробелов
    while (isspace((unsigned char)*arg)) arg++;
    if (*arg == '\0') return 0;

    errno = 0;
    char *end = NULL;
    double v = strtod(arg, &end);

    if (arg == end) return 0;
    if (errno == ERANGE) return 0;

    // Пропуск хвостовых пробелов
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;

    *value = v;

    double ipart;
    double frac = modf(v, &ipart);
    *is_int = (fabs(frac) < 1e-12);

    return 1;
}

static void process_argument(const char *arg, const char *who)
{
    double x;
    int is_int;

    if (parse_number(arg, &x, &is_int)) {
        if (is_int) {
            long long xi = (long long) llround(x);
            printf("[%s, PID=%d] Аргумент \"%s\" — целое число: %lld, удвоенное значение: %lld\n",
                   who, getpid(), arg, xi, xi * 2);
        } else {
            printf("[%s, PID=%d] Аргумент \"%s\" — вещественное число: %.6g, удвоенное значение: %.6g\n",
                   who, getpid(), arg, x, x * 2.0);
        }
    } else {
        printf("[%s, PID=%d] Аргумент \"%s\" не является числом\n",
               who, getpid(), arg);
    }
}

static void process_range(char *argv[], int start, int end, const char *who)
{
    for (int i = start; i <= end; i++) {
        process_argument(argv[i], who);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s аргумент1 [аргумент2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Отключаем буферизацию stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    int total = argc - 1;
    int half  = total / 2;

    pid_t pid = fork();
    if (pid < 0) {
        perror("Ошибка fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        if (half >= 1)
            process_range(argv, 1, half, "дочерний процесс");
        return EXIT_SUCCESS;
    } else {
        if (half + 1 <= total)
            process_range(argv, half + 1, total, "родительский процесс");

        waitpid(pid, NULL, 0);
        return EXIT_SUCCESS;
    }
}

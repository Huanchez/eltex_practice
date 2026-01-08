#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

static int fd = -1;
static volatile sig_atomic_t sigint_total = 0;
static volatile sig_atomic_t got_sigquit = 0;

static int write_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, buf + off, len - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

static void listener(int sig) {
    if (sig == SIGINT) sigint_total++;
    else if (sig == SIGQUIT) got_sigquit = 1;
}

int main(void) {
    fd = open("counter.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        perror("Ошибка открытия файла");
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = listener;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)");
        close(fd);
        return 1;
    }
    if (sigaction(SIGQUIT, &sa, NULL) < 0) {
        perror("sigaction(SIGQUIT)");
        close(fd);
        return 1;
    }

    unsigned long long counter = 1;
    int processed_sigint = 0;

    for (;;) {
        if (got_sigquit) {
            got_sigquit = 0;
            const char *msg = "Получен и обработан сигнал SIGQUIT\n";
            if (write_all(fd, msg, strlen(msg)) < 0) {
                perror("Ошибка записи в файл (SIGQUIT)");
                close(fd);
                return 1;
            }
        }

        while (processed_sigint < sigint_total) {
            processed_sigint++;
            char msg[128];
            int n = snprintf(msg, sizeof(msg),
                             "Получен и обработан сигнал SIGINT (%d/3)\n",
                             processed_sigint);
            if (n > 0 && write_all(fd, msg, (size_t)n) < 0) {
                perror("Ошибка записи в файл (SIGINT)");
                close(fd);
                return 1;
            }
        }

        if (processed_sigint >= 3) {
            const char *end = "Завершение: получен третий SIGINT\n";
            write_all(fd, end, strlen(end));
            break;
        }

        char line[64];
        int n = snprintf(line, sizeof(line), "%llu\n", counter);
        if (n > 0 && write_all(fd, line, (size_t)n) < 0) {
            perror("Ошибка записи счётчика");
            close(fd);
            return 1;
        }
        counter++;

        sleep(1);
    }

    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    return 0;
}

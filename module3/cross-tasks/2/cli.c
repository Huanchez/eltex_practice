#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_DRIVERS 128
#define MAX_LINE 512

typedef struct {
    pid_t pid;
    int fd;
    int alive;
} Driver;

static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static int set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static ssize_t write_all(int fd, const void *buf, size_t n) {
    const char *p = (const char *)buf;
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(fd, p + off, n - off);
        if (w > 0) {
            off += (size_t)w;
            continue;
        }
        if (w < 0 && (errno == EINTR)) continue;
        if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return (ssize_t)off;
        return -1;
    }
    return (ssize_t)off;
}

static int read_line_nb(int fd, char *out, size_t cap) {
    static char buf[MAX_DRIVERS][MAX_LINE];
    static size_t len[MAX_DRIVERS];

    int slot = fd % MAX_DRIVERS;

    for (;;) {
        for (size_t i = 0; i < len[slot]; i++) {
            if (buf[slot][i] == '\n') {
                size_t take = i + 1;
                if (take >= cap) take = cap - 1;
                memcpy(out, buf[slot], take);
                out[take] = '\0';

                size_t rest = len[slot] - (i + 1);
                memmove(buf[slot], buf[slot] + i + 1, rest);
                len[slot] = rest;
                return 1;
            }
        }

        ssize_t r = read(fd, buf[slot] + len[slot], sizeof(buf[slot]) - len[slot]);
        if (r > 0) {
            len[slot] += (size_t)r;
            if (len[slot] == sizeof(buf[slot])) {
                len[slot] = 0;
            }
            continue;
        }
        if (r == 0) return -1;
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
}

static int find_driver(Driver *ds, int n, pid_t pid) {
    for (int i = 0; i < n; i++) {
        if (ds[i].alive && ds[i].pid == pid) return i;
    }
    return -1;
}

static void driver_loop(int fd) {
    int ep = epoll_create1(0);
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u32 = 1;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u32 = 2;
    epoll_ctl(ep, EPOLL_CTL_ADD, tfd, &ev);

    int busy = 0;
    time_t busy_until = 0;

    {
        const char *hello = "READY Available\n";
        write_all(fd, hello, strlen(hello));
    }

    while (1) {
        struct epoll_event events[8];
        int k = epoll_wait(ep, events, 8, -1);
        if (k < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < k; i++) {
            if (events[i].data.u32 == 2) {
                unsigned long long expir = 0;
                read(tfd, &expir, sizeof(expir));
                busy = 0;
                busy_until = 0;
                const char *msg = "STATUS Available\n";
                write_all(fd, msg, strlen(msg));
                continue;
            }

            if (events[i].data.u32 == 1) {
                char line[MAX_LINE];
                int got = 0;

                while ((got = read_line_nb(fd, line, sizeof(line))) == 1) {
                    line[strcspn(line, "\r\n")] = '\0';

                    if (strcmp(line, "QUIT") == 0) {
                        const char *bye = "BYE\n";
                        write_all(fd, bye, strlen(bye));
                        close(tfd);
                        close(ep);
                        close(fd);
                        _exit(0);
                    }

                    if (strcmp(line, "STATUS") == 0) {
                        if (!busy) {
                            const char *s = "STATUS Available\n";
                            write_all(fd, s, strlen(s));
                        } else {
                            time_t now = time(NULL);
                            long rem = (long)(busy_until - now);
                            if (rem < 0) rem = 0;
                            char s[128];
                            snprintf(s, sizeof(s), "STATUS Busy %ld\n", rem);
                            write_all(fd, s, strlen(s));
                        }
                        continue;
                    }

                    if (strncmp(line, "TASK ", 5) == 0) {
                        int sec = atoi(line + 5);
                        if (sec < 0) sec = 0;

                        if (!busy) {
                            busy = 1;
                            time_t now = time(NULL);
                            busy_until = now + sec;

                            struct itimerspec it;
                            memset(&it, 0, sizeof(it));
                            it.it_value.tv_sec = sec;
                            timerfd_settime(tfd, 0, &it, NULL);

                            char s[128];
                            snprintf(s, sizeof(s), "OK Busy %d\n", sec);
                            write_all(fd, s, strlen(s));
                        } else {
                            time_t now = time(NULL);
                            long rem = (long)(busy_until - now);
                            if (rem < 0) rem = 0;
                            char s[128];
                            snprintf(s, sizeof(s), "BUSY %ld\n", rem);
                            write_all(fd, s, strlen(s));
                        }
                        continue;
                    }

                    {
                        const char *e = "ERR UnknownCommand\n";
                        write_all(fd, e, strlen(e));
                    }
                }

                if (got == -1) {
                    close(tfd);
                    close(ep);
                    close(fd);
                    _exit(0);
                }
            }
        }
    }

    close(tfd);
    close(ep);
    close(fd);
    _exit(0);
}

static void print_help(void) {
    printf("Команды:\n");
    printf("  create_driver\n");
    printf("  send_task <pid> <seconds>\n");
    printf("  get_status <pid>\n");
    printf("  get_drivers\n");
    printf("  exit\n\n");
}

int main(void) {
    signal(SIGINT, on_sigint);

    Driver drivers[MAX_DRIVERS];
    memset(drivers, 0, sizeof(drivers));
    int dcnt = 0;

    int ep = epoll_create1(0);
    if (ep < 0) {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u32 = 1000;
    epoll_ctl(ep, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    printf("Опорный пункт запущен. Ctrl+C или exit для завершения.\n");
    print_help();
    printf("taxi> ");
    fflush(stdout);

    while (running) {
        // Подчищаем зомби
        for (;;) {
            int st = 0;
            pid_t p = waitpid(-1, &st, WNOHANG);
            if (p <= 0) break;
            int idx = find_driver(drivers, dcnt, p);
            if (idx >= 0) {
                drivers[idx].alive = 0;
                close(drivers[idx].fd);
                drivers[idx].fd = -1;
                printf("\nDriver %d завершился\n", (int)p);
                printf("taxi> ");
                fflush(stdout);
            }
        }

        struct epoll_event events[32];
        int n = epoll_wait(ep, events, 32, 200);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.u32 == 1000) {
                char cmdline[MAX_LINE];
                if (!fgets(cmdline, sizeof(cmdline), stdin)) {
                    running = 0;
                    break;
                }
                cmdline[strcspn(cmdline, "\r\n")] = '\0';

                if (strcmp(cmdline, "") == 0) {
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                if (strcmp(cmdline, "help") == 0) {
                    print_help();
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                if (strcmp(cmdline, "exit") == 0) {
                    running = 0;
                    break;
                }

                if (strcmp(cmdline, "create_driver") == 0) {
                    if (dcnt >= MAX_DRIVERS) {
                        printf("Слишком много drivers\n");
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }

                    int sp[2];
                    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sp) < 0) {
                        perror("socketpair");
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork");
                        close(sp[0]);
                        close(sp[1]);
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }

                    if (pid == 0) {
                        close(sp[0]);
                        set_nonblock(sp[1]);
                        driver_loop(sp[1]);
                        _exit(0);
                    }

                    close(sp[1]);
                    set_nonblock(sp[0]);

                    drivers[dcnt].pid = pid;
                    drivers[dcnt].fd = sp[0];
                    drivers[dcnt].alive = 1;

                    // fd в epoll
                    memset(&ev, 0, sizeof(ev));
                    ev.events = EPOLLIN;
                    ev.data.u32 = (unsigned)pid; // метка = pid
                    epoll_ctl(ep, EPOLL_CTL_ADD, sp[0], &ev);

                    dcnt++;

                    printf("Создан driver pid=%d\n", (int)pid);
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(cmdline, "send_task ", 10) == 0) {
                    int pid = 0, sec = 0;
                    if (sscanf(cmdline + 10, "%d %d", &pid, &sec) != 2) {
                        printf("Формат: send_task <pid> <seconds>\n");
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }
                    int idx = find_driver(drivers, dcnt, (pid_t)pid);
                    if (idx < 0) {
                        printf("Driver %d не найден\n", pid);
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }
                    char msg[64];
                    snprintf(msg, sizeof(msg), "TASK %d\n", sec);
                    write_all(drivers[idx].fd, msg, strlen(msg));
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(cmdline, "get_status ", 11) == 0) {
                    int pid = 0;
                    if (sscanf(cmdline + 11, "%d", &pid) != 1) {
                        printf("Формат: get_status <pid>\n");
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }
                    int idx = find_driver(drivers, dcnt, (pid_t)pid);
                    if (idx < 0) {
                        printf("Driver %d не найден\n", pid);
                        printf("taxi> ");
                        fflush(stdout);
                        continue;
                    }
                    const char *q = "STATUS\n";
                    write_all(drivers[idx].fd, q, strlen(q));
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                if (strcmp(cmdline, "get_drivers") == 0) {
                    int any = 0;
                    for (int j = 0; j < dcnt; j++) {
                        if (!drivers[j].alive) continue;
                        any = 1;
                        const char *q = "STATUS\n";
                        write_all(drivers[j].fd, q, strlen(q));
                    }
                    if (!any) printf("Drivers нет\n");
                    printf("taxi> ");
                    fflush(stdout);
                    continue;
                }

                printf("Неизвестная команда\n");
                printf("taxi> ");
                fflush(stdout);
                continue;
            } else {
                pid_t pid = (pid_t)events[i].data.u32;
                int idx = find_driver(drivers, dcnt, pid);
                if (idx < 0) continue;

                char line[MAX_LINE];
                int got = 0;

                while ((got = read_line_nb(drivers[idx].fd, line, sizeof(line))) == 1) {
                    line[strcspn(line, "\r\n")] = '\0';
                    printf("\n[%d] %s\n", (int)pid, line);
                    printf("taxi> ");
                    fflush(stdout);
                }

                if (got == -1) {
                    drivers[idx].alive = 0;
                    close(drivers[idx].fd);
                    drivers[idx].fd = -1;
                    printf("\nDriver %d отключился\n", (int)pid);
                    printf("taxi> ");
                    fflush(stdout);
                }
            }
        }
    }

    for (int i = 0; i < dcnt; i++) {
        if (!drivers[i].alive) continue;
        const char *q = "QUIT\n";
        write_all(drivers[i].fd, q, strlen(q));
    }

    for (int i = 0; i < dcnt; i++) {
        if (!drivers[i].alive) continue;
        kill(drivers[i].pid, SIGTERM);
    }

    while (waitpid(-1, NULL, 0) > 0) {}

    for (int i = 0; i < dcnt; i++) {
        if (drivers[i].fd >= 0) close(drivers[i].fd);
    }
    close(ep);

    printf("\nВыход. Всего хорошего!\n");
    return 0;
}

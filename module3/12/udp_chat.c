#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#define MAX_MSG 1024

#define W(x) do { if ((x) < 0) {} } while (0)

static volatile sig_atomic_t running = 1;

static int sockfd = -1;
static struct sockaddr_in peer_addr;

static pthread_mutex_t line_mx = PTHREAD_MUTEX_INITIALIZER;
static char cur_line[MAX_MSG];
static size_t cur_len = 0;

static struct termios old_term;

static void term_restore(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
}

static void term_raw(void) {
    tcgetattr(STDIN_FILENO, &old_term);
    atexit(term_restore);

    struct termios t = old_term;
    t.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
    if (sockfd != -1) close(sockfd);
}

static void redraw_prompt_locked(void) {
    W(write(STDOUT_FILENO, "\r\033[2K", 5));
    W(write(STDOUT_FILENO, "you> ", 5));
    if (cur_len > 0) W(write(STDOUT_FILENO, cur_line, cur_len));
    fflush(stdout);
}

static void *recv_thread(void *arg) {
    (void)arg;

    char buf[MAX_MSG];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    while (running) {
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&from, &fromlen);
        if (n <= 0) break;

        buf[n] = '\0';

        pthread_mutex_lock(&line_mx);

        W(write(STDOUT_FILENO, "\r\033[2K", 5));

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));

        char msgline[MAX_MSG + 64];
        int m = snprintf(msgline, sizeof(msgline),
                         "< %s:%d > %s\n", ip, ntohs(from.sin_port), buf);
        if (m > 0) W(write(STDOUT_FILENO, msgline, (size_t)m));

        redraw_prompt_locked();
        pthread_mutex_unlock(&line_mx);

        if (strcmp(buf, "exit") == 0) {
            running = 0;
            break;
        }
    }

    return NULL;
}

static void send_text(const char *s) {
    sendto(sockfd, s, strlen(s) + 1, 0,
           (struct sockaddr *)&peer_addr, sizeof(peer_addr));
}

static void *send_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&line_mx);
    cur_len = 0;
    cur_line[0] = '\0';
    redraw_prompt_locked();
    pthread_mutex_unlock(&line_mx);

    while (running) {
        unsigned char c;
        ssize_t r = read(STDIN_FILENO, &c, 1);
        if (r <= 0) break;

        pthread_mutex_lock(&line_mx);

        if (c == '\r' || c == '\n') {
            cur_line[cur_len] = '\0';
            W(write(STDOUT_FILENO, "\n", 1));

            if (cur_len > 0) {
                send_text(cur_line);
                if (strcmp(cur_line, "exit") == 0) {
                    running = 0;
                    pthread_mutex_unlock(&line_mx);
                    break;
                }
            }

            cur_len = 0;
            cur_line[0] = '\0';
            redraw_prompt_locked();
            pthread_mutex_unlock(&line_mx);
            continue;
        }

        if (c == 127 || c == '\b') {
            if (cur_len > 0) {
                cur_len--;
                cur_line[cur_len] = '\0';
            }
            redraw_prompt_locked();
            pthread_mutex_unlock(&line_mx);
            continue;
        }

        if (c >= 32 && c <= 126) {
            if (cur_len + 1 < sizeof(cur_line)) {
                cur_line[cur_len++] = (char)c;
                cur_line[cur_len] = '\0';
            }
            redraw_prompt_locked();
            pthread_mutex_unlock(&line_mx);
            continue;
        }

        pthread_mutex_unlock(&line_mx);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Использование: %s <1|2>\n", argv[0]);
        return 1;
    }

    int me = atoi(argv[1]);
    int my_port   = (me == 1) ? 5000 : 5001;
    int peer_port = (me == 1) ? 5001 : 5000;

    signal(SIGINT, on_sigint);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons((uint16_t)my_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr));

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons((uint16_t)peer_port);
    inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);

    printf("UDP чат #%d. Я слушаю %d, собеседник 127.0.0.1:%d\n", me, my_port, peer_port);
    printf("exit(Ctrl+C) для завершения\n\n");

    term_raw();

    pthread_t tr, ts;
    pthread_create(&tr, NULL, recv_thread, NULL);
    pthread_create(&ts, NULL, send_thread, NULL);

    pthread_join(ts, NULL);
    running = 0;
    pthread_join(tr, NULL);

    printf("\nЧат завершён\n");
    return 0;
}

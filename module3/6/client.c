#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <unistd.h>

#define KEY_PATH "."
#define KEY_ID   'S'

#define SERVER_ID 10
#define MAX_TEXT  256

typedef struct {
    long mtype;
    int sender;
    char text[MAX_TEXT];
} ChatMsg;

static int parse_client_id(const char *s) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s[0] || (end && *end) || v <= SERVER_ID || (v % 10) != 0) return -1;
    return (int)v;
}

static void recv_all(int qid, int my_id) {
    for (;;) {
        ChatMsg msg;
        memset(&msg, 0, sizeof(msg));

        ssize_t r = msgrcv(qid, &msg, sizeof(msg) - sizeof(long), my_id, IPC_NOWAIT);
        if (r < 0) {
            if (errno == ENOMSG) break;
            if (errno == EINTR) continue;
            perror("msgrcv (клиент)");
            break;
        }

        printf("\n[%d -> мне %d] %s\n", msg.sender, my_id, msg.text);
        printf("чат(%d)> ", my_id);
        fflush(stdout);
    }
}

static int send_to_server(int qid, int my_id, const char *text) {
    ChatMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.mtype = SERVER_ID;
    msg.sender = my_id;
    snprintf(msg.text, MAX_TEXT, "%s", text);

    if (msgsnd(qid, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("msgsnd (клиент->сервер)");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <ID клиента: 20/30/40/...>\n", argv[0]);
        return 1;
    }

    int my_id = parse_client_id(argv[1]);
    if (my_id < 0) {
        fprintf(stderr, "Ошибка: ID должен быть > 10 и кратен 10 (пример: 20, 30, 40)\n");
        return 1;
    }

    key_t key = ftok(KEY_PATH, KEY_ID);
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    int qid = msgget(key, 0666);
    if (qid < 0) {
        perror("msgget (очередь не найдена, запусти сервер)");
        return 1;
    }

    printf("Клиент %d запущен. Пиши сообщения и жми Enter. 'shutdown' отключит пересылку тебе.\n", my_id);

    // Сразу регистрируемся на сервере
    if (send_to_server(qid, my_id, "join") < 0) return 1;

    char line[MAX_TEXT];

    for (;;) {
        printf("чат(%d)> ", my_id);
        fflush(stdout);

        for (;;) {
            recv_all(qid, my_id);

            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 200000;

            int rc = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
            if (rc < 0) {
                if (errno == EINTR) continue;
                perror("select");
                return 1;
            }

            if (rc == 0) continue;

            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                if (!fgets(line, sizeof(line), stdin)) {
                    printf("\nEOF. Выход.\n");
                    return 0;
                }
                line[strcspn(line, "\n")] = '\0';
                break;
            }
        }

        if (send_to_server(qid, my_id, line) < 0) {
            return 1;
        }

        if (strcmp(line, "shutdown") == 0) {
            printf("Сервер отключит пересылку сообщений этому клиенту (%d).\n", my_id);
            break;
        }
    }

    return 0;
}

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define KEY_PATH "."
#define KEY_ID   'S'

#define SERVER_ID 10
#define MAX_TEXT  256
#define MAX_CLIENTS 64

typedef struct {
    long mtype;
    int sender;
    char text[MAX_TEXT];
} ChatMsg;

typedef struct {
    int id;       // 20/30/40/и т.д.
    int active;
} ClientInfo;

static int qid = -1;
static ClientInfo clients[MAX_CLIENTS];
static size_t clients_cnt = 0;

static void cleanup(int sig) {
    (void)sig;

    if (qid != -1) {
        msgctl(qid, IPC_RMID, NULL);
        qid = -1;
    }

    const char *msg = "\nСервер остановлен, очередь удалена\n";
    if (write(STDOUT_FILENO, msg, strlen(msg)) < 0) {
    }

    _exit(0);
}

static int find_client(int id) {
    for (size_t i = 0; i < clients_cnt; i++) {
        if (clients[i].id == id) return (int)i;
    }
    return -1;
}

static void ensure_client(int id) {
    int idx = find_client(id);
    if (idx >= 0) return;

    if (clients_cnt >= MAX_CLIENTS) {
        printf("Лимит клиентов достигнут, клиент %d не добавлен\n", id);
        return;
    }

    clients[clients_cnt].id = id;
    clients[clients_cnt].active = 1;
    clients_cnt++;

    printf("Подключён клиент %d\n", id);
}

static void deactivate_client(int id) {
    int idx = find_client(id);
    if (idx < 0) return;

    clients[idx].active = 0;
    printf("Клиент %d отключён командой shutdown\n", id);
}

static void broadcast_except_sender(const ChatMsg *in) {
    for (size_t i = 0; i < clients_cnt; i++) {
        if (!clients[i].active) continue;
        if (clients[i].id == in->sender) continue;

        ChatMsg out;
        memset(&out, 0, sizeof(out));
        out.mtype  = clients[i].id;
        out.sender = in->sender;
        snprintf(out.text, MAX_TEXT, "%s", in->text);

        if (msgsnd(qid, &out, sizeof(out) - sizeof(long), 0) < 0) {
            perror("msgsnd (сервер->клиент)");
        }
    }
}

int main(void) {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    key_t key = ftok(KEY_PATH, KEY_ID);
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        perror("msgget");
        return 1;
    }

    printf("Сервер запущен. Очередь создана. mtype=%d для сообщений на сервер.\n", SERVER_ID);

    for (;;) {
        ChatMsg msg;
        memset(&msg, 0, sizeof(msg));

        if (msgrcv(qid, &msg, sizeof(msg) - sizeof(long), SERVER_ID, 0) < 0) {
            if (errno == EINTR) continue;
            perror("msgrcv (сервер)");
            break;
        }

        ensure_client(msg.sender);

        if (strcmp(msg.text, "join") == 0) {
            printf("Клиент %d зарегистрирован\n", msg.sender);
            continue;
        }

        printf("От %d: %s\n", msg.sender, msg.text);

        if (strcmp(msg.text, "shutdown") == 0) {
            deactivate_client(msg.sender);
            continue;
        }

        broadcast_except_sender(&msg);
    }

    cleanup(0);
    return 0;
}

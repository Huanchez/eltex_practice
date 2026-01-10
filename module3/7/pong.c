#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define Q_PING2PONG "/mq_ping2pong"
#define Q_PONG2PING "/mq_pong2ping"

#define MAX_MSG 256

#define PRIO_NORMAL 5
#define PRIO_STOP   99

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void chomp(char *s) {
    if (!s) return;
    s[strcspn(s, "\n")] = '\0';
}

static mqd_t open_retry(const char *name, int flags) {
    for (;;) {
        mqd_t q = mq_open(name, flags);
        if (q != (mqd_t)-1) return q;
        if (errno != ENOENT) die("mq_open");
        sleep(200000);
    }
}

int main(void) {
    // понг подключается к уже созданным очередям
    mqd_t q_recv = open_retry(Q_PING2PONG, O_RDONLY);
    mqd_t q_send = open_retry(Q_PONG2PING, O_WRONLY);

    printf("PONG запущен\n");
    printf("exit для завершения\n");

    char buf[MAX_MSG];
    unsigned int prio = PRIO_NORMAL;

    while (1) {
        // ждём сообщение от пинга
        memset(buf, 0, sizeof(buf));
        if (mq_receive(q_recv, buf, sizeof(buf), &prio) == -1) die("mq_receive");

        printf("PING> %s\n", buf);

        if (prio == PRIO_STOP) {
            const char *bye = "exit";
            if (mq_send(q_send, bye, strlen(bye) + 1, PRIO_STOP) == -1) die("mq_send stop");
            printf("PONG: получен сигнал завершения, отправлен ответ завершения\n");
            break;
        }

        printf("PONG> ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) {
            snprintf(buf, sizeof(buf), "exit");
            prio = PRIO_STOP;
        } else {
            chomp(buf);
            prio = (strcmp(buf, "exit") == 0) ? PRIO_STOP : PRIO_NORMAL;
        }

        if (mq_send(q_send, buf, strlen(buf) + 1, prio) == -1) die("mq_send");

        if (prio == PRIO_STOP) {
            printf("PONG: отправлен сигнал завершения\n");
            break;
        }
    }

    mq_close(q_send);
    mq_close(q_recv);

    printf("PONG завершён\n");
    return 0;
}

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

#define MAX_MSG   256
#define MAX_COUNT 10

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

int main(void) {
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = MAX_COUNT;
    attr.mq_msgsize = MAX_MSG;

    // пинг создаёт очереди, предварительно удаляя старые
    mq_unlink(Q_PING2PONG);
    mq_unlink(Q_PONG2PING);

    mqd_t q_send = mq_open(Q_PING2PONG, O_CREAT | O_WRONLY, 0666, &attr);
    if (q_send == (mqd_t)-1) die("mq_open ping->pong");

    mqd_t q_recv = mq_open(Q_PONG2PING, O_CREAT | O_RDONLY, 0666, &attr);
    if (q_recv == (mqd_t)-1) die("mq_open pong->ping");

    printf("PING запущен\n");
    printf("exit для завершения\n");

    char buf[MAX_MSG];
    unsigned int prio = PRIO_NORMAL;

    while (1) {
        printf("PING> ");
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
            printf("PING: отправлен сигнал завершения\n");
            break;
        }

        // ждём ответ от понга
        memset(buf, 0, sizeof(buf));
        if (mq_receive(q_recv, buf, sizeof(buf), &prio) == -1) die("mq_receive");

        printf("PONG> %s\n", buf);

        if (prio == PRIO_STOP) {
            printf("PING: получен сигнал завершения\n");
            break;
        }
    }

    mq_close(q_send);
    mq_close(q_recv);

    mq_unlink(Q_PING2PONG);
    mq_unlink(Q_PONG2PING);

    printf("PING завершён\n");
    return 0;
}

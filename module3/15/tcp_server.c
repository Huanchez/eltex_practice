/*
   Пример простого TCP сервера
   Порт является аргументом, для запуска сервера необходимо ввести:
   ./server <номер_порта>
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

// прототип функции, обслуживающей подключившихся пользователей
void dostuff(int);

// Функция обработки ошибок
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// глобальная переменная – количество активных пользователей
int nclients = 0;

// макрос для печати количества активных пользователей
void printusers()
{ 
    if (nclients)
        printf("%d user on-line\n", nclients);
    else
        printf("No User on line\n");
}

// функция обработки арифметических операций
int myfunc(int a, int b, char op)
{
    switch (op)
    {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? a / b : 0;
        default:  return 0;
    }
}

// функция приема файла от клиента
void receive_file(int sock);

static int set_nb(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

typedef enum {
    ST_RECV_NAME_LEN = 0,
    ST_RECV_NAME,
    ST_RECV_FILE_SIZE,
    ST_RECV_FILE_DATA,
    ST_SEND_OP,
    ST_RECV_OP,
    ST_SEND_A,
    ST_RECV_A,
    ST_SEND_B,
    ST_RECV_B,
    ST_SEND_RESULT,
    ST_DONE
} State;

typedef struct {
    int fd;
    State st;

    unsigned char inbuf[4096];
    size_t inlen;

    char outbuf[2048];
    size_t outlen;
    size_t outpos;

    size_t name_len;
    char filename[256];

    off_t file_size;
    off_t file_received;
    FILE *fp;

    char op;
    int a;
    int b;
} Client;

static Client *clients[8192];

static void cli_close(int epfd, int fd)
{
    if (fd < 0 || fd >= (int)(sizeof(clients) / sizeof(clients[0]))) {
        close(fd);
        return;
    }

    Client *c = clients[fd];
    if (c) {
        if (c->fp) fclose(c->fp);
        free(c);
        clients[fd] = NULL;
    }

    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    if (nclients > 0) nclients--;
    printf("-disconnect\n");
    printusers();
}

static void want_write(int epfd, int fd, int on)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;

    uint32_t events = EPOLLIN;
    if (on) events |= EPOLLOUT;
    ev.events = events;

    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

static void queue_send(int epfd, Client *c, const char *s)
{
    size_t n = strlen(s);
    if (n >= sizeof(c->outbuf)) n = sizeof(c->outbuf) - 1;
    memcpy(c->outbuf, s, n);
    c->outbuf[n] = '\0';
    c->outlen = n;
    c->outpos = 0;
    want_write(epfd, c->fd, 1);
}

static int flush_send(int epfd, Client *c)
{
    while (c->outpos < c->outlen) {
        ssize_t w = send(c->fd, c->outbuf + c->outpos, c->outlen - c->outpos, 0);
        if (w > 0) {
            c->outpos += (size_t)w;
            continue;
        }
        if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            want_write(epfd, c->fd, 1);
            return 0;
        }
        return -1;
    }

    c->outlen = 0;
    c->outpos = 0;
    want_write(epfd, c->fd, 0);
    return 0;
}

static int need_bytes(Client *c, size_t want)
{
    return c->inlen >= want;
}

static void consume_bytes(Client *c, size_t n)
{
    if (n >= c->inlen) {
        c->inlen = 0;
        return;
    }
    memmove(c->inbuf, c->inbuf + n, c->inlen - n);
    c->inlen -= n;
}

static int read_more(Client *c)
{
    for (;;) {
        ssize_t r = recv(c->fd, c->inbuf + c->inlen, sizeof(c->inbuf) - c->inlen, 0);
        if (r > 0) {
            c->inlen += (size_t)r;
            if (c->inlen == sizeof(c->inbuf)) return 0;
            continue;
        }
        if (r == 0) return -1;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
}

static void kick_next_step(int epfd, Client *c)
{
    #define STR_OP "Введите операцию (+ - * /)\n"
    #define STR_A  "Введите первое число: \n"
    #define STR_B  "Введите второе число:\n"

    if (c->st == ST_SEND_OP) {
        queue_send(epfd, c, STR_OP);
        c->st = ST_RECV_OP;
        return;
    }

    if (c->st == ST_SEND_A) {
        queue_send(epfd, c, STR_A);
        c->st = ST_RECV_A;
        return;
    }

    if (c->st == ST_SEND_B) {
        queue_send(epfd, c, STR_B);
        c->st = ST_RECV_B;
        return;
    }

    if (c->st == ST_SEND_RESULT) {
        char buff[1024];
        int res = myfunc(c->a, c->b, c->op);
        snprintf(buff, sizeof(buff), "Результат = %d\n", res);
        queue_send(epfd, c, buff);
        c->st = ST_DONE;
        return;
    }
}

static int take_line(Client *c, char *out, size_t cap)
{
    for (size_t i = 0; i < c->inlen; i++) {
        if (c->inbuf[i] == '\n') {
            size_t n = i + 1;
            if (n >= cap) n = cap - 1;
            memcpy(out, c->inbuf, n);
            out[n] = '\0';
            consume_bytes(c, i + 1);
            return 1;
        }
    }
    return 0;
}

static void step_in(int epfd, Client *c)
{
    if (c->st == ST_RECV_NAME_LEN) {
        if (!need_bytes(c, sizeof(size_t))) return;
        memcpy(&c->name_len, c->inbuf, sizeof(size_t));
        consume_bytes(c, sizeof(size_t));
        if (c->name_len >= sizeof(c->filename)) c->name_len = sizeof(c->filename) - 1;
        c->st = ST_RECV_NAME;
    }

    if (c->st == ST_RECV_NAME) {
        if (!need_bytes(c, c->name_len)) return;
        memcpy(c->filename, c->inbuf, c->name_len);
        c->filename[c->name_len] = '\0';
        consume_bytes(c, c->name_len);
        c->st = ST_RECV_FILE_SIZE;
    }

    if (c->st == ST_RECV_FILE_SIZE) {
        if (!need_bytes(c, sizeof(off_t))) return;
        memcpy(&c->file_size, c->inbuf, sizeof(off_t));
        consume_bytes(c, sizeof(off_t));
        c->file_received = 0;
        c->fp = fopen(c->filename, "wb");
        c->st = ST_RECV_FILE_DATA;
    }

    if (c->st == ST_RECV_FILE_DATA) {
        if (!c->fp) {
            cli_close(epfd, c->fd);
            return;
        }

        while (c->file_received < c->file_size && c->inlen > 0) {
            off_t need = c->file_size - c->file_received;
            size_t take = c->inlen;
            if ((off_t)take > need) take = (size_t)need;

            fwrite(c->inbuf, 1, take, c->fp);
            c->file_received += (off_t)take;
            consume_bytes(c, take);
        }

        if (c->file_received >= c->file_size) {
            fclose(c->fp);
            c->fp = NULL;
            printf("Файл получен: %s (%ld bytes)\n", c->filename, (long)c->file_size);
            c->st = ST_SEND_OP;
            kick_next_step(epfd, c);
        }
    }

    if (c->st == ST_RECV_OP) {
        char line[128];
        if (!take_line(c, line, sizeof(line))) return;
        c->op = line[0];
        c->st = ST_SEND_A;
        kick_next_step(epfd, c);
    }

    if (c->st == ST_RECV_A) {
        char line[128];
        if (!take_line(c, line, sizeof(line))) return;
        c->a = atoi(line);
        c->st = ST_SEND_B;
        kick_next_step(epfd, c);
    }

    if (c->st == ST_RECV_B) {
        char line[128];
        if (!take_line(c, line, sizeof(line))) return;
        c->b = atoi(line);
        c->st = ST_SEND_RESULT;
        kick_next_step(epfd, c);
    }
}

int main(int argc, char *argv[])
{
    char buff[1024];    // буфер для различных нужд
    printf("TCP SERVER DEMO\n");

    int sockfd, newsockfd; // дескрипторы сокетов
    int portno;            // номер порта
    socklen_t clilen;      // размер адреса клиента
    struct sockaddr_in serv_addr, cli_addr; // структура сокета сервера и клиента

    // ошибка, если порт не указан
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Шаг 1 - создание сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Шаг 2 - связывание сокета с локальным адресом
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;    // сервер принимает подключения на все IP-адреса
    serv_addr.sin_port = htons(portno);

    // вызываем bind для связывания
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Шаг 3 - ожидание подключений
    listen(sockfd, 32);
    clilen = sizeof(cli_addr);

    set_nb(sockfd);

    int epfd = epoll_create1(0);
    if (epfd < 0) error("epoll_create1");

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) error("epoll_ctl");

    // Шаг 4 - извлекаем сообщения из очереди
    while (1)
    {
        struct epoll_event events[64];
        int n = epoll_wait(epfd, events, 64, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == sockfd) {
                for (;;) {
                    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
                    if (newsockfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        break;
                    }

                    set_nb(newsockfd);

                    Client *c = (Client *)calloc(1, sizeof(Client));
                    c->fd = newsockfd;
                    c->st = ST_RECV_NAME_LEN;

                    if (newsockfd < (int)(sizeof(clients) / sizeof(clients[0])))
                        clients[newsockfd] = c;

                    nclients++; // увеличиваем счетчик подключившихся клиентов
                    printusers();

                    struct epoll_event cev;
                    memset(&cev, 0, sizeof(cev));
                    cev.events = EPOLLIN;
                    cev.data.fd = newsockfd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, newsockfd, &cev);
                }
                continue;
            }

            Client *c = NULL;
            if (fd >= 0 && fd < (int)(sizeof(clients) / sizeof(clients[0])))
                c = clients[fd];
            if (!c) {
                close(fd);
                continue;
            }

            if (events[i].events & EPOLLOUT) {
                if (flush_send(epfd, c) < 0) {
                    cli_close(epfd, fd);
                    continue;
                }
                if (c->st == ST_DONE && c->outlen == 0) {
                    cli_close(epfd, fd);
                    continue;
                }
            }

            if (events[i].events & EPOLLIN) {
                if (read_more(c) < 0) {
                    cli_close(epfd, fd);
                    continue;
                }
                step_in(epfd, c);
            }
        }
    }

    close(epfd);
    close(sockfd);
    return 0;
}

void dostuff(int sock)
{
    (void)sock;
}

void receive_file(int sock)
{
    (void)sock;
}

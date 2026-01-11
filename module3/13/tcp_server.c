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
void receive_file(int sock)
{
    size_t name_len;
    off_t file_size;
    char filename[256];

    // принимаем длину имени файла
    read(sock, &name_len, sizeof(name_len));
    // принимаем имя файла
    read(sock, filename, name_len);
    filename[name_len] = '\0';

    // принимаем размер файла
    read(sock, &file_size, sizeof(file_size));

    FILE *fp = fopen(filename, "wb");
    if (!fp) return;

    char buffer[1024];
    ssize_t bytes;
    off_t received = 0;

    // принимаем файл частями
    while (received < file_size)
    {
        bytes = read(sock, buffer, sizeof(buffer));
        fwrite(buffer, 1, bytes, fp);
        received += bytes;
    }

    fclose(fp);
    printf("File received: %s (%ld bytes)\n", filename, file_size);
}

int main(int argc, char *argv[])
{
    char buff[1024];    // буфер для различных нужд
    printf("TCP SERVER DEMO\n");

    int sockfd, newsockfd; // дескрипторы сокетов
    int portno;            // номер порта
    int pid;               // id процесса
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
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Шаг 4 - извлекаем сообщения из очереди
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        nclients++; // увеличиваем счетчик подключившихся клиентов
        printusers();

        pid = fork();
        if (pid < 0)
            error("ERROR on fork");

        if (pid == 0)
        {
            close(sockfd);

            // сначала принимаем файл
            receive_file(newsockfd);

            // затем выполняем вычисления
            dostuff(newsockfd);
            exit(0);
        }
        else close(newsockfd);
    }

    close(sockfd);
    return 0;
}

void dostuff(int sock)
{
    int bytes_recv; // размер принятого сообщения
    int a, b; // переменные для myfunc
    char op;
    char buff[1024];

    #define STR_OP "Enter operation (+ - * /)\n"
    #define STR_A  "Enter first number\n"
    #define STR_B  "Enter second number\n"

    // запрос операции
    write(sock, STR_OP, strlen(STR_OP));
    bytes_recv = read(sock, buff, sizeof(buff));
    op = buff[0];

    // запрос первого числа
    write(sock, STR_A, strlen(STR_A));
    bytes_recv = read(sock, buff, sizeof(buff));
    a = atoi(buff);

    // запрос второго числа
    write(sock, STR_B, strlen(STR_B));
    bytes_recv = read(sock, buff, sizeof(buff));
    b = atoi(buff);

    int res = myfunc(a, b, op);
    snprintf(buff, sizeof(buff), "Result = %d\n", res);

    // отправляем клиенту результат
    write(sock, buff, strlen(buff));

    nclients--; // уменьшаем счетчик активных клиентов
    printf("-disconnect\n");
    printusers();
}

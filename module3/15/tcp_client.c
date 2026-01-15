#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void send_file(int sock, const char *filename)
{
    struct stat st;
    if (stat(filename, &st) != 0) {
        // если файла нет — отправим пустой файл
        size_t name_len = strlen(filename);
        off_t file_size = 0;

        write(sock, &name_len, sizeof(name_len));
        write(sock, filename, name_len);
        write(sock, &file_size, sizeof(file_size));
        return;
    }

    size_t name_len = strlen(filename);
    off_t file_size = st.st_size;

    write(sock, &name_len, sizeof(name_len));
    write(sock, filename, name_len);
    write(sock, &file_size, sizeof(file_size));

    FILE *fp = fopen(filename, "rb");
    if (!fp) return;

    char buffer[1024];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        write(sock, buffer, bytes);

    fclose(fp);
}


int main(int argc, char *argv[])
{
    int my_sock, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buff[1024];
    printf("TCP DEMO CLIENT\n");

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    // извлечение порта
    portno = atoi(argv[2]);

    // Шаг 1 - создание сокета
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0)
        error("ERROR opening socket");

    // извлечение хоста
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    // заполнение структуры serv_addr
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(portno);

    // Шаг 2 - установка соединения
    if (connect(my_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    send_file(my_sock, "example.txt");

    // Шаг 3 - чтение и передача сообщений
    while ((n = recv(my_sock, buff, sizeof(buff) - 1, 0)) > 0)
    {
        // ставим завершающий ноль в конце строки
        buff[n] = 0;
        printf("S=>C: %s", buff);   // выводим на экран

        printf("S<=C: ");           // читаем пользовательский ввод с клавиатуры
        fgets(buff, sizeof(buff), stdin);

        // проверка на "quit"
        if (!strcmp(buff, "quit\n"))
        {
            printf("Exit...\n");
            close(my_sock);
            return 0;
        }
        
        // передаем строку клиента серверу
        send(my_sock, buff, strlen(buff), 0);
    }

    close(my_sock);
    return 0;
}

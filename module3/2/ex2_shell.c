#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_BUF 512
#define MAX_ARGS 32

static void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

static int split_args(char *buf, char *args[], int max_args) {
    int count = 0;
    char *token = strtok(buf, " \t");
    while (token != NULL && count < max_args - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t");
    }
    args[count] = NULL;
    return count;
}

int main(void)
{
    char buf[MAX_BUF];
    char *args[MAX_ARGS];

    while (1) {
        printf("Интерпретатор> ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) {
            printf("\n");
            break;
        }

        trim_newline(buf);

        if (buf[0] == '\0') continue;
        if (strcmp(buf, "exit") == 0) break;

        int argc = split_args(buf, args, MAX_ARGS);
        if (argc == 0) continue;

        pid_t pid = fork();
        if (pid < 0) {
            perror("Ошибка fork");
            continue;
        }

        if (pid == 0) {
            execvp(args[0], args);
            if (errno == ENOENT && strchr(args[0], '/') == NULL) {
                char local_path[MAX_BUF];
                snprintf(local_path, sizeof(local_path), "./%s", args[0]);
                execv(local_path, args);
            }

            // Если запуск не удался
            if (errno == ENOENT)
                fprintf(stderr, "Программа не найдена: %s\n", args[0]);
            else
                perror("Ошибка запуска");

            _exit(127);
        } else {
            int status = 0;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}

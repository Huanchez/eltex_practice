#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_BUF 1024
#define MAX_ARGS 64

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    const char *in_file;
    const char *out_file;
} Command;

typedef struct {
    Command *cmds;
    size_t ncmd;
    const char *in_file;
    const char *out_file;
} Pipeline;

static void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

static int is_special(const char *t) {
    return (strcmp(t, "|") == 0) || (strcmp(t, "<") == 0) || (strcmp(t, ">") == 0);
}

static int tokenize(char *buf, char *tokens[], int max_tokens) {
    static char expanded[3 * MAX_BUF];
    size_t j = 0;

    for (size_t i = 0; buf[i] != '\0' && j + 4 < sizeof(expanded); i++) {
        char c = buf[i];
        if (c == '|' || c == '<' || c == '>') {
            expanded[j++] = ' ';
            expanded[j++] = c;
            expanded[j++] = ' ';
        } else {
            expanded[j++] = c;
        }
    }
    expanded[j] = '\0';

    int n = 0;
    char *save = NULL;
    char *tok = strtok_r(expanded, " \t", &save);
    while (tok && n < max_tokens - 1) {
        tokens[n++] = tok;
        tok = strtok_r(NULL, " \t", &save);
    }
    tokens[n] = NULL;
    return n;
}


static void pipeline_free(Pipeline *pl) {
    if (!pl) return;
    free(pl->cmds);
    pl->cmds = NULL;
    pl->ncmd = 0;
    pl->in_file = NULL;
    pl->out_file = NULL;
}

static int pipeline_parse(char *line, Pipeline *pl) {
    char *tokens[3 * MAX_ARGS];
    int ntok = tokenize(line, tokens, (int)(sizeof(tokens)/sizeof(tokens[0])));

    pl->cmds = NULL;
    pl->ncmd = 0;
    pl->in_file = NULL;
    pl->out_file = NULL;

    if (ntok == 0) return 0;

    size_t cap = 4;
    pl->cmds = (Command*)calloc(cap, sizeof(Command));
    if (!pl->cmds) {
        perror("calloc");
        return -1;
    }

    pl->ncmd = 1;
    Command *cur = &pl->cmds[0];
    cur->argc = 0;
    cur->in_file = NULL;
    cur->out_file = NULL;

    int i = 0;
    while (i < ntok) {
        char *t = tokens[i];

        if (strcmp(t, "|") == 0) {
            if (cur->argc == 0) {
                fprintf(stderr, "Ошибка: пустая команда перед '|'\n");
                pipeline_free(pl);
                return -1;
            }
            cur->argv[cur->argc] = NULL;

            if (pl->ncmd == cap) {
                cap *= 2;
                Command *tmp = (Command*)realloc(pl->cmds, cap * sizeof(Command));
                if (!tmp) {
                    perror("realloc");
                    pipeline_free(pl);
                    return -1;
                }
                pl->cmds = tmp;
            }

            cur = &pl->cmds[pl->ncmd];
            memset(cur, 0, sizeof(*cur));
            cur->argc = 0;
            pl->ncmd++;
            i++;
            continue;
        }

        if (strcmp(t, "<") == 0) {
            if (i + 1 >= ntok || is_special(tokens[i + 1])) {
                fprintf(stderr, "Ошибка: после '<' должен быть файл\n");
                pipeline_free(pl);
                return -1;
            }
            if (pl->in_file) {
                fprintf(stderr, "Ошибка: повторный ввод '<'\n");
                pipeline_free(pl);
                return -1;
            }
            pl->in_file = tokens[i + 1];
            i += 2;
            continue;
        }

        if (strcmp(t, ">") == 0) {
            if (i + 1 >= ntok || is_special(tokens[i + 1])) {
                fprintf(stderr, "Ошибка: после '>' должен быть файл\n");
                pipeline_free(pl);
                return -1;
            }
            if (pl->out_file) {
                fprintf(stderr, "Ошибка: повторный вывод '>'\n");
                pipeline_free(pl);
                return -1;
            }
            pl->out_file = tokens[i + 1];
            i += 2;
            continue;
        }

        if (cur->argc >= MAX_ARGS - 1) {
            fprintf(stderr, "Ошибка: слишком много аргументов в команде\n");
            pipeline_free(pl);
            return -1;
        }
        cur->argv[cur->argc++] = t;
        i++;
    }

    if (cur->argc == 0) {
        fprintf(stderr, "Ошибка: пустая команда в конце\n");
        pipeline_free(pl);
        return -1;
    }
    cur->argv[cur->argc] = NULL;

    return 0;
}

static void exec_command(char *const argv[]) {
    execvp(argv[0], argv);

    if (errno == ENOENT && strchr(argv[0], '/') == NULL) {
        char local_path[4096];
        snprintf(local_path, sizeof(local_path), "./%s", argv[0]);
        execv(local_path, argv);
    }

    if (errno == ENOENT)
        fprintf(stderr, "Программа не найдена: %s\n", argv[0]);
    else
        perror("Ошибка запуска");

    _exit(127);
}

static int run_pipeline(Pipeline *pl) {
    if (!pl || pl->ncmd == 0) return 0;

    int in_fd = -1;
    int out_fd = -1;

    if (pl->in_file) {
        in_fd = open(pl->in_file, O_RDONLY);
        if (in_fd < 0) {
            perror("open (<)");
            return -1;
        }
    }

    if (pl->out_file) {
        out_fd = open(pl->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            perror("open (>)");
            if (in_fd >= 0) close(in_fd);
            return -1;
        }
    }

    pid_t *pids = (pid_t*)calloc(pl->ncmd, sizeof(pid_t));
    if (!pids) {
        perror("calloc pids");
        if (in_fd >= 0) close(in_fd);
        if (out_fd >= 0) close(out_fd);
        return -1;
    }

    int prev_read = -1;
    int pipefd[2] = {-1, -1};

    for (size_t i = 0; i < pl->ncmd; i++) {
        int need_pipe = (i + 1 < pl->ncmd);
        if (need_pipe) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                free(pids);
                if (in_fd >= 0) close(in_fd);
                if (out_fd >= 0) close(out_fd);
                if (prev_read >= 0) close(prev_read);
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(pids);
            if (in_fd >= 0) close(in_fd);
            if (out_fd >= 0) close(out_fd);
            if (prev_read >= 0) close(prev_read);
            if (need_pipe) { close(pipefd[0]); close(pipefd[1]); }
            return -1;
        }

        if (pid == 0) {
            if (i == 0) {
                if (in_fd >= 0) {
                    dup2(in_fd, STDIN_FILENO);
                }
            } else {
                dup2(prev_read, STDIN_FILENO);
            }

            if (i + 1 == pl->ncmd) {
                if (out_fd >= 0) {
                    dup2(out_fd, STDOUT_FILENO);
                }
            } else {
                dup2(pipefd[1], STDOUT_FILENO);
            }

            if (in_fd >= 0) close(in_fd);
            if (out_fd >= 0) close(out_fd);
            if (prev_read >= 0) close(prev_read);
            if (need_pipe) { close(pipefd[0]); close(pipefd[1]); }

            exec_command(pl->cmds[i].argv);
        }

        pids[i] = pid;

        if (prev_read >= 0) close(prev_read);
        if (need_pipe) {
            close(pipefd[1]);
            prev_read = pipefd[0];
        } else {
            prev_read = -1;
        }
    }

    if (in_fd >= 0) close(in_fd);
    if (out_fd >= 0) close(out_fd);
    if (prev_read >= 0) close(prev_read);

    int status = 0;
    int rc = 0;
    for (size_t i = 0; i < pl->ncmd; i++) {
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            rc = -1;
        }
    }

    free(pids);
    return rc;
}

int main(void) {
    char buf[MAX_BUF];

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

        Pipeline pl;
        if (pipeline_parse(buf, &pl) < 0) {
            continue;
        }

        if (run_pipeline(&pl) < 0) {
            pipeline_free(&pl);
            continue;
        }

        pipeline_free(&pl);
    }

    return 0;
}

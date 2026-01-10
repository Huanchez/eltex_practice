#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_NUMS 12

static void make_sem_name(char *out, size_t cap, const char *filename) {
    unsigned long h = 5381;
    for (const unsigned char *p = (const unsigned char *)filename; *p; p++) {
        h = ((h << 5) + h) + *p;
    }
    snprintf(out, cap, "/sem_eltex_%lx", h);
}

static void make_off_name(char *out, size_t cap, const char *filename) {
    snprintf(out, cap, "%s.off", filename);
}

static long read_offset(const char *off_file) {
    FILE *f = fopen(off_file, "r");
    if (!f) return 0;
    long off = 0;
    if (fscanf(f, "%ld", &off) != 1) off = 0;
    fclose(f);
    if (off < 0) off = 0;
    return off;
}

static void write_offset(const char *off_file, long off) {
    FILE *f = fopen(off_file, "w");
    if (!f) return;
    fprintf(f, "%ld\n", off);
    fclose(f);
}

static void make_line(char *out, size_t cap) {
    out[0] = '\0';
    int n = 1 + rand() % MAX_NUMS;

    for (int i = 0; i < n; i++) {
        int x = rand() % 1000;
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d", x);

        if (strlen(out) + strlen(tmp) + 2 >= cap) break;
        strcat(out, tmp);
        if (i + 1 < n) strcat(out, " ");
    }
}

static int parse_minmax(const char *line, int *out_min, int *out_max) {
    int mn = INT_MAX;
    int mx = INT_MIN;
    int found = 0;

    const char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        char *end = NULL;
        long v = strtol(p, &end, 10);
        if (end == p) {
            while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
            continue;
        }

        if ((int)v < mn) mn = (int)v;
        if ((int)v > mx) mx = (int)v;
        found = 1;
        p = end;
    }

    if (!found) return 0;
    *out_min = mn;
    *out_max = mx;
    return 1;
}

static void run_fabricator(const char *filename, sem_t *sem) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    printf("Производитель (родитель) запущен. Файл: %s\n", filename);

    while (1) {
        sem_wait(sem);

        FILE *f = fopen(filename, "a");
        if (f) {
            char line[MAX_LINE];
            make_line(line, sizeof(line));
            fprintf(f, "%s\n", line);
            fclose(f);
            printf("Производитель записал: %s\n", line);
        }

        sem_post(sem);
        sleep(1);
    }
}

static void run_user(const char *filename, sem_t *sem) {
    char off_file[512];
    make_off_name(off_file, sizeof(off_file), filename);

    printf("Потребитель (дочерний) запущен. Файл: %s\n", filename);

    while (1) {
        sem_wait(sem);

        long off = read_offset(off_file);

        FILE *f = fopen(filename, "r");
        if (f) {
            fseek(f, off, SEEK_SET);

            char line[MAX_LINE];
            int processed = 0;

            while (fgets(line, sizeof(line), f)) {
                int mn, mx;
                if (parse_minmax(line, &mn, &mx)) {
                    printf("Потребитель: min=%d max=%d | %s", mn, mx, line);
                } else {
                    printf("Потребитель: строка без чисел | %s", line);
                }
                processed++;
            }

            long new_off = ftell(f);
            fclose(f);

            if (processed > 0) {
                write_offset(off_file, new_off);
            }
        }

        sem_post(sem);
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    FILE *touch = fopen(filename, "a");
    if (!touch) {
        perror("fopen");
        return 1;
    }
    fclose(touch);

    char sem_name[128];
    make_sem_name(sem_name, sizeof(sem_name), filename);

    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        sem_close(sem);
        return 1;
    }

    if (pid == 0) {
        run_user(filename, sem);
        _exit(0);
    } else {
        run_fabricator(filename, sem);
        wait(NULL);
    }

    sem_close(sem);
    return 0;
}

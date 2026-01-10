#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX_LINE 256

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int sem_lock(int semid) {
    struct sembuf op = {0, -1, SEM_UNDO};
    return semop(semid, &op, 1);
}

static int sem_unlock(int semid) {
    struct sembuf op = {0, 1, SEM_UNDO};
    return semop(semid, &op, 1);
}

static int get_sem_for_file(const char *filename) {
    key_t key = ftok(filename, 'S');
    if (key == -1) {
        perror("ftok");
        return -1;
    }

    int semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return -1;
    }

    union semun u;
    u.val = 1;
    semctl(semid, 0, SETVAL, u);

    return semid;
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

        if (v < mn) mn = (int)v;
        if (v > mx) mx = (int)v;
        found = 1;
        p = end;
    }

    if (!found) return 0;
    *out_min = mn;
    *out_max = mx;
    return 1;
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

    int semid = get_sem_for_file(filename);
    if (semid < 0) return 1;

    char off_file[512];
    make_off_name(off_file, sizeof(off_file), filename);

    printf("Потребитель запущен. Файл: %s\n", filename);

    while (1) {
        if (sem_lock(semid) == -1) {
            perror("semop lock");
            continue;
        }

        long off = read_offset(off_file);

        FILE *f = fopen(filename, "r");
        if (!f) {
            perror("fopen");
            sem_unlock(semid);
            sleep(1);
            continue;
        }

        fseek(f, off, SEEK_SET);

        char line[MAX_LINE];
        int processed = 0;

        while (fgets(line, sizeof(line), f)) {
            int mn, mx;
            if (parse_minmax(line, &mn, &mx)) {
                printf("min=%d max=%d | %s", mn, mx, line);
            } else {
                printf("строка без чисел | %s", line);
            }
            processed++;
        }

        long new_off = ftell(f);
        fclose(f);

        if (processed > 0) {
            write_offset(off_file, new_off);
        }

        sem_unlock(semid);

        sleep(1);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX_LINE 256
#define MAX_NUMS 12

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

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    printf("Производитель запущен. Файл: %s\n", filename);

    while (1) {
        if (sem_lock(semid) == -1) {
            perror("semop lock");
            continue;
        }

        FILE *f = fopen(filename, "a");
        if (!f) {
            perror("fopen");
            sem_unlock(semid);
            sleep(1);
            continue;
        }

        char line[MAX_LINE];
        make_line(line, sizeof(line));
        fprintf(f, "%s\n", line);
        fclose(f);

        sem_unlock(semid);

        printf("Производитель записал: %s\n", line);
        sleep(1);
    }

    return 0;
}

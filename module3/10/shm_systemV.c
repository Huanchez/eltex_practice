#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_NUMS 12

#define SEM_NUMS 0
#define SEM_RES  1

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef struct {
    int n;
    int nums[MAX_NUMS];
    int mn;
    int mx;
} Shared;

static volatile sig_atomic_t stop_flag = 0;
static volatile sig_atomic_t processed = 0;

static void on_sigint(int sig) {
    (void)sig;
    stop_flag = 1;
}

static int sem_down(int semid, int idx) {
    struct sembuf op = {0, 0, 0};
    op.sem_num = (unsigned short)idx;
    op.sem_op = -1;
    op.sem_flg = SEM_UNDO;
    return semop(semid, &op, 1);
}

static int sem_up(int semid, int idx) {
    struct sembuf op = {0, 0, 0};
    op.sem_num = (unsigned short)idx;
    op.sem_op = 1;
    op.sem_flg = SEM_UNDO;
    return semop(semid, &op, 1);
}

static void make_set(Shared *sh) {
    sh->n = 1 + rand() % MAX_NUMS;
    for (int i = 0; i < sh->n; i++) {
        sh->nums[i] = rand() % 1000;
    }
}

static void calc_minmax(Shared *sh) {
    int mn = INT_MAX;
    int mx = INT_MIN;
    for (int i = 0; i < sh->n; i++) {
        int v = sh->nums[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    sh->mn = mn;
    sh->mx = mx;
}

static void print_set(const Shared *sh) {
    printf("Набор: ");
    for (int i = 0; i < sh->n; i++) {
        printf("%d", sh->nums[i]);
        if (i + 1 < sh->n) printf(" ");
    }
    printf("\n");
}

int main(void) {
    signal(SIGINT, on_sigint);

    key_t key = ftok(".", 'M');
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    int shmid = shmget(key, sizeof(Shared), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    int semid = semget(key, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    union semun u;
    u.val = 0;
    semctl(semid, SEM_NUMS, SETVAL, u);
    semctl(semid, SEM_RES, SETVAL, u);

    Shared *sh = (Shared *)shmat(shmid, NULL, 0);
    if (sh == (void *)-1) {
        perror("shmat");
        return 1;
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_IGN);

        while (1) {
            if (sem_down(semid, SEM_NUMS) == -1) continue;

            if (sh->n <= 0) {
                sem_up(semid, SEM_RES);
                break;
            }

            calc_minmax(sh);
            sem_up(semid, SEM_RES);
        }

        shmdt(sh);
        return 0;
    }

    printf("Родитель запущен\n");

    while (!stop_flag) {
        make_set(sh);

        print_set(sh);

        sem_up(semid, SEM_NUMS);

        if (sem_down(semid, SEM_RES) == -1) continue;

        printf("min=%d max=%d\n\n", sh->mn, sh->mx);
        processed++;

        sleep(1);
    }

    printf("\nПолучен SIGINT. Обработано наборов: %d\n", (int)processed);

    sh->n = 0;
    sem_up(semid, SEM_NUMS);
    sem_down(semid, SEM_RES);

    wait(NULL);

    shmdt(sh);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}

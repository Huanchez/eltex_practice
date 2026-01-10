#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <limits.h>
#include <string.h>

#define MAX_NUMS 12

#define SHM_NAME "/shm_posix" 
#define SEM_NUMS "/shm_posix_nums" 
#define SEM_RES "/shm_posix_res"

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

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NUMS);
    sem_unlink(SEM_RES);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, (off_t)sizeof(Shared)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    Shared *sh = (Shared *)mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    close(fd);
    memset(sh, 0, sizeof(*sh));

    sem_t *sem_nums = sem_open(SEM_NUMS, O_CREAT, 0666, 0);
    sem_t *sem_res  = sem_open(SEM_RES,  O_CREAT, 0666, 0);
    if (sem_nums == SEM_FAILED || sem_res == SEM_FAILED) {
        perror("sem_open");
        munmap(sh, sizeof(Shared));
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NUMS);
        sem_unlink(SEM_RES);
        return 1;
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        sem_close(sem_nums);
        sem_close(sem_res);
        munmap(sh, sizeof(Shared));
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NUMS);
        sem_unlink(SEM_RES);
        return 1;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_IGN);

        while (1) {
            sem_wait(sem_nums);

            if (sh->n <= 0) {
                sem_post(sem_res);
                break;
            }

            calc_minmax(sh);
            sem_post(sem_res);
        }

        sem_close(sem_nums);
        sem_close(sem_res);
        munmap(sh, sizeof(Shared));
        return 0;
    }

    printf("Родитель запущен\n");

    while (!stop_flag) {
        make_set(sh);
        print_set(sh);

        sem_post(sem_nums);
        sem_wait(sem_res);

        printf("min=%d max=%d\n\n", sh->mn, sh->mx);
        processed++;

        sleep(1);
    }

    printf("\nПолучен SIGINT. Обработано наборов: %d\n", (int)processed);

    sh->n = 0;
    sem_post(sem_nums);
    sem_wait(sem_res);

    wait(NULL);

    sem_close(sem_nums);
    sem_close(sem_res);

    sem_unlink(SEM_NUMS);
    sem_unlink(SEM_RES);

    munmap(sh, sizeof(Shared));
    shm_unlink(SHM_NAME);

    return 0;
}

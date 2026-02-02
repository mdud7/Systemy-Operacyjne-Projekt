#include "common.h"
#include <string.h>

int g_shmid = -1;
int g_semid = -1;
int g_msg_staff = -1;
int g_msg_kasa = -1;

void my_ipc_clean(int sig) {
    kill(0, SIGTERM);
    if (g_shmid != -1)
        shmctl(g_shmid, IPC_RMID, NULL);
    if (g_semid != -1)
        semctl(g_semid, 0, IPC_RMID);
    if (g_msg_staff != -1)
        msgctl(g_msg_staff, IPC_RMID, NULL);
    if (g_msg_kasa != -1)
        msgctl(g_msg_kasa, IPC_RMID, NULL);
    unlink(FIFO_FILE);
    exit(0);
}

void check(int result, const char* msg) {
    if (result == -1) {
        perror(msg);
        exit(1);
    }
}

void log_main(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now); t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [SYSTEM]-> %s\n", t_str, msg);
        fclose(f);
    }
}

void setup_tables(BarSharedMemory *shm) {
    int idx = 0;
    for (int cap = 1; cap <= 4; cap++) {
        for (int k = 0; k < 5; k++) {
            shm->tables[idx].id = idx;
            shm->tables[idx].capacity = cap;
            shm->tables[idx].current_count = 0;
            shm->tables[idx].current_group_size = 0;
            shm->tables[idx].is_reserved = 0;
            idx++;
        }
    }
    shm->table_count = idx;
    log_main("Stoliki ustawione.");
}

int main() {
    struct sigaction sa;
    sa.sa_handler = my_ipc_clean;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    FILE *f = fopen(REPORT_FILE, "w");
    if(f) {
        fprintf(f, "START SYMULACJI\n");
        fclose(f);
    }

    printf("Start symulacji. Log: %s\n", REPORT_FILE);
    
    if (mkfifo(FIFO_FILE, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    key_t k_shm = get_key(PROJ_ID_SHM);
    key_t k_sem = get_key(PROJ_ID_SEM);
    key_t k_msg = get_key(PROJ_ID_MSG);
    key_t k_kasa = get_key(PROJ_ID_KASA);

    g_shmid = shmget(k_shm, sizeof(BarSharedMemory), IPC_CREAT | 0600);
    g_semid = semget(k_sem, SEM_COUNT, IPC_CREAT | 0600);
    
    semctl(g_semid, SEM_ACCESS, SETVAL, 1);
    semctl(g_semid, SEM_QUEUE_LIMITER, SETVAL, 5000);
    semctl(g_semid, SEM_BARRIER, SETVAL, 0);

    g_msg_staff = msgget(k_msg, IPC_CREAT | 0600);
    g_msg_kasa = msgget(k_kasa, IPC_CREAT | 0600);

    BarSharedMemory *shm = (BarSharedMemory*)shmat(g_shmid, NULL, 0);
    memset(shm, 0, sizeof(BarSharedMemory));
    setup_tables(shm);

    pid_t pid = fork();
    if (pid == 0) {
        execl("./cashier", "cashier", NULL);
        exit(1);
    }
    shm->cashier_pid = pid;

    pid = fork();
    if (pid == 0) {
        execl("./staff", "staff", NULL);
        exit(1);
    }
    shm->staff_pid = pid;

    pid = fork();
    if (pid == 0) {
        execl("./menager", "menager", NULL);
        exit(1);
    }
    shm->menager_pid = pid;

    MenagerOrderMsg sync_msg;
    msgrcv(g_msg_staff, &sync_msg, sizeof(MenagerOrderMsg) - sizeof(long), 999, 0);
    msgsnd(g_msg_staff, &sync_msg, sizeof(MenagerOrderMsg) - sizeof(long), 0);

    pid = fork();
    if (pid == 0) {
        execl("./generator", "generator", NULL);
        exit(1);
    }

    shmdt(shm);
    while(wait(NULL) > 0);
    my_ipc_clean(0);
    return 0;
}
#include "common.h"
#include <string.h>

void check( int result, const char* msg) {
    if (result == -1) { perror(msg); exit(1); }
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
    printf("[main] ustawiono %d stolikow\n", idx);
}

void log_main(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [SYSTEM]   -> %s\n", t_str, msg);
        fclose(f);
    }
    printf("[MAIN] %s\n", msg);
}

int main() {
    FILE *f = fopen(REPORT_FILE, "w");
    if(f) { fprintf(f, "START SYMULACJI BARU MLECZNEGO\n"); fclose(f); }

    log_main("Inicjalizacja zasobow");

    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), IPC_CREAT | 0600);
    check(shmid, "main shmget");

    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    check(semid, "main semget");
    semctl(semid, 0, SETVAL, 1);

    int msgid_staff = msgget(MSG_KEY, IPC_CREAT | 0600); 
    check(msgid_staff, "main msgget staff");

    int msgid_kasa = msgget(KASA_KEY, IPC_CREAT | 0600);
    check(msgid_kasa, "main msgget kasa");

    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    memset(shm, 0, sizeof(BarSharedMemory));

    setup_tables(shm);

    log_main("uruchamianie procesow symulacji");

    pid_t staff_pid = fork();
    if (staff_pid == 0) { 
        execl("./staff", "staff", NULL); 
        exit(1); 
    }
    shm->staff_pid = staff_pid;

    if (fork() == 0) { execl("./cashier", "cashier", NULL); exit(1); } //kasjer

    if (fork() == 0) { sleep(1); execl("./generator", "generator", NULL); exit(1); } //generator

    if (fork() == 0) { sleep(1); execl("./menager", "menager", NULL); exit(1); } //menager

    shmdt(shm);

    while(wait(NULL) > 0);

    log_main("wszystkie procesy zakonczone, sprzatanie...");
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(msgid_staff, IPC_RMID, NULL);
    msgctl(msgid_kasa, IPC_RMID, NULL);
    
    log_main("koniec symulacji");
    
    return 0;
}
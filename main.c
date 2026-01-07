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

int main() {
    printf("[main] start sys\n");

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

    FILE *f = fopen(REPORT_FILE, "w");
    if(f) { fprintf(f, "START SYMULACJI\n"); fclose(f); }

    shmdt(shm);
    
    if (fork() == 0) { execl("./staff", "staff", NULL); exit(1); } //pracownik

    if (fork() == 0) { execl("./cashier", "cashier", NULL); exit(1); } //kasjer

    if (fork() == 0) { sleep(1); execl("./generator", "generator", NULL); exit(1); } //generator

    if (fork() == 0) { sleep(1); execl("./manager", "manager", NULL); exit(1); } //menager

    while(wait(NULL) > 0);

    printf("[main] sprzatanie zasobow\n");
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(msgid_staff, IPC_RMID, NULL);
    msgctl(msgid_kasa, IPC_RMID, NULL);
    
    return 0;
}
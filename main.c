#include "common.h"
#include <string.h>

/**
 * main.c
 * Główny proces zarządzający symulacją.
 * - Inicjalizuje zasoby (pamięć współdzieloną, semafory, kolejki komunikatów).
 * - Tworzy procesy potomne (pracowników, kasę i generator klientów).
 * - Odpowiada za poprawne usuwanie zasobów po zakończeniu.
 */

int g_shmid = -1;
int g_semid = -1;
int g_msg_staff = -1;
int g_msg_kasa = -1;

void my_ipc_clean(int sig) {
    if (g_shmid != -1) shmctl(g_shmid, IPC_RMID, NULL);
    
    if (g_semid != -1) semctl(g_semid, 0, IPC_RMID);
    
    if (g_msg_staff != -1) msgctl(g_msg_staff, IPC_RMID, NULL);
    if (g_msg_kasa != -1) msgctl(g_msg_kasa, IPC_RMID, NULL);
    
    kill(0, SIGTERM); 

    if (sig != 0) {
        exit(0);
    }
}

void check( int result, const char* msg) {
    if (result == -1) { perror(msg); exit(1); }
}

void log_main(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
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
    char buf[50];
    sprintf(buf, "Ustawiono %d stolikow", idx);
    log_main(buf);
}

int main() {

    signal(SIGINT, my_ipc_clean);
    signal(SIGTERM, my_ipc_clean);

    FILE *f = fopen(REPORT_FILE, "w");
    if(f) { fprintf(f, "START SYMULACJI BARU MLECZNEGO\n"); fclose(f); }

    printf("Start symulacji. Plik z logami 'raport_bar.txt' \n");
    log_main("Inicjalizacja zasobow");

    g_shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), IPC_CREAT | 0600);
    check(g_shmid, "main shmget");

    g_semid = semget(SEM_KEY, 2, IPC_CREAT | 0600);
    check(g_semid, "main semget");
    
    semctl(g_semid, SEM_ACCESS, SETVAL, 1);
    semctl(g_semid, SEM_QUEUE_LIMITER, SETVAL, 20);

    g_msg_staff = msgget(MSG_KEY, IPC_CREAT | 0600); 
    check(g_msg_staff, "main msgget staff");

    g_msg_kasa = msgget(KASA_KEY, IPC_CREAT | 0600);
    check(g_msg_kasa, "main msgget kasa");

    BarSharedMemory *shm = (BarSharedMemory*)shmat(g_shmid, NULL, 0);

    memset(shm, 0, sizeof(BarSharedMemory));

    setup_tables(shm);

    log_main("uruchamianie procesow symulacji");

    pid_t pid;
    
    pid = fork();
    if (pid == -1) { perror("Blad fork (cashier)"); exit(1); }
    if (pid == 0) { 
        execl("./cashier", "cashier", NULL); 
        perror("Blad execl (cashier)"); exit(1); 
    }
    shm->cashier_pid = pid;

    pid = fork();
    if (pid == -1) { perror("Blad fork (staff)"); exit(1); }
    if (pid == 0) { 
        execl("./staff", "staff", NULL); 
        perror("Blad execl (staff)"); exit(1);
    }
    shm->staff_pid = pid;

    pid = fork();
    if (pid == -1) { perror("Blad fork (menager)"); exit(1); }
    if (pid == 0) { 
        execl("./menager", "menager", NULL); 
        perror("Blad execl (menager)"); exit(1); 
    }
    shm->menager_pid = pid;
    
    MenagerOrderMsg sync_msg;
    msgrcv(g_msg_staff, &sync_msg, sizeof(MenagerOrderMsg) - sizeof(long), 999, 0);
    msgsnd(g_msg_staff, &sync_msg, sizeof(MenagerOrderMsg) - sizeof(long), 0);

    pid = fork();
    if (pid == -1) { perror("Blad fork (generator)"); exit(1); }
    if (pid == 0) { 
        execl("./generator", "generator", NULL); 
        perror("Blad execl (generator)"); exit(1); 
    }

    shmdt(shm);

    while(wait(NULL) > 0);

    log_main("wszystkie procesy zakonczone, sprzatanie...");
    my_ipc_clean(0);
    
    log_main("koniec symulacji");
    printf("Koniec symulacji\n");
    
    return 0;
}
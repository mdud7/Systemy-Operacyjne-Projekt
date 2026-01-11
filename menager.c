#include "common.h"
#include <string.h>

void check(int result, const char *msg) {
    if (result == -1) { perror(msg); exit(1); }
}

void log_menager(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [KIEROWNIK]-> %s\n", t_str, msg);
        fclose(f);
    }
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    check(shmid, "menager shmget");
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, 0600);

    printf("[MANAGER] Czekam na pracownika.\n");
    while (shm->staff_pid == 0) sleep(1);

    log_menager("Panel kierownika aktywny.");

    int running = 1;
    while(running) {
        printf("\nMENU KIEROWNIKA\n");
        printf("1 Potrój liczbę stolików 3-os (Sygnał 1)\n");
        printf("2 Rezerwacja (Wiadomość + Sygnał 2)\n");
        printf("3 POŻAR (Ewakuacja)\n");
        printf("0 Wyjście\n> ");
    
        int choice;
        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue ;}
    
        if (shm->fire_alarm) break;

        switch(choice) {
            case 1:
                log_menager("wydano sygnal potrojenia");
                kill(shm->staff_pid, SIG_TRIPLE_X3);
                printf(" wyslano potrjoenie\n");
                break;
            case 2:
                printf("Ile stolikow: ");
                int n; 
                scanf("%d", &n);
                
                if (n > 0) {
                    char buf[100];
                    sprintf(buf, "wydano polecenie rezerwacji %d stolikow", n);
                    log_menager(buf);

                    MenagerOrderMsg msg;
                    msg.mtype = 1;
                    msg.count = n;
                    msgsnd(msgid, &msg, sizeof(MenagerOrderMsg), 0);
                    kill(shm->staff_pid, SIG_RESERVE);
                    printf("Wyslano rezerwacje na %d stolikow\n", n);
                }
                break;
            case 3:
                log_menager("oglaszam pozar, ewakuacja");
                shm->fire_alarm = 1;
                kill(shm->staff_pid, SIG_FIRE);
                printf("pozar ogloszony");
                running = 0;
                break;
            case 0:
                log_menager("zamykanie baru, normalne wyjscie");
                shm->stop_simulation = 1;
                running = 0;
                break;
        }
    }

    shmdt(shm);
    return 0;
}
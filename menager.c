#include "common.h"

void check(int result, const char *msg) {
    if (result == -1) { perror(msg); exit(1); }
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    check(shmid, "menager shmget");
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, 0600);

    printf("[menager] czekam na PID pracownika... \n");
    while (shm->staff_pid == 0) sleep(1);

    int running = 1;
    while(running) {
        printf("\nMENU KIEROWNIKA\n");
        printf("1 Potroj liczbę stolikow 3-os (Sygnał 1)\n");
        printf("2 Rezerwacja (wiadomosc + sygnał 2)\n");
        printf("3 POZAR (Ewakuacja)\n");
        printf("0 Wyjscie\n> ");
    
        int choice;
        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue ;}
    
        if (shm->fire_alarm) break;

        switch(choice) {
            case 1:
                kill(shm->staff_pid, SIG_TRIPLE_X3);
                printf(" wyslano potrjoenie\n");
                break;
            case 2:
                printf("Ile stolikow: ");
                int n; scanf("%d", &n);
                if (n > 0) {
                    MenagerOrderMsg msg;
                    msg.mtype = 1;
                    msg.count = n;
                    msgsnd(msgid, &msg, sizeof(MenagerOrderMsg), 0);
                    kill(shm->staff_pid, SIG_RESERVE);
                    printf("Wyslano rezerwacje na %d stolikow\n", n);
                }
                break;
            case 3:
                shm->fire_alarm = 1;
                kill(shm->staff_pid, SIG_FIRE);
                printf("pozar ogloszony");
                running = 0;
                break;
            case 0:
                shm->stop_simulation = 1;
                running = 0;
                break;
        }
    }

    shmdt(shm);
    return 0;
}
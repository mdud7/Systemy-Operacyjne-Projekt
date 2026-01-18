#include "common.h"
#include <string.h>

/**
 * menager.c
 * Konsola sterująca dla Kierownika.
 *
 * Zadanie:
 * - Interaktywne menu dla użytkownika.
 * - Wysyłanie sygnałów sterujących do procesu Staff (SIGUSR1, SIGUSR2, SIGRTMIN).
 * - Zlecanie zadań przez kolejkę komunikatów (np. rezerwacje).
 * - Awaryjne zatrzymanie systemu (Pożar).
 */

void check(int result, const char *msg) {
    if (result == -1) { perror(msg); exit(1); }
}

void log_menager(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [MENAGER]-> %s\n", t_str, msg);
        fclose(f);
    }
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    check(shmid, "menager shmget");
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, 0600);

    int msgid_kasa = msgget(KASA_KEY, 0600);

    printf("[MANAGER] Czekam na pracownika.\n");
    while (shm->staff_pid == 0) sleep(1);

    log_menager("Panel kierownika aktywny.");

    int running = 1;
    while(running) {
        printf("\nMENU KIEROWNIKA\n");
        printf("1 Podwoj liczbę stolików 3-os (Sygnał 1)\n");
        printf("2 Rezerwacja (Wiadomość + Sygnał 2)\n");
        printf("3 POZAR (Ewakuacja)\n");
        printf("0 Wyjście\n> \n");
        printf("Wybrano: ");
    
        int choice;
        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue ;}
    
        if (shm->fire_alarm) break;

        switch(choice) {
            case 1:
                log_menager("wydano sygnal podwojenia");
                kill(shm->staff_pid, SIG_DOUBLE_X3);
                printf(" wyslano sygnal podwojenia\n");
                break;
            case 2:
                printf("Ile miejsc: ");
                int n; 
                scanf("%d", &n);
                
                if (n > 0) {
                    char buf[100];
                    sprintf(buf, "wydano polecenie rezerwacji %d miejsc", n);
                    log_menager(buf);

                    MenagerOrderMsg msg;
                    msg.mtype = 1;
                    msg.count = n;
                    msgsnd(msgid, &msg, sizeof(MenagerOrderMsg) - sizeof(long), 0);
                    kill(shm->staff_pid, SIG_RESERVE);
                    printf("Wyslano rezerwacje na %d miejsc\n", n);
                }
                break;
            case 3:
                log_menager("oglaszam pozar, ewakuacja");
                shm->fire_alarm = 1;
                kill(shm->staff_pid, SIG_FIRE);

                PaymentMsg kill_msg;
                kill_msg.mtype = MSG_END_WORK;
                msgsnd(msgid_kasa, &kill_msg, sizeof(PaymentMsg) - sizeof(long), 0);
                printf("[MENAGER] pozar ogloszony. Ewakuacja");
                sleep(3);
                log_menager("zamykam lokal");
                running = 0;
                break;
            case 0:
                log_menager("zamykanie baru, normalne wyjscie");
                shm->stop_simulation = 1;

                PaymentMsg end_msg;
                end_msg.mtype = MSG_END_WORK; 
                msgsnd(msgid_kasa, &end_msg, sizeof(PaymentMsg) - sizeof(long), 0);

                printf("[MENAGER] zamykanie systemu");
                running = 0;
                break;
        }
    }

    shmdt(shm);
    return 0;
}
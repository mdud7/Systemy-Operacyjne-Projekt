#include "common.h"
#include <string.h>

/**
 * menager.c
 * Panel administratora do interaktywnego zarządzania barem.
 * - Umożliwia użytkownikowi zmianę parametrów symulacji w czasie rzeczywistym.
 * - Przesyła polecenia systemowe do pozostałych procesów.
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

    MenagerOrderMsg ready;
    msgrcv(msgid, &ready, sizeof(MenagerOrderMsg) - sizeof(long), 999, 0);

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
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            printf("[BLAD] To nie jest liczba!\n");
            continue;
        }

        if (choice < 0 || choice > 3) {
            printf("[BLAD] Nie ma takiej opcji w menu (wybierz 0-3).\n");
            continue;
        }
    
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

                if (scanf("%d", &n) != 1) {
                    while(getchar() != '\n');
                    printf("[BLAD] Podaj poprawną liczbę\n");
                    break;
                }

                if (n <= 0 || n > MAX_TABLES) {
                    printf("[BLAD] Nieprawidlowa liczba miejsc %d\n", MAX_TABLES);
                } 
                else {
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

                kill(shm->staff_pid, SIGTERM);

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
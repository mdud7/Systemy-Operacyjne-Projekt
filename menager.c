#include "common.h"
#include <string.h>
#include <fcntl.h>

void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    key_t k_shm = get_key(PROJ_ID_SHM);
    key_t k_msg = get_key(PROJ_ID_MSG);
    key_t k_kasa = get_key(PROJ_ID_KASA);

    int shmid = shmget(k_shm, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int msgid = msgget(k_msg, 0600);
    int msgid_kasa = msgget(k_kasa, 0600);

    MenagerOrderMsg ready;
    msgrcv(msgid, &ready, sizeof(MenagerOrderMsg) - sizeof(long), MSG_READY_SYNC, 0);
    
    write_log("MENAGER", "System gotow.");

    int running = 1;
    while(running) {

        printf("\n[STATUS] Aktualna liczba stolików w barze: %d\n", shm->table_count);

        printf("\n===PANEL KIEROWNIKA===\n");
        printf("1. Podwoj stoliki (X3)\n");
        printf("2. Rezerwacja\n");
        printf("3. POZAR (Ewakuacja)\n");
        printf("4. Wydane dania(FIFO)\n");
        printf("5. WYGENERUJ NOWYCH KLIENTÓW (5000 Klientow)\n");
        printf("0. Wyjscie\n ");
        
        int choice;
        int result = scanf("%d", &choice);
        clear_stdin();

        if (result != 1) {
            printf("[BLAD] To nie jest liczba\n");
            continue;
        }

        if (shm->fire_alarm && choice != 0) {
            printf("[ALARM] Trwa pozar.\n");
            continue;
        }

        switch(choice) {
            case 1:
                kill(shm->staff_pid, SIG_DOUBLE_X3);
                printf("[INFO] Wyslano polecenie podwojenia\n");
                write_log("MENAGER", "wyslano polecenie podwojenia X3.");
                break;
            case 2: {
                int safe_limit = shm->table_count - 1;
                printf("Podaj liczbe stolikow do rezerwacji od 1 do %d: ", safe_limit);
                int n;
                if (scanf("%d", &n) == 1) {
                    if (n > 0 && n < shm->table_count) {
                        MenagerOrderMsg msg;
                        msg.mtype=1;
                        msg.count=n;
                        msgsnd(msgid, &msg, sizeof(MenagerOrderMsg)-sizeof(long), 0);
                        kill(shm->staff_pid, SIG_RESERVE);
                        write_log("MENAGER", "Polecenie Rezerwacji");
                    } else 
                        printf("[BLAD] Zly zakres\n");
                } else {
                    printf("[BLAD] Zla wartosc\n");
                }
                clear_stdin();
                break;
            }
            case 3:
                shm->fire_alarm = 1;
                write_log("MENGAER", "OGLOSZONO POZAR");
                kill(shm->staff_pid, SIG_FIRE);
                PaymentMsg km;
                km.mtype=MSG_END_WORK;
                msgsnd(msgid_kasa, &km, sizeof(PaymentMsg)-sizeof(long), 0);
                printf("[ALARM] OGLOSZONO POZAR!\n");
                running = 0;
                kill(getppid(), SIGINT);
                break;
            case 4: {
                int fd = open(FIFO_FILE, O_RDONLY | O_NONBLOCK);
                if (fd != -1) {
                    char buf[4096];
                    long total = 0;
                    ssize_t bytes;
                    while ((bytes = read(fd, buf, sizeof(buf))) > 0) 
                        total += bytes;
                    printf("Kuchnia wydala: %ld posilkow\n", total);
                    close(fd);
                } else {
                    printf("[INFO] Kuchnia nie odpowiada.\n");
                }
                break;
            }
            case 5:
                if (shm->generator_pid > 0) {
                    printf("Wysylam sygnal nowej fali do Generatora...\n");
                    kill(shm->generator_pid, SIG_NEW_WAVE);
                    write_log("MENAGER", "Nowa fala klientow.");
                } else {
                    printf("Blad: Nie znam PID generatora.\n");
                }
                break;
            case 0:
                shm->stop_simulation = 1;
                write_log("MENAGER", "Zamykanie systemu...");
                kill(getppid(), SIGTERM);
                running = 0;
                break;
            default:
                printf("[BLAD] Nieznana opcja.\n");
        }
    }
    shmdt(shm);
    return 0;
}
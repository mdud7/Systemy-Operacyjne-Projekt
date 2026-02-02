#include "common.h"
#include <string.h>
#include <fcntl.h>

void log_menager(const char* msg) {
    int fd = open(REPORT_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd != -1) {
        char buf[256];
        time_t now = time(NULL);
        char *t_str = ctime(&now); t_str[strlen(t_str)-1] = '\0';
        int len = sprintf(buf, "[%s] [MENAGER]-> %s\n", t_str, msg);
        write(fd, buf, len);
        close(fd);
    }
}

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
    msgrcv(msgid, &ready, sizeof(MenagerOrderMsg) - sizeof(long), 999, 0);
    log_menager("System gotowy.");

    int running = 1;
    while(running) {
        printf("\n===PANEL KIEROWNIKA===\n");
        printf("1. Podwoj stoliki (X3)\n");
        printf("2. Rezerwacja\n");
        printf("3. POZAR (Ewakuacja)\n");
        printf("4. Wydane posilki(FIFO)\n");
        printf("5. WYGENERUJ NOWYCH KLIENTÓW (5000 Klientow)\n");
        printf("0. Wyjscie\n> ");
        
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
                log_menager("Polecenie podwojenia X3.");
                break;
            case 2: {
                printf("Podaj liczbe stolikow do rezerwacji: ");
                int n;
                if (scanf("%d", &n) == 1) {
                    if (n > 0 && n <= MAX_TABLES) {
                        MenagerOrderMsg msg;
                        msg.mtype=1;
                        msg.count=n;
                        msgsnd(msgid, &msg, sizeof(MenagerOrderMsg)-sizeof(long), 0);
                        kill(shm->staff_pid, SIG_RESERVE);
                        log_menager("Polecenie Rezerwacji");
                    } else printf("[BLAD] Zly zakres\n");
                } else {
                    printf("[BLAD] Zla wartosc\n");
                }
                clear_stdin();
                break;
            }
            case 3:
                shm->fire_alarm = 1;
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
                    log_menager("Nowa fala 5000 klientow.");
                } else {
                    printf("Blad: Nie znam PID generatora.\n");
                }
                break;
            case 0:
                shm->stop_simulation = 1;
                log_menager("Zamykanie..");
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
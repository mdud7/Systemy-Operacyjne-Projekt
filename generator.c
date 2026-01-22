#include "common.h"
#include <time.h>
#include <string.h>

/**
 * generator.c
 * Proces generujący ruch w barze.
 * - Symuluje napływ klientów o zróżnicowanych rozmiarach grup.
 * - Kontroluje natężenie ruchu w barze poprzez losowe odstępy czasu.
 */

void check(int result, const char *msg) {
    if (result == -1) { perror(msg); exit (1); }
}

void log_gen(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [GENERATOR]-> %s\n", t_str, msg);
        fclose(f);
    }
}

int main() {

    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    check(shmid, "generator shmget");
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    srand(time(NULL));

    log_gen("Rozpoczynam wpuszczanie klientow.");

    while(1) {
            if (shm->fire_alarm) {
                log_gen("pożar, nie wpuszczam ludzi\n");
                break;
            }
            if (shm->stop_simulation) {
                log_gen(" koniec symulacji \n");
                break;
            }

            pid_t pid = fork();

            if (pid == 0) {
                int size = 1 + (rand() % 3);

                char size_str[4];
                sprintf(size_str, "%d", size);

                execl("./client", "client", size_str, NULL);

                perror("Blad execl w generatorze");
                exit (1);
            } 
            else if (pid > 0) {
                while (waitpid(-1, NULL, WNOHANG) > 0);
                usleep(1000000 + (rand() % 1000000));
            }
            else {
                perror("blad fork");
            }
    }
    log_gen("Koniec generatora");
    shmdt(shm);
    return 0;
}
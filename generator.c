#include "common.h"
#include <time.h>

void check(int result, const char *msg) {
    if (result == -1) { perror(msg); exit (1); }
}

int main() {

    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    check(shmid, "generator shmget");
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    srand(time(NULL));

    printf("[generator] rozpoczynam wpuszczanie kloientów...\n");

    while(1) {
            if (shm->fire_alarm) {
                printf("[generator] pozar, nie wpuszczam ludzi\n");
                break;
            }
            if (shm->stop_simulation) {
                printf("[generator] koniec symulacji \n");
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
                usleep(50000 + (rand() % 1000000));
            }
            else {
                perror("blad fork");
            }
    }
    
    shmdt(shm);
    return 0;
}
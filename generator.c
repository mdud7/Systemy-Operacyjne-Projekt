#include "common.h"
#include <errno.h>

volatile sig_atomic_t flag_exit = 0;
volatile sig_atomic_t flag_new_wave = 1;

void handler(int sig) {
    if (sig == SIGTERM)
        flag_exit = 1;
    if (sig == SIG_NEW_WAVE)
        flag_new_wave = 1;
}

int main() {
    key_t k_shm = get_key(PROJ_ID_SHM);
    int shmid = shmget(k_shm, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    shm->generator_pid = getpid();

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIG_NEW_WAVE, &sa, NULL);

    srand(time(NULL));

    unsigned long long total_created = 0;

    while(!flag_exit && !shm->stop_simulation) {
        
        if (flag_new_wave) {
            printf("[GENERATOR] generowanie %d procesow\n", WAVE_SIZE);
            
            for (int i = 0; i < WAVE_SIZE; i++) {
                if (flag_exit || shm->stop_simulation)
                    break;

                int size = 1 + (rand() % 3);

                pid_t pid = fork();

                if (pid == 0) {
                    char size_str[2];
                    size_str[0] = size + '0'; size_str[1] = '\0';
                    execl("./client", "client", size_str, NULL);
                    _exit(1);
                } else if (pid > 0) {
                    total_created++;
                } else {
                    if (errno != EAGAIN) perror("fork");
                }
                waitpid(-1, NULL, WNOHANG);
            }
            
            flag_new_wave = 0;
            printf("[GENERATOR] generowanie zakonczone, lacznie: %llu.\n", total_created);
        }

        pid_t wpid = waitpid(-1, NULL, 0);

        if (wpid > 0) {
            continue;
        }

        if (wpid == -1) {
            if (errno == ECHILD) {
                pause();
            }
            else if (errno == EINTR) {
                continue;
            }
            else {
                perror("waitpid");
                break;
            }
        }
    }

    printf("[GENERATOR] stop.\n");
    while(wait(NULL) > 0);
    shmdt(shm);
    return 0;
}
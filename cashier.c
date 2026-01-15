#include "common.h"
#include <string.h>

void log_cashier(const char* msg) {
    FILE* f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char *t_str = ctime(&now);
        t_str[strlen(t_str)-1] = '\0';
        fprintf(f, "[%s] [KASJER]   -> %s\n", t_str, msg);
        fclose(f);
    }
}

int main() {
    int msgid = msgget(KASA_KEY, 0600);
    if (msgid == -1) { perror("cashier msgget"); exit(1); }
    
    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    log_cashier("Kasa otwarta, czekam na klientow");
    printf("[cashier] kasa otwarta");

    while (!shm->stop_simulation && !shm->fire_alarm) {
        PaymentMsg msg;

        if (msgrcv(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), 0, 0) != -1) {
            
            if (msg.mtype == MSG_END_WORK) { 
                printf("[KASJER] Otrzymano sygnał końca pracy. Zamykam kasę.\n");
                log_cashier("Otrzymano rozkaz zamkniecia kasy.");
                break;
            }

            usleep(50000);

            char buf[128];
            sprintf(buf, "Przyjeto wplate od klienta PID %d (Grupa %d os.), wydano paragon.", msg.client_pid, msg.group_size);
            log_cashier(buf);

            usleep(20000);

            msg.mtype = msg.client_pid;
            msgsnd(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), 0);
        }

        log_cashier("[cashier] kasa zamkneita");
        
    }
    shmdt(shm);
    return 0;
}
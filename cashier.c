#include "common.h"

int main() {
    int msgid = msgget(KASA_KEY, 0600);
    if (msgid == -1) { perror("cashier msgget"); exit(1); }
    
    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    printf("[cashier] kasa otwarta");

    while (!shm->stop_simulation && !shm->fire_alarm) {
        PaymentMsg msg;

        if (msgrcv(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), 1, IPC_NOWAIT) != -1) {
            usleep(50000);

            msg.mtype = msg.client_pid;

            msgsnd(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), 0);
        } else {
            usleep(10000);
        }
    }

    prointf("[cashier] kasa zamkneita");
    shmdt(shm);
    return 0;
}
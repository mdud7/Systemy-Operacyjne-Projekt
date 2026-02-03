#include "common.h"

int main() {
    key_t k_shm = get_key(PROJ_ID_SHM);
    key_t k_kasa = get_key(PROJ_ID_KASA);

    int msgid = msgget(k_kasa, 0600);
    int shmid = shmget(k_shm, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);

    PaymentMsg msg;
    while (!shm->stop_simulation && !shm->fire_alarm) {
        
        if (msgrcv(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), MSG_TYPE_PAYMENT, 0) == -1) {
            break;
        }

        msg.mtype = msg.client_pid;
        msgsnd(msgid, &msg, sizeof(PaymentMsg) - sizeof(long), 0);
    }

    shmdt(shm);
    return 0;
}
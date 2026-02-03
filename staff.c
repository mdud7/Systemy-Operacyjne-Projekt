#include "common.h"
#include <string.h>
#include <fcntl.h>

volatile sig_atomic_t flag_triple = 0;
volatile sig_atomic_t flag_reserve = 0;
volatile sig_atomic_t flag_end = 0;

void signal_handler(int sig) {
    if (sig == SIG_DOUBLE_X3)
        flag_triple = 1;
    if (sig == SIG_RESERVE)
        flag_reserve = 1;
    if (sig == SIGTERM)
        flag_end = 1;
}

int main() {
    key_t k_shm = get_key(PROJ_ID_SHM);
    key_t k_sem = get_key(PROJ_ID_SEM);
    key_t k_msg = get_key(PROJ_ID_MSG);
    key_t k_kasa = get_key(PROJ_ID_KASA);

    int shmid = shmget(k_shm, sizeof(BarSharedMemory), 0600);
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int semid = semget(k_sem, SEM_COUNT, 0600);
    int msgid = msgget(k_msg, 0600);
    int msgid_kasa = msgget(k_kasa, 0600);

    int fifo_fd = open(FIFO_FILE, O_RDWR | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("STAFF: Nie udalo sie otworzyc FIFO");
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIG_DOUBLE_X3, &sa, NULL);
    sigaction(SIG_RESERVE, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    lock_tables(semid);
    shm->staff_pid = getpid();
    unlock_tables(semid);

    MenagerOrderMsg ready;
    ready.mtype = MSG_READY_SYNC;
    msgsnd(msgid, &ready, sizeof(MenagerOrderMsg) - sizeof(long), 0);

    PaymentMsg food_req;
    MenagerOrderMsg msg;
    char byte = '1';

    while (1) {
        if (flag_end || shm->fire_alarm || shm->stop_simulation)
            break;

        if (flag_triple) {
            int added_seats = 0;
            
            lock_tables(semid);
            if (!shm->x3_tripled) {
                int orig = shm->table_count;
                int added_tables = 0;
                for(int i=0; i<orig; i++) {
                    if (shm->tables[i].capacity == 3)
                        added_tables++;
                }
                for(int i=0; i<added_tables*2 && shm->table_count < MAX_TABLES; i++) {
                    int idx = shm->table_count;
                    shm->tables[idx].id = idx;
                    shm->tables[idx].capacity = 3;
                    shm->tables[idx].current_count = 0;
                    shm->tables[idx].is_reserved = 0;
                    shm->table_count++;
                }
                shm->x3_tripled = 1;
                write_log("STAFF", "Podwojono stoliki.");
                added_seats = added_tables * 2;
            }
            unlock_tables(semid);

            if (added_seats > 0) {
                PaymentMsg wake;
                wake.mtype = MSG_TYPE_FREE_TABLE;
                for(int i=0; i<added_seats; i++) {
                    msgsnd(msgid_kasa, &wake, sizeof(PaymentMsg)-sizeof(long), IPC_NOWAIT);
                }
            }
            flag_triple = 0;
        }

        if (flag_reserve)
            flag_reserve = 0;

        while(msgrcv(msgid, &food_req, sizeof(PaymentMsg)-sizeof(long), MSG_TYPE_ORDER_FOOD, IPC_NOWAIT) > 0) {
            food_req.mtype = food_req.client_pid;
            msgsnd(msgid, &food_req, sizeof(PaymentMsg)-sizeof(long), 0);
            if (fifo_fd != -1) write(fifo_fd, &byte, 1);
        }

        ssize_t res = msgrcv(msgid, &msg, sizeof(MenagerOrderMsg)-sizeof(long), 1, IPC_NOWAIT);
        if (res > 0) {
            lock_tables(semid);
            int reserved_cnt = 0;
            for (int i = 0; i < shm->table_count && reserved_cnt < msg.count; i++) {
                if (shm->tables[i].current_count == 0 && !shm->tables[i].is_reserved) {
                    shm->tables[i].is_reserved = 1;
                    reserved_cnt++;
                }
            }
            unlock_tables(semid);
            char buf[64];
            sprintf(buf, "Zarezerwowano %d.", reserved_cnt);
            write_log("STAFF", buf);
        }
    }
    
    if (fifo_fd != -1)
        close(fifo_fd);
    shmdt(shm);
    return 0;
}
#include "common.h"

void sem_lock(int semid) {
    struct sembuf s = {0, -1, 0};
    if (semop(semid, &s, 1) == -1 && errno != EINTR) perror("sem");
}
void sem_unlock(int semid) {
    struct sembuf s = {0, 1, 0};
    semop(semid, &s, 1);
}

int main(int argc, char *argv[]) {
    int group = (argc > 1) ? atoi(argv[1]) : 1;
    int pid = getpid();
    srand(pid);

    int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
    if(shmid == -1) return 0;
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int semid = semget(SEM_KEY, 1, 0600);
    int msgid = msgget(KASA_KEY, 0600);

    if(shm->fire_alarm || shm->stop_simulation) { shmdt(shm); return 0; }
    if((rand()%100) < 5) { shmdt(shm); return 0; }

    int idx = -1;
    for(int k=0; k<5; k++) {
        if(shm->fire_alarm) break;
        sem_lock(semid);
        for(int i=0; i<shm->table_count; i++) {
            Table *t = &shm->tables[i];
            if(t->is_reserved) continue;
            
            if(t->current_count == 0 && t->capacity >= group) {
                t->current_count = group;
                t->current_group_size = group;
                idx = i; break;
            }

            if(t->current_group_size == group && (t->capacity - t->current_count >= group)) {
                t->current_count += group;
                idx = i; break;
            }
        }
        sem_unlock(semid);
        if(idx != -1) break;
        usleep(200000);
    }

    if(idx == -1) { shmdt(shm); return 0; }

    
    PaymentMsg msg = {1, pid, group};
    if(msgsnd(msgid, &msg, sizeof(PaymentMsg)-sizeof(long), 0) == -1) {
        sem_lock(semid); shm->tables[idx].current_count -= group; sem_unlock(semid);
        shmdt(shm); return 0;
    }

    msgrcv(msgid, &msg, sizeof(PaymentMsg)-sizeof(long), pid, 0);

    if(!shm->fire_alarm) sleep(1 + rand()%3);

    sem_lock(semid);
    shm->tables[idx].current_count -= group;
    if(shm->tables[idx].current_count == 0) shm->tables[idx].current_group_size = 0;
    sem_unlock(semid);

    if(!shm->fire_alarm) {
        FILE *f = fopen(REPORT_FILE, "a");
        if(f) { fprintf(f, "KLIENT: grupa %d os. zjadla przy stoliku %d.\n", group, idx); fclose(f); }
    }
    shmdt(shm);
    return 0;
}

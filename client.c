#include "common.h"
#include <string.h>
#include <pthread.h>

typedef struct {
    int id;
    int pid;
    int msgid_kasa;
    int msgid_staff;
} ThreadArg;

void log_thread(const char* msg, int pid, int thread_id) {
    FILE *f = fopen(REPORT_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        char* t_str = ctime(&now);
        t_str[strlen(t_str) -1] = '\0';
        fprintf(f, "[%s] [KLIENT %d.%d]-> %s\n", t_str, pid, thread_id, msg);
        fclose(f);
    }
}

void* client_thread_action(void* arg) {
    ThreadArg* data = (ThreadArg*)arg;
    
    PaymentMsg req;
    PaymentMsg receipt;

    req.mtype = MSG_TYPE_PAYMENT;
    req.client_pid = data->pid;
    req.group_size = 1;

    if(msgsnd(data->msgid_kasa, &req, sizeof(PaymentMsg)-sizeof(long), 0) == -1) {
        free(data);
        return NULL;
    }
    if(msgrcv(data->msgid_kasa, &receipt, sizeof(PaymentMsg)-sizeof(long), data->pid, 0) == -1) {
        free(data);
        return NULL;
    }

    req.mtype = MSG_TYPE_ORDER_FOOD;
    if(msgsnd(data->msgid_staff, &req, sizeof(PaymentMsg)-sizeof(long), 0) == -1) {
        free(data);
        return NULL;
    }
    if(msgrcv(data->msgid_staff, &receipt, sizeof(PaymentMsg)-sizeof(long), data->pid, 0) == -1) {
        free(data);
        return NULL;
    }

    log_thread("Zjadlem i wychodze.", data->pid, data->id);
    free(data);
    return NULL;
}

int main(int argc, char *argv[]) {
    int group = 1;
    if (argc > 1)
        group = atoi(argv[1]);
    if (group < 1)
        group = 1;
    if (group > 20)
        group = 20;

    int pid = getpid();
    srand(pid ^ time(NULL));

    key_t k_shm = get_key(PROJ_ID_SHM);
    key_t k_sem = get_key(PROJ_ID_SEM);
    key_t k_msg = get_key(PROJ_ID_MSG);
    key_t k_kasa = get_key(PROJ_ID_KASA);

    int shmid = shmget(k_shm, sizeof(BarSharedMemory), 0600);
    if(shmid == -1)
        return 0;
    BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
    int semid = semget(k_sem, SEM_COUNT, 0600);
    int msgid_kasa = msgget(k_kasa, 0600);
    int msgid_staff = msgget(k_msg, 0600);

    enter_queue(semid);

    if ((rand() % 100) < 5) {
        FILE *f = fopen(REPORT_FILE, "a");
        if (f) {
            time_t now = time(NULL);
            char* t_str = ctime(&now);
            t_str[strlen(t_str) -1] = '\0';
            fprintf(f, "[%s] [KLIENT_PROC %d]-> Rezygnacja na wejsciu, wychodze 5 procent.\n", t_str, pid);
            fclose(f);
        }
        leave_queue(semid);
        shmdt(shm);
        return 0;
    }

    if(shm->fire_alarm || shm->stop_simulation) {
        leave_queue(semid);
        shmdt(shm);
        return 0;
    }

    int idx = -1;
    
    while(idx == -1) {
        if(shm->fire_alarm || shm->stop_simulation)
            break;

        lock_tables(semid);
        int start_idx = rand() % shm->table_count;
        for(int i=0; i<shm->table_count; i++) {
            int j = (start_idx + i) % shm->table_count;
            Table *t = &shm->tables[j];
            if(t->is_reserved)
                continue;
            
            if(t->current_count == 0 && t->capacity >= group) {
                t->current_count = group;
                t->current_group_size = group;
                idx = j;
                break;
            }
            if(t->current_group_size == group && (t->capacity - t->current_count >= group)) {
                t->current_count += group;
                idx = j;
                break;
            }
        }
        unlock_tables(semid);

        if(idx != -1)
            break;
        
        PaymentMsg signal;
        if (msgrcv(msgid_kasa, &signal, sizeof(PaymentMsg)-sizeof(long), MSG_TYPE_FREE_TABLE, 0) == -1)
            break;
    }

    if(idx == -1) {
        leave_queue(semid);
        shmdt(shm);
        return 0;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * group);
    for(int i = 0; i < group; i++) {
        ThreadArg *arg = malloc(sizeof(ThreadArg));
        arg->id = i + 1;
        arg->pid = pid;
        arg->msgid_kasa = msgid_kasa;
        arg->msgid_staff = msgid_staff;
        pthread_create(&threads[i], NULL, client_thread_action, arg);
    }
    for(int i = 0; i < group; i++)
        pthread_join(threads[i], NULL);
        
    free(threads);

    if(!shm->fire_alarm) {
        lock_tables(semid);
        shm->tables[idx].current_count -= group;
        if(shm->tables[idx].current_count == 0)
            shm->tables[idx].current_group_size = 0;
        unlock_tables(semid);

        PaymentMsg wake; wake.mtype = MSG_TYPE_FREE_TABLE;
        msgsnd(msgid_kasa, &wake, sizeof(PaymentMsg)-sizeof(long), IPC_NOWAIT);
    }

    leave_queue(semid);
    shmdt(shm);
    return 0;
}
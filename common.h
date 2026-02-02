#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h>    
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define FTOK_PATH "."
#define PROJ_ID_SHM 1
#define PROJ_ID_SEM 2
#define PROJ_ID_MSG 3
#define PROJ_ID_KASA 4

#define FIFO_FILE "kitchen_stats_fifo"
#define REPORT_FILE "raport_bar.txt"

#define MAX_TABLES 100
#define MSG_END_WORK 999

#define SIG_DOUBLE_X3 SIGUSR1 
#define SIG_RESERVE   SIGUSR2
#define SIG_FIRE      SIGRTMIN
#define SIG_NEW_WAVE  SIGRTMIN+1

#define SEM_ACCESS 0         
#define SEM_QUEUE_LIMITER 1  
#define SEM_BARRIER 2        
#define SEM_COUNT 3          

#define MSG_TYPE_PAYMENT 1     
#define MSG_TYPE_ORDER_FOOD 2  
#define MSG_TYPE_FREE_TABLE 3  

typedef struct {
    int id;
    int capacity;
    int current_count;
    int current_group_size;
    int is_reserved;
} Table;

typedef struct{
    pid_t staff_pid;
    pid_t cashier_pid;
    pid_t menager_pid;
    pid_t generator_pid;
    int stop_simulation;
    int fire_alarm;
    int x3_tripled;
    int table_count;
    Table tables[MAX_TABLES];
} BarSharedMemory;

typedef struct{
    long mtype;
    int count;
} MenagerOrderMsg;

typedef struct{
    long mtype;
    int client_pid;
    int group_size; 
} PaymentMsg;

static void sem_call(int semid, int sem_num, int op) {
    struct sembuf s;
    s.sem_num = sem_num;
    s.sem_op = op;
    s.sem_flg = 0;
    while (semop(semid, &s, 1) == -1) {
        if (errno == EINTR)
            continue;
        if (errno == EIDRM || errno == EINVAL)
            exit(0);
        perror("semop error");
        exit(1);
    }
}

static void lock_tables(int semid) { sem_call(semid, SEM_ACCESS, -1); }
static void unlock_tables(int semid) { sem_call(semid, SEM_ACCESS, 1); }
static void enter_queue(int semid) { sem_call(semid, SEM_QUEUE_LIMITER, -1); }
static void leave_queue(int semid) { sem_call(semid, SEM_QUEUE_LIMITER, 1); }

static key_t get_key(int proj_id) {
    key_t key = ftok(FTOK_PATH, proj_id);
    if (key == -1) {
        perror("ftok error");
        exit(1);
    }
    return key;
}

#endif
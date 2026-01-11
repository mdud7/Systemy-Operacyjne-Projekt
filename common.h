#ifndef COMMON_H
#define COMMON_H

#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define SHM_KEY 0x1001 //pam dziel
#define SEM_KEY 0x1002 //semafor do sali
#define MSG_KEY 0x1003 //kolejka komunikacja kierownik pracowanik
#define KASA_KEY 0x1004	//kom klient kasjer

#define MAX_TABLES 100
#define REPORT_FILE "raport_bar.txt"

#define SIG_TRIPLE_X3 SIGUSR1
#define SIG_RESERVE   SIGUSR2
#define SIG_FIRE      SIGRTMIN



typedef struct {
	int id;
	int capacity;
	int current_count;
	int current_group_size;
	int is_reserved; // 0 - nie, 1 - tak
} Table;


typedef struct{
	pid_t staff_pid;
	pid_t cashier_pid;
	pid_t menager_pid;

	int stop_simulation;	// 1 - koniec
	int fire_alarm;		// 1 - tak
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
	int group_size;
	int client_pid;
} PaymentMsg;

#endif

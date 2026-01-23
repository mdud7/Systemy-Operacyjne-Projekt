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
#include <signal.h>
#include <errno.h>
#include <time.h>

//      TYPY WIADOMOSCI DO KOLEJEK
//  1 - zamownie / platnosc
//  pid_klienta - potwierdzenie platonsoci leci do dla pid danego klienta
//  3 - sygnal zwolnienia miejsca przy stoliku
//  999 - synchronizacja startu

//  	KONFIGURACJA ZASOBOW
#define SHM_KEY 0x54983A01 // Pamiec dzielona
#define SEM_KEY 0x54983A02 // Zestaw semaforow
#define MSG_KEY 0x54983A03 // Kolejka Menager <-> Staff
#define KASA_KEY 0x54983A04 // Kolejka Klient <-> Kasjer
#define MSG_END_WORK 999

#define MAX_TABLES 100
#define REPORT_FILE "raport_bar.txt"

//		 SYGNALY STERUJACE
#define SIG_DOUBLE_X3 SIGUSR1  // Sygnal podwojenia stolikow (X3)
#define SIG_RESERVE   SIGUSR2  // Sygnal rezerwacji miejsc
#define SIG_FIRE      SIGRTMIN // Sygnal pozaru (priorytetowy)

//       SEMAFORY
#define SEM_ACCESS 0          // Semafor dostepu do stolikow
#define SEM_QUEUE_LIMITER 1  // Semafor "licznik" klientow w kolejce

// STRUKTURY DANYCH

// Struktura pojedynczego stolika
typedef struct {
    int id;
    int capacity;           // Pojemnosc: 1, 2, 3 lub 4 osoby
    int current_count;      // Aktualna liczba osob
    int current_group_size; // Rozmiar grupy zajmujacej stolik
    int is_reserved;        // 0 - wolny, 1 - zarezerwowany przez Menagera
} Table;

// Glowna struktura pamieci dzielonej
typedef struct{
    pid_t staff_pid;
    pid_t cashier_pid;
    pid_t menager_pid;

    int stop_simulation;    // Flaga 1 konczy dzialanie wszystkich procesow
    int fire_alarm;         // Flaga 1 = ewakuacja
    int x3_tripled;         // Flaga czy wykonano juz podwojenie stolikow
    int table_count;        // Aktualna calkowita liczba stolikow
    Table tables[MAX_TABLES];
} BarSharedMemory;

// Komunikat Menager -> Staff (Rezerwacja)
typedef struct{
    long mtype;
    int count; 		// Liczba miejsc do rezerwacji
} MenagerOrderMsg;

// Komunikat Klient -> Kasjer (Platnosc)
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

        if (errno == EINTR) continue;

        if (errno == EIDRM || errno == EINVAL) {
            exit(0); 
        }

        perror("semop error");
        exit(1);
    }
}

static void lock_tables(int semid) {
    sem_call(semid, SEM_ACCESS, -1);
}

static void unlock_tables(int semid) {
    sem_call(semid, SEM_ACCESS, 1);
}

static void enter_queue(int semid) {
    sem_call(semid, SEM_QUEUE_LIMITER, -1);
}

static void leave_queue(int semid) {
    sem_call(semid, SEM_QUEUE_LIMITER, 1);
}
#endif

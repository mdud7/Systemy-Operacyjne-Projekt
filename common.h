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

//  	KONFIGURACJA ZASOBOW
#define SHM_KEY 0x99A1 // Pamiec dzielona
#define SEM_KEY 0x99A2 // Zestaw semaforow
#define MSG_KEY 0x99A3 // Kolejka Menager <-> Staff
#define KASA_KEY 0x99A4 // Kolejka Klient <-> Kasjer
#define MSG_END_WORK 999

#define MAX_TABLES 100
#define REPORT_FILE "raport_bar.txt"

//		 SYGNAŁY STERUJĄCE
#define SIG_DOUBLE_X3 SIGUSR1  // Sygnal podwojenia stolikow (X3)
#define SIG_RESERVE   SIGUSR2  // Sygnal rezerwacji miejsc
#define SIG_FIRE      SIGRTMIN // Sygnal pozaru (priorytetowy)

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

#endif

/*

==	common.h	==

Ten plik zawiera wstepne definicje wspolnych struktur danych i stalych.

Zawiera:
 - Struktura reprezentujaca stoliki('Table') i dane dzielone ('ShareData')
 - Struktura komunikatow dla klientow ('ClientMsg')

 - Stale konfig:

	* MAX_TABLES, MAX_CLIENTSS - ...
	* SHM_KEY - klucz do pamieci dzielonej
	* SEM_KEY - klucz do semaforow
	* MSG_KEY_CASHIER, MSG_KEY_SERVICE - klucze do kolejki komunikatow

Struktury i klucze beda uzywane przy implementacji :
	* pamieci wspoldzielonej
	* semaforow systemowych
	* komunikacji miedzyprocesowej (kolejki komunikatow)

	[02.12.2025]

*/

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

#define MAX_TABLES 20
#define MAX_CLIENTS 50
#define SHM_KEY 0x1234		//klucz do pamieci dzielonej
#define SEM_KEY 0x5678		//klucz do semaforow
#define MSG_KEY_CASHIER 0x1111	//kolejka kom. do kasjera
#define MSG_KEY_SERVICE 0x2222	//kolejka kom. do obslugi

typedef struct {
	int size;	//liczba miejc wolnych
	int occupied;	// 0 - wolny, 1 - zajety
	int reserved; 	//  - || -
} Table;

typedef struct {
	long mtype;	//typ komunikatu dla kolejki komunikatow
	int client_id;
	int group_size;
} ClientMsg;

typedef struct {
	int fire;	// 0 - brak, 1 - pozar
	int X3_boost; 	// czy stoliki 3-osobowe podwojone ( 0 - nie )
	Table tables[MAX_TABLES];
} SharedData;

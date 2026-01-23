#include "common.h"
#include <string.h>
#include <errno.h>

/**
 * staff.c
 * Proces odpowiedzialny za logikę obsługi stolików.
 * - Monitoruje stan zajętości stolików w pamięci współdzielonej.
 * - Obsługuje rezerwacje oraz sygnały od zarządcy.
 * - Synchronizuje gotowość baru do otwarcia z procesem głównym.
 */

volatile sig_atomic_t flag_triple = 0;
volatile sig_atomic_t flag_reserve = 0;
volatile sig_atomic_t flag_end = 0;


void log_action(const char* msg) {
	FILE* f = fopen(REPORT_FILE, "a");
	if (f) {
		time_t now = time(NULL);
		char *t_str = ctime(&now);
		t_str[strlen(t_str)-1] = '\0';
		fprintf(f, "[%s] [STAFF]-> %s\n", t_str, msg);
		fclose(f);
	}
}

void signal_handler(int sig) {
	if (sig == SIG_DOUBLE_X3)
		flag_triple = 1;
	if (sig == SIG_RESERVE)
		flag_reserve = 1;
	if (sig == SIGTERM)
		flag_end = 1;
}

int main() {
	int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
	BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
	int semid = semget(SEM_KEY, 2, 0600);
	int msgid = msgget(MSG_KEY, 0600);
	int msgid_kasa = msgget(KASA_KEY, 0600);

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
	ready.mtype = 999;
	msgsnd(msgid, &ready, sizeof(MenagerOrderMsg) - sizeof(long), 0);

	log_action("obsluga gotowa, czekam na rozkaz podwojenia (X3) lub rezerwacje.");

	MenagerOrderMsg msg;
	while (1) {
		ssize_t result = msgrcv(msgid, &msg, sizeof(MenagerOrderMsg) - sizeof(long), 1, 0);

        if (result == -1) {
            if (errno != EINTR) 
				break;
        }
        
		if (shm->fire_alarm) { log_action("POZAR, ewakuacja"); break; }
        if (shm->stop_simulation) { log_action("Koniec pracy."); break; }

	if (flag_reserve) {
            MenagerOrderMsg r_msg;
            while (msgrcv(msgid, &r_msg, sizeof(MenagerOrderMsg), 1, IPC_NOWAIT) != -1) {
                lock_tables(semid);
                int reserved_cnt = 0;
                for (int i = 0; i < shm->table_count && reserved_cnt < r_msg.count; i++) {
                    if (shm->tables[i].current_count == 0 && !shm->tables[i].is_reserved) {
                        shm->tables[i].is_reserved = 1;
                        reserved_cnt++;
                    }
                }
                unlock_tables(semid);
                char buf[64]; 
				sprintf(buf, "Zarezerwowano %d stolikow.", reserved_cnt);
                log_action(buf);
            }
            flag_reserve = 0;
        }

	if (flag_triple) {
		lock_tables(semid);

		if (shm->x3_tripled) {
			log_action("otrzymano sygnal podwojenia ale stoliki sa juz zwiekszone");
		} else {
				int current_total = shm->table_count;
				int count_x3 = 0;
                		for (int i=0; i<current_total; i++) {
                    		if (shm->tables[i].capacity == 3) count_x3++;
                		}

				int to_add = count_x3 * 2;
				int added = 0;

				for (int i=0; i < to_add; i++) {
                    			if (shm->table_count < MAX_TABLES) {
                        			int idx = shm->table_count;
                        			shm->tables[idx].id = idx;
                        			shm->tables[idx].capacity = 3;
                        			shm->tables[idx].current_count = 0;
                        			shm->tables[idx].current_group_size = 0;
                        			shm->tables[idx].is_reserved = 0;
									shm->table_count++;
                        			added++;
                    			}
                		}

				shm->x3_tripled = 1;

				char buf[128];
				sprintf(buf, "Podwojono stoliki 3-osobowe, bylo: %d, dodano: %d (suma: %d).", count_x3, added, count_x3 + added);
                log_action(buf);

				PaymentMsg wake; wake.mtype = 3;
                for(int k=0; k<added; k++) 
                	msgsnd(msgid_kasa, &wake, sizeof(PaymentMsg)-sizeof(long), 0);
            }

			unlock_tables(semid);
        	flag_triple = 0;
		}
	}

	shmdt(shm);
	return 0;
}

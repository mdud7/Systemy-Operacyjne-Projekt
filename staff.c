#include "common.h"
#include <string.h>

volatile sig_atomic_t flag_triple = 0;
volatile sig_atomic_t flag_reserve = 0;

void sem_lock(int semid) {
	struct sembuf sb = {0, -1, 0};
	if (semop(semid, &sb, 1) == -1) {
		if (errno != EINTR) perror("sem_lock error");
	}
}

void sem_unlock(int semid) {
	struct sembuf sb = {0, 1, 0};
	if (semop(semid, &sb, 1) == -1) {
		perror("sem_unlock_error");
	}
}

void log_action(const char* msg) {
	FILE* f = fopen(REPORT_FILE, "a");
	if (f) {
		time_t now = time(NULL);
		char *t_str = ctime(&now);
		t_str[strlen(t_str)-1] = '\0';
		fprintf(f, "[%s] [STAFF]-> %s\n", t_str, msg);
		fclose(f);
	}

	printf("[STAFF] %s\n", msg);
}

void signal_handler(int sig) {
	if (sig == SIG_DOUBLE_X3) {
		flag_triple = 1;
	}
	if ( sig == SIG_RESERVE) {
		flag_reserve = 1;
	}
}

int main() {
	int shmid = shmget(SHM_KEY, sizeof(BarSharedMemory), 0600);
	BarSharedMemory *shm = (BarSharedMemory*)shmat(shmid, NULL, 0);
	int semid = semget(SEM_KEY, 1, 0600);
	int msgid = msgget(MSG_KEY, 0600);

	struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
	sigaction(SIG_DOUBLE_X3, &sa, NULL);
	sigaction(SIG_RESERVE, &sa, NULL);

	sem_lock(semid);
	shm->staff_pid = getpid();
	sem_unlock(semid);

	log_action("obsluga gotowa, czekam na rozkaz podwojenia (X3) lub rezerwacje.");

	while (1) {
        if (shm->fire_alarm) { log_action("POZAR, ewakuacja"); break; }
        if (shm->stop_simulation) { log_action("Koniec pracy."); break; }

	if (flag_reserve) {
            MenagerOrderMsg msg;
            while (msgrcv(msgid, &msg, sizeof(MenagerOrderMsg), 1, IPC_NOWAIT) != -1) {
                sem_lock(semid);
                int reserved_cnt = 0;
                for (int i = 0; i < shm->table_count && reserved_cnt < msg.count; i++) {
                    if (shm->tables[i].current_count == 0 && !shm->tables[i].is_reserved) {
                        shm->tables[i].is_reserved = 1;
                        reserved_cnt++;
                    }
                }
                sem_unlock(semid);
                char buf[64]; 
				sprintf(buf, "Zarezerwowano %d miejsc.", reserved_cnt);
                log_action(buf);
            }
            flag_reserve = 0;
        }

	if (flag_triple) {
		sem_lock(semid);

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
                        			shm->tables[idx].capacity = 3;       // 3-osobowy
                        			shm->tables[idx].current_count = 0;  // Pusty
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
            }

			sem_unlock(semid);
        	flag_triple = 0;
		}

		usleep(100000);

	}

	shmdt(shm);
	return 0;
}

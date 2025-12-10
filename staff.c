#include "common.h"

volatile sig_atomic_t flag_triple = 0;
volatile sig_atomic_t flag_check_msg = 0;

void chech(int result, const char* msg) {
	if (result == -1) {
		perror(msg);
		exit(1);
	}
}

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
		time_t now = time(NULL):
		char *t_str = ctime(&now);
		t_str[strlen(t_str)-1] = '\0';
		frpintf(f, "[%s] STAFF: %s\n", t_str, msg);
		fclose(f);
	}

	printf("[STAFF] %s\n", msg);
}

void signal_handler(int sig) {
	if (sig == SIG_TRIPLE_X) {
		flag_triples_tables = 1;
	}
	if ( sig == SIG_RESERVE) {
		flag_check_msg = 1;
	}
}


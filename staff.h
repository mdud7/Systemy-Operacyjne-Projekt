#ifndef STAFF_H
#define STAFF_H

void check(int result, const char* msg);
void sem_lock(int semid);
void sem_unlock(int semid);
void log_action(const char* msg);
void signal_handler(int sig);

#endif

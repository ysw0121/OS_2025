#ifndef __lib_h__
#define __lib_h__

#include "types.h"

#define SYS_WRITE 0
#define SYS_READ 1
#define SYS_FORK 2
#define SYS_EXEC 3
#define SYS_SLEEP 4
#define SYS_EXIT 5
#define SYS_SEM 6
#define SYS_SHAREDVAR 7

#define STD_OUT 0
#define STD_IN 1

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

#define SHAREDVAR_CREATE 0
#define SHAREDVAR_DESTROY 1
#define SHAREDVAR_READ 2
#define SHAREDVAR_WRITE 3

#define MAX_BUFFER_SIZE 256

int printf(const char *format,...);

int scanf(const char *format,...);

pid_t fork();

int exec(void (*func)(void));

int sleep(uint32_t time);

int exit();

int sem_init(sem_t *sem, uint32_t value);

int sem_wait(sem_t *sem);

int sem_post(sem_t *sem);

int sem_destroy(sem_t *sem);

int createSharedVariable(sharedvar_t *svar, int value);

int destroySharedVariable(sharedvar_t *svar);

int readSharedVariable(sharedvar_t *svar);

int writeSharedVariable(sharedvar_t *svar, int value);

#endif

/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>



void itoa(int a, char *b);

int strlen(char *a);

int getpid();

int fork();

void exit();

void perror();

int write(int fd, char *buffer, int size);

int gettime();

int get_stats(int pid, struct stats *st);

int clone(void (*function)(void), void *stack);

int read (int fd, char *buf, int count);

int sem_init(int n_sem,unsigned int value);

int sem_wait(int n_sem);

int sem_signal(int n_sem);

int sem_destroy(int n_sem);

void *sbrk(int increment);

#endif  /* __LIBC_H__ */

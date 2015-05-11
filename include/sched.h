/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>

#define POLICY 1
#define ROUNDROBIN 1
#define QUANTUM 100
#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED, ST_ZOMBIE };

struct task_struct {
  int PID;			/* Process ID */
  int kernel_esp;		/* esp que apunta a la pila del proceso*/
  struct list_head list;
  page_table_entry * dir_pages_baseAddr;
  int quantum;
  int quantumRest;
  int tics;
  int rafagas_cpu;
  int characters_left;
  int heap_start;
  int heap_break;
  int offset;
  struct list_head sems;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

struct semaphore{
	struct list_head list;
	int count;
	struct list_head blocked;
	int owner;
};

extern union task_union task[NR_TASKS]; /* Vector de tasques */
extern struct task_struct *idle_task;

struct semaphore sem[20];

#define KERNEL_ESP       (DWord) &task[1].stack[KERNEL_STACK_SIZE]

int find_dir(page_table_entry * dir);
void allocate_page_dir (struct task_struct *p);
struct task_struct * get_free_pcb();
page_table_entry * get_free_dir();

void schedule(void);
void roundRobin_tics(void);
void schedule_exit(void);
void roundRobin_exit(void);

/* Inicialitza les dades del proces inicial */
int asignar_PID();
void init_task1(void);
void init_idle(void);
void init_sched(void);

struct task_struct * current();
void task_switch(union task_union*t);
struct task_struct *list_head_to_task_struct(struct list_head *l);
struct semaphore * list_head_to_semaphore(struct list_head *l);

page_table_entry * get_PT (struct task_struct *t);
page_table_entry * get_DIR (struct task_struct *t);

void process_block(struct list_head * list);
void process_block_at_begining(struct list_head * list);
void process_unblock(struct task_struct * pcb);


#endif  /* __SCHED_H__ */

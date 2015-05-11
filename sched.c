/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>


union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif 

struct semaphore * list_head_to_semaphore(struct list_head *l)
{
	return (struct semaphore *)l;
}

/* Global variables */
int counterPID = 0;
int dir_pages_used[NR_TASKS];
struct task_struct *idle_task;

struct list_head freequeue;
struct list_head readyqueue;


/* External global variables */
extern page_table_entry dir_pages[NR_TASKS][TOTAL_PAGES];
extern struct list_head blocked;
extern struct list_head keyboardqueue;

/*
 * Memory Directory
 */

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}
/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

page_table_entry * get_free_dir(){
	int i=0;
	int found=0;
	while(!found){
		if(dir_pages_used[i] == 0) found = 1;
		++i;
	}
	return dir_pages[--i];



}

int find_dir(page_table_entry * dir){
	int i=0;
	int found=0;
	while(!found){
		if(dir == &dir_pages[i]) found = 1;
		++i;
	}
	return --i;
}


//Assign a page directory to a process.
void allocate_page_dir (struct task_struct *p) {
        /*int pos = ((int) task - (int) p) / sizeof(union task_union);
        p->dir_pages_baseAddr = &dir_pages[pos];*/
	int i = 0;
	int free = 0;
	while(!free){
		if(dir_pages_used[i] == 0) free = 1;
		++i;
	}
	--i;
	p->dir_pages_baseAddr=&dir_pages[i];
}

/*
 * Scheduling
 */

int asignar_PID(){
	
	return counterPID++;
}

struct task_struct * get_free_pcb(){
	if(list_empty(&freequeue) != 1){
		struct task_struct *pcb = list_head_to_task_struct(list_first(&freequeue));
		list_del(list_first(&freequeue));
		return pcb;
	}
	else{
		return NULL;
	}

}

void schedule(){
	if(POLICY == ROUNDROBIN)
		roundRobin_tics();
}

void schedule_exit(){
	if(POLICY == ROUNDROBIN)
		roundRobin_exit();
}

void roundRobin_tics(){
	struct task_struct * pcb = current();
	--pcb->quantumRest;
	pcb->tics +=1;
	if(pcb->quantumRest == 0){
		
		if( list_empty(&readyqueue) != 1 ){
			pcb->quantumRest = pcb->quantum;
			list_add_tail(&pcb->list, &readyqueue);
			pcb = list_head_to_task_struct(list_first(&readyqueue));
			list_del(list_first(&readyqueue));
			task_switch((union task_union*) pcb);
		}
		else
			pcb->quantumRest = pcb->quantum;
	}
}

void roundRobin_exit(){
	if( list_empty(&readyqueue) != 1 ){
		struct task_struct * pcb = list_head_to_task_struct(list_first(&readyqueue));
		list_del(list_first(&readyqueue));
		task_switch((union task_union*) pcb);
	}
	else
		task_switch((union task_union*) idle_task);
}


/*
 * Initial tasks
 */


void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");
	printk("idle\n");
	while(1)
	{
		
	}
}

void init_idle (void)
{
	struct task_struct * pcb;
  	pcb = list_head_to_task_struct(list_first(&freequeue));
	pcb->PID = asignar_PID();
	pcb->quantum = QUANTUM;
	pcb->quantumRest = QUANTUM;
	pcb->tics=0;
	pcb->rafagas_cpu = 1;

	list_del(list_first(&freequeue));

	union task_union * task = (union task_union*)  pcb;
	task->stack[1023] = (int)cpu_idle;
	task->stack[1022] = 0;
	pcb->kernel_esp= (int) &task->stack[1022];
	idle_task = pcb;
	dir_pages_used[ find_dir(pcb->dir_pages_baseAddr) ]++;
	INIT_LIST_HEAD(&pcb->sems);

}

void init_task1(void)
{
	struct task_struct * pcb;	
  	pcb = list_head_to_task_struct(list_first(&freequeue));

	pcb->PID = asignar_PID();
	pcb->quantum = QUANTUM;
	pcb->quantumRest = QUANTUM;
	pcb->tics=0;
	pcb->rafagas_cpu = 1;
	pcb->heap_start = PAG_HEAT;
	pcb->heap_break = 0x00;
	list_del(list_first(&freequeue));
	set_user_pages(pcb);	
	set_cr3(get_DIR(pcb));
	dir_pages_used[ find_dir(pcb->dir_pages_baseAddr) ]++;
	INIT_LIST_HEAD(&pcb->sems);
	

}


void init_sched(){
	int i;			
	//iniciamos las dos colas
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	INIT_LIST_HEAD(&keyboardqueue);
	

	//todos los pcb's estan Free
	for(i = 0; i < NR_TASKS; ++i) {
		list_add_tail(&task[i].task.list, &freequeue);
		dir_pages_used[i] = 0;
	}
}

void task_switch(union task_union * new){
	struct task_struct * actual = current();
	struct task_struct * pcb = (struct task_struct *) new;
/*1) Update the TSS to make it point to the new_task system stack.*/
/* TSS */
	actual->quantumRest = actual->quantum;
	pcb->rafagas_cpu++;
	tss.esp0 = (int)&(new->stack[KERNEL_STACK_SIZE]);

/*2) Change the user address space by updating the current page directory: use the set_cr3 funtion to set the cr3 register to point to the page directory of the new_task.*/
	page_table_entry * dir_new = get_DIR(pcb);
	page_table_entry * dir_actual = get_DIR(actual);
	if( dir_new != dir_actual)
		set_cr3(dir_new);

/*3) Store, in the PCB, the current value of the EBP register (corresponding to the position in the current system stack where this routine begins).*/
	__asm__ __volatile__("movl %%ebp, %0":"=r"(actual->kernel_esp));
/*4) Change the current system stack by setting ESP register to point to the stored value in the new PCB.*/
	__asm__ __volatile__("movl %0, %%esp"::"r" (pcb->kernel_esp));

/*5) Restore the EBP register from the stack.*/
/*6) RET.*/
	__asm__ __volatile__("popl %ebp \n"
                             "ret \n");
}


struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

/*
 * Blocks the current() process; Adds it to keyboardqueue.
 * Runs first Ready task. If there isn't any, runs idle task.
 */
void process_block(struct list_head * list)
{
	struct task_struct * pcb;
	pcb = current();
	list_add_tail(&pcb->list, list);

	if(!list_empty(&readyqueue)) {
		pcb = list_head_to_task_struct(list_first(&readyqueue));
		list_del(list_first(&readyqueue));
		task_switch((union task_union*) pcb);
	}
	else
		task_switch((union task_union*) idle_task);
}

void process_block_at_begining(struct list_head * list)
{
	struct task_struct * pcb;
	pcb = current();
	list_add(&pcb->list, list); // <- esta es la unica diferencia respecto a process block

	if(!list_empty(&readyqueue)) {
		pcb = list_head_to_task_struct(list_first(&readyqueue));
		list_del(list_first(&readyqueue));
		task_switch((union task_union*) pcb);
	}
	else
		task_switch((union task_union*) idle_task);
}


/*
 * Unblocks process at *pcb.
 * Adds it to the Readyqueue.
 *//*
void process_unblock(struct task_struct * pcb)
{
	list_del(&pcb->list);
	list_add_tail(&pcb->list, &readyqueue);
}*/





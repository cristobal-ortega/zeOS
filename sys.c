/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <stats.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
extern struct list_head freequeue;
extern struct list_head readyqueue;
extern int counterPID;
extern int dir_pages_used[NR_TASKS];
extern struct semaphore sem[20];

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
  return 0;
}

int check_fd_read(int fd, int permissions)
{
  if (fd!=0) return -EBADF;
  if (permissions!=LECTURA) return -EACCES;
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS;
}
/* 
 * Coming from user routine write()
 * Syscall 20
 */
int sys_getpid()
{
	return current()->PID;
}

void ret_from_fork(){
	__asm__ __volatile__("popl %ebp\n"
	"popl %ebx \n"
	"popl %ecx \n"
	"popl %edx \n"
	"popl %esi \n"
	"popl %edi \n"
	"popl %ebp \n"
	"popl %eax \n"
	"popl %ds \n"
	"popl %es \n"
	"popl %fs \n"
	"popl %gs \n"
	"movl $0, %eax \n"
	"iret \n");
}


/* 
 * Coming from user routine fork()
 * Syscall 2
 */

int sys_fork()
{
  int i;
  int j=0;
  struct task_struct * pcb;
  struct task_struct * actual = current();
  int pag_data[NUM_PAG_DATA];
/*a) Get a free task_struct for the process. If there is no space for a new process, an error
   will be returned.*/

  pcb = get_free_pcb();
  if(pcb == NULL)
	return -ENOPCB;

  /*b) Search physical pages in which to map logical pages for data+stack of the child process
   (using the alloc_frames function). If there is no enough free pages, an error will be
   return.*/
	
  for(i=0; i<NUM_PAG_DATA; ++i){
	pag_data[i] = alloc_frame();
  }
  if( i!= NUM_PAG_DATA){
	for(j=0; j<i; ++j)
		free_frame(pag_data[j]);
	list_add_tail(&pcb->list, &freequeue);
	return -ENOMEM;
  }
  //c) Inherit system data: copy the parent’s task_union to the child. 
  copy_data(actual, pcb, KERNEL_STACK_SIZE*sizeof(int));
  pcb->dir_pages_baseAddr = get_free_dir();
  
  /*d) Inherit user data:
  i) Create new address space: Access page table of the child process through the direc-
     tory field in the task_struct to initialize it (get_PT routine can be used)10 :
     A) Page table entries for the system code and data and for the user code can be a
        copy of the page table entries of the parent process (they will be shared)
     B) Page table entries for the user data+stack have to point to new allocated pages
        which hold this region */
  page_table_entry * process_PT = get_PT(actual);
  page_table_entry * child_PT = get_PT(pcb);

  for(i=0;i<NUM_PAG_CODE; ++i)
	child_PT[PAG_LOG_INIT_CODE_P0 + i].entry = process_PT[PAG_LOG_INIT_CODE_P0 +i].entry;

  for(i=0; i<NUM_PAG_DATA; ++i)
	set_ss_pag(child_PT, PAG_LOG_INIT_DATA_P0 + i, pag_data[i]);
  /*ii) Copy the user data+stack pages from the parent process to the child process. The
      child’s physical pages cannot be directly accessed because they are not mapped in
      the parent’s page table. In addition, they cannot be mapped directly because the
      logical parent process pages are the same. They must therefore be mapped in new
      entries of the page table temporally (only for the copy). Thus, both pages can be
      accessed simultaneously as follows:
      A) Use temporal free entries on the page table of the parent. Use the set_ss_pag and
         del_ss_pag functions.
      B) Copy data+stack pages.
      C) Free temporal entries in the page table and flush the TLB to really disable the
         parent process to access the child pages.*/

  int pag_temp_child[NUM_PAG_DATA];
  i=28;j=0;
	//logicas del padre libres
  while(i<TOTAL_PAGES && j<NUM_PAG_DATA){
	if( process_PT[PAG_LOG_INIT_CODE_P0 + i].entry == 0){
		pag_temp_child[j] = i;
		++j;
	}
	++i;
  }
  if(j!=NUM_PAG_DATA){
	for(i=0; i<NUM_PAG_DATA; ++j)
		free_frame(pag_data[i]);
	list_add_tail(&pcb->list, &freequeue);
	return -ENOLOG;
  }

  for(i=0; i<NUM_PAG_DATA; ++i){
	//linkar pagina logica libre del padre con fisica del hijo
	set_ss_pag(process_PT, PAG_LOG_INIT_DATA_P0 + pag_temp_child[i], pag_data[i]);
	//pagina de datos del padre	
	unsigned int start = PAGE_SIZE * ( PAG_LOG_INIT_DATA_P0 + i );
	//pagina linkada del hijo al padre
	unsigned int dest = PAGE_SIZE*(PAG_LOG_INIT_DATA_P0 + pag_temp_child[i] );
	copy_data((void*)start, (void*)dest, PAGE_SIZE);
	del_ss_pag(process_PT,PAG_LOG_INIT_DATA_P0 + pag_temp_child[i] );
  }

  set_cr3(get_DIR(actual));
  /*e) Assign a new PID to the process. The PID must be different from its position in the
   task_array table.
   f) Initialize the fields of the task_struct that are not common to the child.*/
  	pcb->PID = asignar_PID();
	pcb->quantum = QUANTUM;
	pcb->quantumRest = QUANTUM;
	pcb->tics=0;
	pcb->rafagas_cpu = 0;
  	dir_pages_used[ find_dir(pcb->dir_pages_baseAddr) ]++;
  /*
    i) Think about the register or registers that will not be common in the returning of the
       child process and modify its content in the system stack so that each one receive its
       values when the context is restored.
   g) Prepare the child stack emulating a call to context_switch and be able to restore its
      context in a known position. The stack of the new process must be forged so it can be
      restored at some point in the future by a task_switch. In fact the new process has to
      restore its hardware context and continue the execution of the user process, so you can
      create a routine ret_from_fork which does exactly this. And use it as the restore point
      like in the idle process initialization 4.4.
   h) Insert the new process into the ready list: readyqueue. This list will contain all processes
      that are ready to execute but there is no processor to run them.
   i) Return the pid of the child process.*/

  
  union task_union * task = (union task_union*)  pcb;
  task->stack[1007] = (int)ret_from_fork;
  pcb->kernel_esp = (int)&task->stack[1006];

  list_add_tail(&(pcb->list), &readyqueue);
   INIT_LIST_HEAD(&pcb->sems);

  return pcb->PID;
}
/* 
 * Coming from user routine exit()
 * Syscall 1
 */
void sys_exit()
{  
	struct task_struct * pcb = current();
	if(pcb->PID == 1)
		return;
	pcb->PID = -1;
	/*Control de threads y directorios:*/
	page_table_entry * dir = pcb->dir_pages_baseAddr;
	int i = find_dir(dir);
	if(--dir_pages_used[i] == 0)
		free_user_pages(pcb);
	

	while( !list_empty(&pcb->sems) ){
		struct semaphore * s = list_head_to_semaphore(list_first(&pcb->sems));
		while( !list_empty(&s->blocked) ){
			pcb = list_head_to_task_struct(list_first(&s->blocked));
			list_del(list_first(&s->blocked));
			list_add_tail(&pcb->list, &readyqueue);
		}
		s->count=-1;
		s->owner=-1;
	}

	list_add_tail(&pcb->list, &freequeue);
	schedule_exit();	
}

/* 
 * Coming from user routine write()
 * Syscall 4
 */

int sys_write(int fd, char * buffer, int size) {
	int n = 0;
	int res;
	char buffer_kernel[4];
	DWord b = (DWord) buffer;
	
	res = check_fd(fd, ESCRIPTURA);
	if (res < 0) 
		return res;
	
	if (buffer == 0) 
		return -EBNULL;
	
	if (size == 0)
		return 0;

	if (size < 0)
		return -ESIZE;
	
	while (size > 3) {
		copy_from_user((void *)b, (void *)buffer_kernel, 4);
		n += sys_write_console (buffer_kernel, 4);
		size -= 4;
		b += 4;
	}
	
	while (size > 0) {
		copy_from_user((void *)b, (void *)buffer_kernel, 1);
		n += sys_write_console (buffer_kernel, 1);
		size--;
		b++;
	}
	
    if (n < 0) return -EWCON;
	else return n;
}

/* 
 * Coming from user routine gettime()
 * Syscall 10
 */

int sys_gettime() {
	return zeos_ticks;
}

/* 
 * Coming from user routine get_stats()
 * Syscall 35
 */
int sys_get_stats(int pid, struct stats *st){
	if (st == NULL) 
		return -EBNULL;
	if (pid < 0 || pid > counterPID)
		return -ENOEXIST;
	if ( !access_ok(ESCRIPTURA,st,sizeof(struct stats)) )
		return -EACCES;
	int i = 0;
	int found = 0;
	while (i<NR_TASKS && found == 0){
		if(task[i].task.PID==pid)
			found = 1;
		++i;
	}
	if(found){
		--i;
		struct stats stats;
		stats.tics = task[i].task.tics;
		stats.cs = task[i].task.rafagas_cpu;
		stats.remaining_quantum = task[i].task.quantumRest;
		copy_to_user(&stats, st, sizeof(struct stats));
	}
	else
		return -ENOEXIST;

	return 0;
}




/* 
 * Coming from user routine clone (void (*function)(void), void *stack)
 * Syscall 03
 */
int sys_clone (void (*function)(void), void *stack){
	if( !access_ok(LECTURA,function,sizeof(int)) )
		return -EACCES;
	if( !access_ok(ESCRIPTURA,stack,sizeof(int)) )
		return -EACCES;

	
	/*  It creates a new process that executes the code at function. The new process shares the whole
	address space with the current one, and stack is the base address of the memory region to be
	used as its user stack.*/
	
	struct task_struct * pcb;
  	struct task_struct * actual = current();

  	pcb = get_free_pcb();
  	if(pcb == NULL)
		return -ENOPCB;

 	//Control de páginas
	/*vector[NR_TASKS]
		vector[i] = contador de numero de procesos con dir_pages_baseAddr = dir_page[i]
	*/
	
	//end control
	copy_data(actual, pcb, KERNEL_STACK_SIZE*sizeof(int));
	
	page_table_entry * thread = pcb->dir_pages_baseAddr;
	int i = find_dir(thread);
	dir_pages_used[i]++;

	pcb->PID = asignar_PID();
	pcb->quantum = QUANTUM;
	pcb->quantumRest = QUANTUM;
	pcb->tics=0;
	pcb->rafagas_cpu = 0;


	/*cambiar eip y esp del hijo
	/2C(%esp) - %eip
	/38(%esp) - %oldesp */ 

	union task_union * task = (union task_union*)  pcb;
  	task->stack[1019] = (int)function;
  	task->stack[1022] = (int)stack;
  	pcb->kernel_esp = (int)&task->stack[1006];
	
	list_add_tail(&(pcb->list), &readyqueue);
	INIT_LIST_HEAD(&pcb->sems); //COMPROBAR SI EL CLONE COPIA SEMAFOROS.
	return pcb->PID;
}


int sys_read(int fd, char *buf, int count)
{
	int res;

	res = check_fd_read(fd, LECTURA);
	
	if (res < 0) 
		return res;
	
	if (buf == 0) 
		return -EBNULL;
	
	if (count == 0)
		return 0;

	if (count < 0)
		return -ESIZE;

	res = sys_read_keyboard(fd, buf, count);
	
	if (res < 0) return -ERCON;
	else return res;

}

int sys_sem_init(int n_sem, unsigned int value)
{
	struct task_struct * actual = current();
	
	if(n_sem>=20 || n_sem<0)
		return -ENOSEM;	
	
	struct semaphore * s = &sem[n_sem];
	
	//iniciar
	s->count = value;
	s->owner = actual->PID;
	INIT_LIST_HEAD(&s->blocked);
	list_add(&s->list,&actual->sems);

	return 0;
}

int sys_sem_wait(int n_sem)
{

	if(n_sem>=20 || n_sem<0)
		return -ENOSEM;		

	struct semaphore * s = &sem[n_sem];
	if(s->count==-1)
		return -ENOSEM;


	while(s->count<=0){
		//no recursos, bloquear	
		process_block_at_begining(&s->blocked);
		if(s->count==-1)
			return -ENOSEM;
	}
	--(s->count);

	return 0;
}

int sys_sem_signal( int n_sem)	
{
	if(n_sem>=20 || n_sem<0)
		return -ENOSEM;		
		//error no existe semaforo
	struct semaphore * s = &sem[n_sem];
	if(s->count==-1)
		return -ENOSEM;
		//error no inicializado
	++(s->count);
	if(!list_empty(&s->blocked)){
		struct task_struct * pcb = list_head_to_task_struct(list_first(&s->blocked));
		list_del(list_first(&s->blocked));
		list_add_tail(&pcb->list, &readyqueue);
	}

	return 0;

}

int sys_sem_destroy(int n_sem){
	if(n_sem>=20 || n_sem<0)
		return -ENOSEM;		

	struct semaphore * s = &sem[n_sem];
	if(s->count==-1)
		return -ENOSEM;

	struct task_struct * pcb = current();
	if(s->owner!=pcb->PID)
		return -ENOWNER;

	while( !list_empty(&s->blocked) ){
		pcb = list_head_to_task_struct(list_first(&s->blocked));
		list_del(list_first(&s->blocked));
		list_add_tail(&pcb->list, &readyqueue);
	}
	s->count=-1;
	s->owner=-1;
	return 0;
}


void * sys_sbrk(int increment)
{
	struct task_struct * pcb = current();
	page_table_entry * process_PT = get_PT(pcb);
	int pag = pcb->heap_break/PAGE_SIZE; 
	
	int offset = pcb->heap_break%PAGE_SIZE;
	int ret;
	if(increment == 0){
		ret = ((pcb->heap_start+pag)<<12);
		return (void *)ret+offset;
		//devolver break
	} else if( increment > 0) {
		//pide 1 pagina entera:
		if(increment==PAGE_SIZE){
			int new_pag = alloc_frame();
			set_ss_pag(process_PT, pcb->heap_start+pag, new_pag);
			pcb->heap_break +=increment;
			printk("RESERVO UNA PAGINA\n");
			ret = ((pcb->heap_start+pag)<<12);
			return (void *)ret+offset;
		}
		//menos de 1 pagina
		if( increment < PAGE_SIZE){
			if(pcb->heap_break+increment >PAGE_SIZE){
				int new_pag = alloc_frame();
				set_ss_pag(process_PT, pcb->heap_start+pag, new_pag);		
				pcb->heap_break +=increment;
				printk("MENOS DE UNA PAGINA\n");
				ret = ((pcb->heap_start+pag)<<12);
				return (void *)ret+offset;
			}else{	
				if(pcb->heap_break==0){
					int new_pag = alloc_frame();
					set_ss_pag(process_PT, pcb->heap_start+pag, new_pag);			
					printk("MENOS DE UNA PAGINA DEVUELVO DENTRO LA PAGINA RESERVANDO\n");
				}
				else{
					printk("MENOS DE UNA PAGINA NO RESERVO\n");
				
				}
				pcb->heap_break +=increment;
				ret = ((pcb->heap_start+pag)<<12);
				return (void *)ret+offset;
			}

		}
		//mas de 1 pagina
		if(increment > PAGE_SIZE){
			int count = increment;
			int brk = pag;
			while(count >= PAGE_SIZE){
				int new_pag = alloc_frame();
				set_ss_pag(process_PT, pcb->heap_start+brk, new_pag);
				++brk;
				count -=PAGE_SIZE;
				printk("RESERVO 1 PAGINAS\n");
			}
			if(count){
				int new_pag = alloc_frame();
				set_ss_pag(process_PT, pcb->heap_start+brk, new_pag);
				printk("RESERVO PAGINA EXTRA\n");
			}
			printk("RESERVO N PAGINAS\n");
			pcb->heap_break +=increment;
			ret = ((pcb->heap_start+pag)<<12);
			return (void *)ret+offset;
		}
	} else{
		if(offset + increment >= 0){
			offset -=increment;
			pcb->heap_break +=increment;
			ret = ((pcb->heap_start+pag)<<12);
			return (void *)ret+offset;
		}else{
			int count = increment;
			int brk = pag+1;
			if( pcb->heap_break%PAGE_SIZE )
				++pag;
			while(count <= -PAGE_SIZE){
				int frame = get_frame(process_PT, pcb->heap_start+brk);
				del_ss_pag(process_PT, pcb->heap_start+brk);
				free_frame(frame);
				--pag;
				--brk;
				count +=PAGE_SIZE;
				
			}
			if(count){
				int frame = get_frame(process_PT, pcb->heap_start+brk);
				del_ss_pag(process_PT, pcb->heap_start+brk);
				free_frame(frame);
				--pag;
				offset += count;
				
			}
			pcb->heap_break +=increment;
			ret = ((pcb->heap_start+pag)<<12);
			return (void *)ret+offset;


		}
	}
	return (void *)-1;
}
		
	



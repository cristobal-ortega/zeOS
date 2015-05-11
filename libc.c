/*
 * libc.c 
 */

#include <libc.h>
#include <types.h>
#include <errno.h>

unsigned int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}


void perror()
{
	char * errstr;
	char errno_to_str[4] = "";
	itoa(errno, errno_to_str);
	write(1, errno_to_str, strlen(errno_to_str));
	write(1, " ", 1);
	
	switch(errno) {
		case EBADF:
			errstr = "Bad file descriptor.\n";
			break;
		case EBNULL:
			errstr = "Reference to a NULL pointer.\n";
			break;
		case ESIZE:
			errstr = "Size or count less than 0.\n";
			break;
		case EWCON:
			errstr = "Error writing on console. \n";
			break;
		case ERCON:
			errstr = "Error reading from keyboard. \n";
			break;
		case EACCES:
			errstr = "Access denied.\n";
			break;
		case ENOPCB:
			errstr = "No PCB free.\n";
			break;
		case ENOMEM:
			errstr = "No memory enough free.\n";
			break;
		case ENOLOG:
			errstr = "No pages logicals free for fork.\n";
			break;
		case ENOEXIST:
			errstr = "No match with a process.\n";
			break;
		case ENOSYS:
			errstr = "System call has no function associated.\n";
			break;
		case ENOSEM:
			errstr = "No exist semaphore.\n";
			break;
		case ENOWNER:
			errstr = "No permision for destroy semaphore.\n";
			break;
		case 0:
			errstr = "No error.\n";
			break;
		default: 
			errstr = "Unknown error.\n";
			break;
	}
	
	write(1, errstr, strlen(errstr));
}


/* Wrapper write()
 * 
 * fd -> %ebx
 * &buffer -> %ecx
 * size -> %edx
 * 0x04 -> %eax
 * 
 * 0x04 es la posicion en sys_call_tables.S que corresponde a sys_write()
 * 
 */

int write(int fd, char* buffer, int size)
{
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res),   
		  "+b" (fd),    
		  "+c" (buffer),   
		  "+d" (size)   
		: "a"  (0x04)    
		: "memory", "cc");
	
    if (res < 0) {
		errno = res * (-1); 
		return -1;
	}
	else return res;
}

/* Wrapper gettime()
 * 
 * 0x0A es la posicion en sys_call_tables.S que corresponde a sys_gettime()
 * 
 */

int gettime()
{
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res)   
		: "a"  (0x0A)    
		: "memory", "cc");
	
    	if (res < 0) { 
		errno = res * (-1);
		return -1;
	}
	else return res;
}
/* Wrapper getpid()
 * 
 * 0x14 es la posicion en sys_call_tables.S que corresponde a sys_getpid()
 * 
 */
int getpid(void){
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res)   
		: "a"  (0x14)    
		: "memory", "cc");

	if (res < 0) { 
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper fork()

 * 
 * 0x02 es la posicion en sys_call_tables.S que corresponde a sys_fork()
 * 

 */
int fork(void){
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res)   
		: "a"  (0x02));

	if (res < 0) { 
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper exit()
 * 
 * 0x01 es la posicion en sys_call_tables.S que corresponde a sys_exit()
 * 
 */
void exit(void){
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res)   
		: "a"  (0x01)    
		: "memory", "cc");

}



/* Wrapper get_stats()
 * 
 * 0x23 es la posicion en sys_call_tables.S que corresponde a sys_get_stats()
 * 
 */
int get_stats(int pid, struct stats *st)
{
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res),   
		  "+b" (pid),    
		  "+c" (st)   
		: "a"  (0x23)    
		: "memory", "cc");
	
    	if (res < 0) {
		errno = res * (-1); 
		return -1;
	}
	else return res;

}

/* Wrapper clone()
 *
 * sys_call_table.S position: 0x03
 * function: starting address of the function to be executed by the new process
 * stack   : starting address of a memory region to be used as a stack
 * returns: -1 if error or the pid of the new lightweight process ID if OK
 */
int clone(void (*function)(void), void *stack)
{
	int res;
	__asm__ volatile(
		"int $0x80"        
		: "=a" (res),   
		  "+b" (function),    
		  "+c" (stack)   
		: "a"  (0x03)    
		: "memory", "cc");
	
	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}


/* Wrapper read()
 *
 * 0x05 posicion en tabla.
 *
 */
int read (int fd, char *buf, int count)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (fd),
		  "+c" (buf),
		  "+d" (count)
		: "a"  (0x05)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper sem_init()
 *
 * 0x15 posicion en tabla.
 *
 */
int sem_init( int n_sem,unsigned int value)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (n_sem),
		  "+c" (value)
		: "a"  (0x15)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper sem_wait( int n_sem)
 *
 * 0x16 posicion en tabla.
 *
 */
int sem_wait( int n_sem)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (n_sem)
		: "a"  (0x16)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper sem_signal( int n_sem)
 *
 * 0x17 posicion en tabla.
 *
 */
int sem_signal( int n_sem)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (n_sem)
		: "a"  (0x17)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper sem_destroy(int n_sem)
 *
 * 0x21 posicion en tabla.
 *
 */
int sem_destroy( int n_sem)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (n_sem)
		: "a"  (0x18)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

/* Wrapper *sbrk(int increment)

 *
 * 0x07 posicion en tabla.
 *
 */
void *sbrk(int increment)
{
	int res;
	__asm__ volatile(
		"int $0x80"
		: "=a" (res),
		  "+b" (increment)
		: "a"  (0x07)
		: "memory", "cc");

	if (res < 0) {
		errno = res * (-1);
		return -1;
	}
	else return res;
}

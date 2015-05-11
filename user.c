#include <libc.h>

char buff[24];

int pid;

void print_ticks();
void perror_example();
void print_stats(int pid);
void fork_test();
void clone_test();

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	//int vector[100];
	char *a = (char *)sbrk(0x1200);
	a = sbrk(-0x1000);		
	//int *b = (int *)sbrk(0x100);
	while(1) {
		a[0] = 20;		
		
		a[0x1000] = 20;
		//a[0x1001] = 20;
		
		
		
		//b[0] = 20;
	}
}


void semaphore_test(){
	int a=fork();	
	if(a==0){
		char buffer[35] = "SOY EL HIJO\n";
		write(1,buffer,strlen(buffer));	
		a=fork();	
		if(a==0){
			sem_signal(1);
			sem_signal(1);
			sem_signal(1);
		}	
		int b = sem_wait(1);
		if(b < 0) perror();
		
		print_stats(getpid());
		while(1);
	}else{
		char buffer[35] = "SOY EL PADRE\n";
		write(1,buffer,strlen(buffer));	
		sem_init(1,0);
		sem_wait(1);	
		struct stats st;
		get_stats(getpid(),&st);
		while(st.cs==1)
			get_stats(getpid(),&st);
		
		sem_destroy(1);
		print_stats(getpid());
		while(1);
	}
}
void keyboard_test(){
	int res;
	char buffer[35] = "";
	res = read(0,buffer,33);
	if(res < 0) perror();
	write(1,buffer,res);	
	write(1, " \n", strlen(" \n"));
}

void clone_test(){
	char buffer[30] = "SOY EL THREAD CON ID: ";
	write(1,buffer,strlen(buffer));	
	itoa(getpid(), buffer);
	write(1,buffer,strlen(buffer));	
	print_stats(getpid());
	write(1, " \n", strlen(" \n"));	
	exit();	
}

void print_stats(int pid){
	struct stats st;
	char buffer[30];
	get_stats(pid,&st);
	
	itoa(pid, buffer);
	write(1, " ", strlen(" "));
	write(1,buffer,strlen(buffer));	
	write(1, " ", strlen(" "));

	itoa(st.tics, buffer);
	write(1, " ", strlen(" "));
	write(1,buffer,strlen(buffer));	
	write(1, " ", strlen(" "));

        itoa(st.cs, buffer);
	write(1,buffer,strlen(buffer));	
	write(1, " ", strlen(" "));

	itoa(st.remaining_quantum, buffer);
	write(1,buffer,strlen(buffer));	
	write(1, "\n", strlen("\n"));
}

                                                                                                                                   
void print_ticks()
{
	char buffer[30] = "holaholita vecinito\n";
	itoa(gettime(), buffer);
	write(1, buffer, strlen(buffer));
	write(1, "\n", strlen("\n"));
}

void perror_example()
{

	char * buffer;
	buffer = "hola holita vecinito"; //comentar esta linea para probar error de puntero nulo
	int res = write(1, buffer, 0); //cambiar valor de size: < 1 -> error
	if(res < 0) perror();	
}

void fork_test(){
	int a = fork();
	while(1) {	
		if(a!=0){
		
			char buffer[30] = "ID: ";
			write(1,buffer,strlen(buffer));	
			itoa(getpid(), buffer);
			write(1,buffer,strlen(buffer));	
			write(1, " ", strlen(" "));
			print_stats(getpid());
			a = fork();
			//if(a < 0) perror();
		
		}
		if(a==0){
			char buffer[30] = "SOY EL HIJO CON ID: ";
			write(1,buffer,strlen(buffer));	
			itoa(getpid(), buffer);
			write(1,buffer,strlen(buffer));	
	  		write(1, " \n", strlen(" \n"));			
			exit();

		}
	}
}

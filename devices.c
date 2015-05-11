#include <io.h>
#include <utils.h>
#include <list.h>
#include <cbuffer.h>
#include <sched.h>

// Blocked queue for this device
LIST_HEAD(blocked);
LIST_HEAD(keyboardqueue);
struct circular_buffer keybuffer;

//extern struct list_head keyboardqueue;
extern struct list_head readyqueue;

int sys_write_console(char *buffer, int size)
{
  int i;
  
  for (i = 0; i < size; i++)
    printc(buffer[i]);
  
  return size;
}

/*
 * Si keyboardqueue not empty -> block process
 * Sino
 * 		Si almenos <count> caracteres en keybuffer -> copiar <count> caracteres a buf y retornar.
 * 		Si el buffer esta lleno -> Buffer += keybuffer y bloquea al inicio.
 * 		Sino  
 */ 
int sys_read_keyboard(int fd, char *buf, int count)
{
	current()->characters_left = count;
	int n = 0;
	int i;
		
	if(!list_empty(&keyboardqueue)) {
		process_block(&keyboardqueue);
	}
	while(1) {
		if (cbCount(&keybuffer) >= (count -n )) {
			for (i = 0; i < (count - n); ++i)
				cbRead(&keybuffer, buf + (char)i + (char)n);
			return count;
		}
		else if(cbIsFull(&keybuffer)) {
			for (i = 0; i < BUFFER_SIZE; ++i){
				printc_xy(0x3f, 0x15, ' ');
				cbRead(&keybuffer, buf + (char)i + (char)n);
			}
			n += BUFFER_SIZE;
			current()->characters_left -= BUFFER_SIZE;
			process_block_at_begining(&keyboardqueue);
		}
		else {
			process_block_at_begining(&keyboardqueue);
		}	
	}
	return -1;
}	

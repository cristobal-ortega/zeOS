/*
 * interrupt.c -
 */

#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <sched.h>
#include <cbuffer.h>


#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

void clockHandler(void);
void keyboardHandler(void);

int zeos_ticks = 0;

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head keyboardqueue;

extern struct circular_buffer keybuffer;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setIdt()
{
	/* Program interrups/exception service routines */
	idtR.base  = (DWord)idt;
	idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
	/* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
	set_handlers();
	set_idt_reg(&idtR);

	setInterruptHandler(32, clockHandler, 0);
	setInterruptHandler(33, keyboardHandler, 0);
}


void RSI_CLOCK() {
	++zeos_ticks;
	schedule();
	zeos_show_clock();
}

/*
 * Insert the new character inside the circular buffer.
 *
	If the buffer is full and there are no processes waiting for data, then the new character is lost.
	Otherwise, 1) either the buffer is full and a process is waiting, or 2) there are enough characters
	in the buffer to satisfy the last read operation, this routine must unblock the first process of
	the keyboardqueue.
 */

void RSI_KEYBOARD(){
	unsigned char key = inb(0x60);
	unsigned char mask = 0b10000000;
	unsigned char make = key & mask;
	char c = '0';
	if(make == 0) {
		mask = 0b01111111;
		c = char_map[key & mask];
		if(!(c>=0x20 && c<=0x7E))
			c = 'C';

		cbWrite(&keybuffer, &c);

		//MIRAR
		if (!(cbIsFull(&keybuffer) && list_empty(&keyboardqueue))) {
		
			if(!list_empty(&keyboardqueue)) {
				struct list_head * to_unblock;
				struct task_struct * pcb_to_unlock;
				
				to_unblock = list_first(&keyboardqueue);
				pcb_to_unlock = list_head_to_task_struct(to_unblock);
				
				if ((cbIsFull(&keybuffer) || pcb_to_unlock->characters_left <= cbCount(&keybuffer))) {
						list_del(to_unblock);
						list_add_tail(to_unblock, &readyqueue);
				}
			}
		}
	}
	else
		c = ' ';
		
	printc_xy(0x3f, 0x15, c);
}

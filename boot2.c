//Author: Adam Harms
//Date: 1.25.2017
//CSC 4100 - Operating Systems
//Mike Rogers
//boot2.c is the c driver for our first project
#include "idt.h"
#include "scan_map.h"
#include "vterm_queue.h"
#include "queue.h"
#include "pcb.h"
#include "util.h"

#define BUFFER_MAX 128
#define STACK_SIZE 1024

void ruKeyBoard();

//Program 1 Forward Declarations
void show_eax();
void clearScr();
void writeScr(char *string, int row, int col);

//Program 2 Functions/Declarations Below
void initIDT();
void setupPIC();
char translate(unsigned int scancode);
void initIDTEntry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access);
void defaultHandler();
char k_getchar();

//Assembly Declarations
void kbd_enter();
void lidtr(gdt_r_t idtr);
void outportb(uint16_t p,uint8_t o);

idt_entry_t idt[256];//Global variable
gdt_r_t idtr;//Pointer to IDT
int savecol[25];

//BUFFER QUEUE
char kbd_buffer[BUFFER_MAX];
int tail = 0;
int head = 0;
int count = 0;
int specialKeys[1] = {-1};//Just watch shift for now

//Program 3 Functions/Declarations Below
void p1();
void p2();
int* allocStack();
int createProcess(uint32_t ds, uint32_t ss, uint32_t topOfStack, uint32_t cs, uint32_t processEntry);
void writeln(char* line);
void clearscr_box(int row,int col,int row2,int col2);
void set_cursor(int r, int c);
int gets(char* string,int length);

//Assembly Declarations
void schedule();
void go();
void init_timer_dev(int ms);

int stacks[3][STACK_SIZE];
int nextStack = 0;
int timer = 50;
int num_process = 0;

int row = 0;
int col = 0;

//Program 4 Functions/Declarations Below
uint32_t pushf_cli_fun();
void popf_fun(uint32_t eflags);
void vterm_foreground_next();
void printProcessID();
void vterm_block_if_background();
void sched_fun();
void printNumber(char* num);
int stoi(char *s);
int is_prime(int n);
int cur_vterm;

int main()
{
	clearScr();
	//writeScr("Running two processes",0,0);
	initIDT();	//setup IDT
	
	setupPIC();	//setup PIC
	init_timer_dev(timer);

	//initialize queue for use
	queues.head = 0;
	queues.tail = 0;
	vterm_queues.head = 0;
	vterm_queues.tail = 0;
	vterm_queue_count = 0;

	int *s = allocStack();
	createProcess((uint32_t) 8, (uint32_t) 24, (uint32_t) (s + STACK_SIZE), (uint32_t) 16, (uint32_t) p1);

	s = allocStack();
	createProcess((uint32_t) 8, (uint32_t) 24, (uint32_t) (s + STACK_SIZE), (uint32_t) 16, (uint32_t) p2);

	//currPCB = vterm_dequeue();
	//writeScr("here",currPCB->pid,0);
	cur_vterm = 1;//&(queues.pcbs[queues.head]);
	printProcessID();
	go();
	while(1);
	return 0;
}

void runKeyBoard()
{
	asm("sti");	//Start Interrupts
	int row = 0;
	int col = 0;
	char key[2];
	while(1)
	{
		int position = (row*80) + col;
		outportb(0x3D4, 0x0F);
		outportb(0x3D5, (unsigned char)(position));
		outportb(0x3D4, 0x0E);
		outportb(0x3D5, (unsigned char)(position>>8));

		key[0] = k_getchar();
		key[1] = 0;
		if(key[0] != 0)
		{
			if(key[0] == keys[BACKSPACE])
			{
				//check if you go off screen to the left
				if(col-1 < 0)
				{
					if(row-1 >= 0)
					{
						row--;
						col = savecol[row];
					}
				}
				else col --;

				key[0] = ' ';
				writeScr(key,row,col);
				
			}
			else if(key[0] == keys[TAB])
			{
				col += 9;
			}
			else if(key[0] == keys[ENTER])
			{
				savecol[row] = col;
				row++;
				col = 0;
			}
			else
			{
				writeScr(key,row,col);
				col++;
			}

			if(col > 80)
			{
				row ++;
				col = col%80;
			}
			if(row >= 25)
			{
				clearScr();
				row = 0;
			}
		}
	}
}

void initIDT()
{
	uint8_t access = 0x8e;
	uint16_t selector = 16;
	int a = 0;
	//For entries 0 to 31, setting these entries to point to the default handler
	for(a=0;a<32;a++)
	{
		initIDTEntry(&idt[a],(uint32_t)defaultHandler,selector,access);
	}

	//for entry 32, setting it to point to 0 (Timer Device For Next Assignment)
	initIDTEntry(&idt[32],(uint32_t)schedule,selector,access);

	//for entry 33, setting it to point to the keyboard handler (kbd_enter())
	initIDTEntry(&idt[33],(uint32_t)kbd_enter,selector,access);

	//for entries 34 to 255, setting these entries to point to 0
	for(a=34;a<256;a++)
	{
		initIDTEntry(&idt[a],0,0,0);
	}

	//call lidtr(idtr);
	idtr.limit = sizeof(idt) - 1;
	idtr.base = (uint32_t)idt;
	lidtr(idtr);
}

void setupPIC()
{
	// set up cascading mode:
	outportb(0x20, 0x11); // start 8259 master initialization
	outportb(0xA0, 0x11); // start 8259 slave initialization
	outportb(0x21, 0x20); // set master base interrupt vector (idt 32-38)
	outportb(0xA1, 0x28); // set slave base interrupt vector (idt 39-45)
	// Tell the master that he has a slave:
	outportb(0x21, 0x04); // set cascade ...
	outportb(0xA1, 0x02); // on IRQ2
	// Enabled 8086 mode:
	outportb(0x21, 0x01); // finish 8259 initialization
	outportb(0xA1, 0x01);
	// Reset the IRQ masks
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
	// Now, enable the clock IRQ only
	outportb(0x21, 0xfc); // Turn on the clock IRQ and the keyboard IRQ
	//outportb(0x21, 0xfd); // Turn on the keyboard IRQ
	outportb(0xA1, 0xff); // Turn off all others
}

void kbd_handler(unsigned int scancode)
{
	if(scancode == 0 || count == BUFFER_MAX)
	{
		return;
	}

	if(scancode == 0x3B)//F1
	{
		vterm_foreground_next();
		return;
	}

	char ch = translate(scancode);
	if(ch == 0) return;

	//Add to the queue
	kbd_buffer[tail] = ch;
	count++;
	tail = ++tail%BUFFER_MAX;
}

char translate(unsigned int scancode)
{
	switch(scancode)
	{
		case LSHIFT + 0x80:
		case LSHIFT:
			specialKeys[0] *= -1;
			return 0;
		case RSHIFT + 0x80:
		case RSHIFT:
			specialKeys[0] *= -1;
			return 0;

		case ESC:
		case CTRL:
		case PRT_SCR:
		case ALT:
		case CAPS_LOCK:
			return 0;
		default:
			if(scancode > CAPS_LOCK) return 0;
			if(specialKeys[0] > 0)
				return skeys[scancode];
			else
				return keys[scancode];
	}
}

char k_getchar()
{
	if(count <= 0) return 0;

	char key = kbd_buffer[head];
	head = ++head%BUFFER_MAX;
	count--;

	return key;
}

//Single Entry
void initIDTEntry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access)
{
	//base_hi16 access always0 selector base_low16
	//63.....48 47..40 39...32 31....16 16.......0

	//setting up base address
	entry->base_low16 = (uint16_t)base; //set up low 16 bits
	entry->base_hi16 = (uint16_t)(base >> 16); //set up high 16 bits

	entry->selector = selector; //set up selector
	entry->always0 = 0; //set always0 to 0
	entry->access = access; //set up access
}

//default handler
void defaultHandler()
{
	char msg[] = "Default Handler Exception";
	writeScr(msg,24,0);
	while(1);
}

int convert_num_h(unsigned int num, char buf[])
{
	if(num == 0)
	{
		return 0;
	}
	int idx = convert_num_h(num / 10, buf);
	buf[idx] = num % 10 + '0';
	buf[idx+1] = '\0';
	return idx + 1;
}

void convert_num(unsigned int num, char buf[])
{
	if(num == 0)
	{
		buf[0] = '0';
		buf[1] = '\0';
	}
	else
	{
		convert_num_h(num, buf);
	}
}

//Program 3 Functions Below
void p1()
{
	while(1)
	{
		currPCB->row = 0;
   		writeln("Please enter a string less than 20 characters:");
		char s[20];
    		char t[20];
    		gets(s, 20);
		currPCB->row++;
    		writeln("You entered: ");
		writeScr(s,currPCB->row-1,13);
    		writeln("Hit enter to continue...");
    		gets(t, 20);
    		clearscr_box(1,0,9,79);
	}
}

void p2()
{
	while(1)
	{
		currPCB->row = 12;
		writeln("Please enter a number:");
		char s[10];
    		char t[20];
	
		gets(s, 10);

		char num[10];
		
		
		int a;
		for(a=0;a<10;a++)
		{
			if(s[a] != ' ')
				num[a] = s[a];
		}

		printNumber(num);
    		writeln("Hit enter to continue...");
    		gets(t, 20);
    		clearscr_box(13,0,25,79);
	}
}

void printNumber(char* num)
{
	int value = stoi(num);
	currPCB->row++;
	writeln("The number of primes is:");

	int primes = 0;
	int i;
	for(i=2;i<=value;i++)
		if(is_prime(i))
			primes++;

	char buff[10];
	itoa(primes,buff,10);
	writeScr(buff,currPCB->row-1,25);
}

int is_prime(int n)
{
	int i;
	if(n == 2)
		return 1;
	for(i=2;i<n-1;i++)
	{
		if(n%i == 0)
			return 0;
	}	
  	return 1;
}

int stoi(char *s)
{
	int i = 0;
	while(*s)
	{
		if(*s >= '0' && *s <= '9')
		{
			i *= 10;
			i += (*s++ - '0');
		}
		else
		{
			i = 0;
			break;
		}
	}
	return i;
}

/*void oldp2()
{
	int i = 0;
	char* snum;
	while(1)
	{
		itoa(i,snum,10);
		char* msg = "process p2:  ";
		writeScr(msg,15,0);
		writeScr(snum,15,12);
		i = ++i%10;
	}
	while(1);
}*/

int gets(char* string,int length)
{
	int index = 0;
	int a = 0;
	for(a=0;a<length;a++)
		string[a] = ' ';
	string[length-1] = 0;
	char key[2];
	key[1] = 0;
	while(1)
	{
		int eflags = pushf_cli_fun();
		vterm_block_if_background();
		key[0] = k_getchar();
		popf_fun(eflags);
		if(key[0] != 0)
		{
			if(key[0] == keys[BACKSPACE])
			{
				index--;
				if(index < 0)
					index = 0;
				key[0] = ' ';
				string[index] = key[0];
				writeScr(key,currPCB->row,index);
				set_cursor(currPCB->row,index);
			}
			else if(key[0] == keys[TAB])
			{
				index += 9;
			}
			else if(key[0] == keys[ENTER] || index >= length)
			{
				string[index] = 0;
				return index;
			}
			else
			{
				writeScr(key,currPCB->row,index);
				set_cursor(currPCB->row,index+1);
				string[index] = key[0];
				index++;
			}
		}
	}
}

void set_cursor(int r, int c)
{
	currPCB->row = r;
	currPCB->col = c;
	int position = (currPCB->row*80) + currPCB->col;
	outportb(0x3D4, 0x0F);
	outportb(0x3D5, (unsigned char)(position));
	outportb(0x3D4, 0x0E);
	outportb(0x3D5, (unsigned char)(position>>8));
}

void clearscr_box(int row,int col,int row2,int col2)
{
	int a = 0;
	int b = 0;
	for(a = row;a<row2;a++)
		for(b=col;b<col2;b++)
			writeScr(" ",a,b);
}

void writeln(char* line)
{
	writeScr(line,currPCB->row,0);
	currPCB->row++;
	set_cursor(currPCB->row, 0);
}

int* allocStack()
{
	return stacks[nextStack++];
}

int createProcess(uint32_t ds, uint32_t ss, uint32_t stackTop, uint32_t cs, uint32_t processEntry)
{
	uint32_t *st = (uint32_t *)stackTop;
	st--;
	*st = 0x0200;
	st--;
	*st = cs;
	st--;
	*st = processEntry;
	st--;
	*st = 0;    //ebp
	st--;
	*st = 0;    //esp
	st--;
	*st = 0;    //edi
	st--;
	*st = 0;    //esi
	st--;
	*st = 0;    //edx
	st--;
	*st = 0;    //ecx
	st--;
	*st = 0;    //ebx
	st--;
	*st = 0;    //eax
	st--;
	*st = ds;   //ds
	st--;
	*st = ds;   //es
	st--;
	*st = ds;   //fs
	st--;
	*st = ds;   //gs

	pcb_t *pcb = allocatePCB();
	pcb->esp = (uint32_t)st;
	num_process++;
	pcb->pid = num_process;
	pcb->row = 0;
	pcb->col = 0;
	enqueue(pcb);//add pointer to queue
}

void vterm_foreground_next()
{
	cur_vterm = (currPCB->pid)%num_process;
	cur_vterm++;
	if(vterm_queue_count > 0)
	{
		pcb_t* process = vterm_dequeue();
		enqueue(process);//On Ready Queue
	}
	printProcessID();
}

void printProcessID()
{
	char* pid;
	itoa(cur_vterm,pid,10);
	writeScr("[P",0,76);
	writeScr(pid,0,78);
	writeScr("]",0,79);
}

void vterm_block_if_background()
{
	while(currPCB->pid != cur_vterm)
	{
		if(vterm_queue_count > 0)
		{
			pcb_t* process = vterm_dequeue();
			enqueue(process);//On Ready Queue
		}
		sched_fun();
	}
	set_cursor(currPCB->row,currPCB->col);
}


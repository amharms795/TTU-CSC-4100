#ifndef PCB_H
#define PCB_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct pcb
{
	uint32_t esp;
	uint32_t pid;
	uint32_t row;
	uint32_t col;
};
typedef struct pcb pcb_t;

pcb_t *currPCB;
int pcb_next = 0;

#endif

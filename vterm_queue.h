#ifndef VTERM_QUEUE
#define VTERM_QUEUE

#include "pcb.h"

#define VTERM_QUEUE_SIZE 100

struct vterm_queue
{
    uint32_t head;
    uint32_t tail;
    pcb_t pcbs[VTERM_QUEUE_SIZE];
};
typedef struct vterm_queue vterm_queue_t;

vterm_queue_t vterm_queues;
int vterm_queue_count;

pcb_t* vterm_allocatePCB()
{
	pcb_next++;
	return &(vterm_queues.pcbs[pcb_next-1]);
}

void vterm_enqueue(pcb_t *pcb)
{
	vterm_queues.pcbs[vterm_queues.tail] = *pcb;
	vterm_queues.tail = ++vterm_queues.tail%VTERM_QUEUE_SIZE;
	vterm_queue_count++;
}

pcb_t* vterm_dequeue()
{
	vterm_queue_count--;
	int head = vterm_queues.head;
	vterm_queues.head = ++vterm_queues.head%VTERM_QUEUE_SIZE;
	return &(vterm_queues.pcbs[head]);
}
#endif

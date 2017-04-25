#ifndef QUEUE
#define QUEUE

#include "pcb.h"

#define QUEUE_SIZE 100

struct queue
{
    uint32_t head;
    uint32_t tail;
    pcb_t pcbs[QUEUE_SIZE];
};
typedef struct queue queue_t;

queue_t queues;

pcb_t* allocatePCB()
{
	pcb_next++;
	return &(queues.pcbs[pcb_next-1]);
}

void enqueue(pcb_t *pcb)
{
	queues.pcbs[queues.tail] = *pcb;
	queues.tail = ++queues.tail%QUEUE_SIZE;
}

pcb_t* dequeue()
{
	int head = queues.head;
	queues.head = ++queues.head%QUEUE_SIZE;
	return &(queues.pcbs[head]);
}
#endif

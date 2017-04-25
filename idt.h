//Author: Adam Harms
//Date: 3.12.2017
//CSC 4100 - Operating Systems
//Mike Rogers
//idt.h is the header for the IDC

#ifndef IDT_H
#define IDT_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

//This data structure represents an interrupt descriptor
struct idt_entry
{
	uint16_t base_low16;
	uint16_t selector;
	uint8_t always0;
	uint8_t access;
	uint16_t base_hi16;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct gdt_r_s
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));
typedef struct gdt_r_s gdt_r_t;

#endif

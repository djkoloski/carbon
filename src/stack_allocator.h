#pragma once

#include "util.h"

#include <stdio.h>

typedef struct StackAllocation StackAllocation;
typedef struct StackAllocator StackAllocator;
typedef size_t (*StackAllocatorGetNextCapacityFunc)(const StackAllocator *, size_t);

struct StackAllocation
{
	size_t size;
	size_t capacity;
	StackAllocation *prev;
	StackAllocation *next;
};

void *StackAllocation_GetData(const StackAllocation *allocation);

struct StackAllocator
{
	StackAllocation *head;
	StackAllocation *tail;
	size_t size;
	StackAllocatorGetNextCapacityFunc getNextCapacity;
};

void StackAllocator_Create(StackAllocator *allocator, StackAllocatorGetNextCapacityFunc getNextCapacity);
void StackAllocator_Destroy(StackAllocator *allocator);
void *StackAllocator_Allocate(StackAllocator *allocator, size_t requestSize);
void StackAllocator_Free(StackAllocator *allocator, size_t requestSize);
size_t StackAllocator_DefaultGetNextCapacity(const StackAllocator *allocator, size_t requestSize);
size_t StackAllocator_Write(StackAllocator *allocator, FILE *output);
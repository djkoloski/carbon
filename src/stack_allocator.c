#include "stack_allocator.h"

#include <stdlib.h>
#include <string.h>

void *StackAllocation_GetData(const StackAllocation *allocation)
{
	return (char *)allocation + sizeof(StackAllocation);
}

void StackAllocator_Create(StackAllocator *allocator, StackAllocatorGetNextCapacityFunc getNextCapacity)
{
	allocator->head = NULL;
	allocator->tail = NULL;
	allocator->size = 0;
	allocator->getNextCapacity = getNextCapacity;
}
void StackAllocator_Destroy(StackAllocator *allocator)
{
	StackAllocation *head = allocator->head;
	while (head)
	{
		void *data = head;
		head = head->next;
		free(data);
	}
}
void *StackAllocator_Allocate(StackAllocator *allocator, size_t requestSize)
{
	allocator->size += requestSize;
	
	/* Check if last allocation has room */
	if (allocator->tail &&
		allocator->tail->capacity - allocator->tail->size >= requestSize)
	{
		void *result = (char *)StackAllocation_GetData(allocator->tail) + allocator->tail->size;
		allocator->tail->size += requestSize;
		return result;
	}
	
	/* Make a new allocation */
	size_t nextCapacity = allocator->getNextCapacity(allocator, sizeof(StackAllocation) + requestSize);
	assert(nextCapacity >= sizeof(StackAllocation) + requestSize);
	
	StackAllocation *allocation = malloc(sizeof(StackAllocation) + nextCapacity);
	allocation->size = requestSize;
	allocation->capacity = nextCapacity - sizeof(StackAllocator);
	allocation->next = NULL;
	
	if (!allocator->tail)
	{
		allocation->prev = NULL;
		allocator->head = allocation;
		allocator->tail = allocation;
	}
	else
	{
		allocator->tail->next = allocation;
		allocation->prev = allocator->tail;
		allocator->tail = allocation;
	}
	
	return StackAllocation_GetData(allocation);
}
void StackAllocator_Free(StackAllocator *allocator, size_t requestSize)
{
	allocator->size -= requestSize;
	allocator->tail->size -= requestSize;
	
	if (allocator->tail->size == 0)
	{
		StackAllocation *prev = allocator->tail->prev;
		free(allocator->tail);
		if (prev)
		{
			allocator->tail = prev;
		}
		else
		{
			allocator->head = NULL;
			allocator->tail = NULL;
		}
	}
}
size_t StackAllocator_DefaultGetNextCapacity(const StackAllocator *allocator, size_t requestSize)
{
	/* 1KB */
	const size_t k_defaultCapacity = 1024;
	
	size_t result = k_defaultCapacity;
	if (allocator->tail)
	{
		result = allocator->tail->capacity;
		result += result / 2;
	}
	
	if (result < requestSize)
	{
		result = requestSize;
	}
	
	return result;
}
size_t StackAllocator_Write(StackAllocator *allocator, FILE *output)
{
	size_t result = 0;
	for (StackAllocation *allocation = allocator->head; allocation; allocation = allocation->next)
	{
		result += allocation->size;
		fwrite(StackAllocation_GetData(allocation), 1, allocation->size, output);
	}
	return result;
}
#pragma once

#include "stack_allocator.h"

typedef struct HashTableEntry HashTableEntry;
typedef struct HashTable HashTable;

struct HashTableEntry
{
	const void *key;
	void *value;
	HashTableEntry *next;
};
struct HashTable
{
	StackAllocator allocator;
	size_t size;
	size_t capacity;
	HashTableEntry **entries;
	float loadFactor;
	size_t (*hash)(const void *);
	bool (*compare)(const void *, const void *);
};

void HashTable_Create(HashTable *hashTable, size_t initialCapacity, float loadFactor, size_t (*hash)(const void *), bool (*compare)(const void *, const void *));
void HashTable_Destroy(HashTable *hashTable);
void HashTable_Insert(HashTable *hashTable, const void *key, void *value);
void **HashTable_Find(const HashTable *hashTable, const void *key);
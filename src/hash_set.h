#pragma once

#include "stack_allocator.h"

typedef struct HashSetEntry HashSetEntry;
typedef struct HashSet HashSet;

struct HashSetEntry
{
	const void *key;
	HashSetEntry *next;
};
struct HashSet
{
	StackAllocator allocator;
	size_t size;
	size_t capacity;
	HashSetEntry **entries;
	float loadFactor;
	size_t (*hash)(const void *);
	bool (*compare)(const void *, const void *);
};

void HashSet_Create(HashSet *hashSet, size_t initialCapacity, float loadFactor, size_t (*hash)(const void *), bool (*compare)(const void *, const void *));
void HashSet_Destroy(HashSet *hashSet);
void HashSet_Insert(HashSet *hashSet, const void *key);
const void *HashSet_Find(const HashSet *hashSet, const void *key);
void HashSet_ToArray(const HashSet *hashSet, const void **array);
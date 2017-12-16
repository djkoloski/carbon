#include "hash_set.h"

#include <string.h>

void HashSet_Create(HashSet *hashSet, size_t initialCapacity, float loadFactor, size_t (*hash)(const void *), bool (*compare)(const void *, const void *))
{
	StackAllocator_Create(&hashSet->allocator, StackAllocator_DefaultGetNextCapacity);
	hashSet->size = 0;
	hashSet->capacity = initialCapacity;
	hashSet->entries = StackAllocator_Allocate(&hashSet->allocator, initialCapacity * sizeof(HashSetEntry *));
	memset(hashSet->entries, 0, initialCapacity * sizeof(HashSetEntry *));
	hashSet->loadFactor = loadFactor;
	hashSet->hash = hash;
	hashSet->compare = compare;
}
void HashSet_Destroy(HashSet *hashSet)
{
	StackAllocator_Destroy(&hashSet->allocator);
}
static void HashSet_Resize(HashSet *hashSet, size_t newCapacity)
{
	StackAllocator newAllocator;
	StackAllocator_Create(&newAllocator, StackAllocator_DefaultGetNextCapacity);
	
	HashSetEntry **newEntries = StackAllocator_Allocate(&newAllocator, newCapacity * sizeof(HashSetEntry *));
	for (size_t i = 0; i < hashSet->capacity; ++i)
	{
		HashSetEntry *entry = hashSet->entries[i];
		while (entry)
		{
			size_t newIndex = hashSet->hash(entry->key) % newCapacity;
			
			HashSetEntry *newEntry = StackAllocator_Allocate(&newAllocator, sizeof(HashSetEntry));
			newEntry->key = entry->key;
			
			HashSetEntry *head = newEntries[newIndex];
			if (!head)
			{
				newEntries[newIndex] = newEntry;
			}
			else
			{
				while (head->next)
				{
					head = head->next;
				}
				head->next = newEntry;
			}
			
			entry = entry->next;
		}
	}
	
	StackAllocator_Destroy(&hashSet->allocator);
	hashSet->allocator = newAllocator;
	hashSet->capacity = newCapacity;
	hashSet->entries = newEntries;
}
void HashSet_Insert(HashSet *hashSet, const void *key)
{
	++hashSet->size;
	
	float currentLoadFactor = (float)hashSet->size / (float)hashSet->capacity;
	if (currentLoadFactor > hashSet->loadFactor)
	{
		HashSet_Resize(hashSet, hashSet->size + hashSet->size / 2);
	}
	
	HashSetEntry *newEntry = StackAllocator_Allocate(&hashSet->allocator, sizeof(HashSetEntry));
	newEntry->key = key;
	newEntry->next = NULL;
	
	size_t index = hashSet->hash(key) % hashSet->capacity;
	HashSetEntry *head = hashSet->entries[index];
	if (!head)
	{
		hashSet->entries[index] = newEntry;
	}
	else
	{
		while (head->next)
		{
			head = head->next;
		}
		head->next = newEntry;
	}
}
const void *HashSet_Find(const HashSet *hashSet, const void *key)
{
	size_t index = hashSet->hash(key) % hashSet->capacity;
	HashSetEntry *entry = hashSet->entries[index];
	while (entry)
	{
		if (hashSet->compare(entry->key, key))
		{
			return entry->key;
		}
		entry = entry->next;
	}
	return NULL;
}
void HashSet_ToArray(const HashSet *hashSet, const void **array)
{
	for (size_t i = 0, j = 0; i < hashSet->capacity; ++i)
	{
		HashSetEntry *entry = hashSet->entries[i];
		while (entry)
		{
			array[j++] = entry->key;
			entry = entry->next;
		}
	}
}
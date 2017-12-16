#include "hash_table.h"

#include <string.h>

void HashTable_Create(HashTable *hashTable, size_t initialCapacity, float loadFactor, size_t (*hash)(const void *), bool (*compare)(const void *, const void *))
{
	StackAllocator_Create(&hashTable->allocator, StackAllocator_DefaultGetNextCapacity);
	hashTable->size = 0;
	hashTable->capacity = initialCapacity;
	hashTable->entries = StackAllocator_Allocate(&hashTable->allocator, initialCapacity * sizeof(HashTableEntry *));
	memset(hashTable->entries, 0, initialCapacity * sizeof(HashTableEntry *));
	hashTable->loadFactor = loadFactor;
	hashTable->hash = hash;
	hashTable->compare = compare;
}
void HashTable_Destroy(HashTable *hashTable)
{
	StackAllocator_Destroy(&hashTable->allocator);
}
static void HashTable_Resize(HashTable *hashTable, size_t newCapacity)
{
	StackAllocator newAllocator;
	StackAllocator_Create(&newAllocator, StackAllocator_DefaultGetNextCapacity);
	
	HashTableEntry **newEntries = StackAllocator_Allocate(&newAllocator, newCapacity * sizeof(HashTableEntry *));
	for (size_t i = 0; i < hashTable->capacity; ++i)
	{
		HashTableEntry *entry = hashTable->entries[i];
		while (entry)
		{
			size_t newIndex = hashTable->hash(entry->key) % newCapacity;
			
			HashTableEntry *newEntry = StackAllocator_Allocate(&newAllocator, sizeof(HashTableEntry));
			newEntry->key = entry->key;
			newEntry->value = entry->value;
			
			HashTableEntry *head = newEntries[newIndex];
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
	
	StackAllocator_Destroy(&hashTable->allocator);
	hashTable->allocator = newAllocator;
	hashTable->capacity = newCapacity;
	hashTable->entries = newEntries;
}
void HashTable_Insert(HashTable *hashTable, const void *key, void *value)
{
	++hashTable->size;
	
	float currentLoadFactor = (float)hashTable->size / (float)hashTable->capacity;
	if (currentLoadFactor > hashTable->loadFactor)
	{
		HashTable_Resize(hashTable, hashTable->size + hashTable->size / 2);
	}
	
	HashTableEntry *newEntry = StackAllocator_Allocate(&hashTable->allocator, sizeof(HashTableEntry));
	newEntry->key = key;
	newEntry->value = value;
	newEntry->next = NULL;
	
	size_t index = hashTable->hash(key) % hashTable->capacity;
	HashTableEntry *head = hashTable->entries[index];
	if (!head)
	{
		hashTable->entries[index] = newEntry;
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
void **HashTable_Find(const HashTable *hashTable, const void *key)
{
	size_t index = hashTable->hash(key) % hashTable->capacity;
	HashTableEntry *entry = hashTable->entries[index];
	while (entry)
	{
		if (hashTable->compare(entry->key, key))
		{
			return &entry->value;
		}
		entry = entry->next;
	}
	return NULL;
}
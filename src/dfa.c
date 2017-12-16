#include "dfa.h"

#include "hash_set.h"
#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

void DFAState_Initialize(DFAState *state)
{
	memset(state, 0, sizeof(DFAState));
}

void DFA_Create(DFA *dfa)
{
	StackAllocator_Create(&dfa->allocator, StackAllocator_DefaultGetNextCapacity);
	Vector_Create(&dfa->states, 16);
}
void DFA_Destroy(DFA *dfa)
{
	StackAllocator_Destroy(&dfa->allocator);
	Vector_Destroy(&dfa->states);
}
DFAState *DFA_AddState(DFA *dfa)
{
	DFAState *newState = StackAllocator_Allocate(&dfa->allocator, sizeof(DFAState));
	DFAState_Initialize(newState);
	Vector_Push(&dfa->states, newState);
	return newState;
}
static void DFA_BuildEtaSet(NFAState *state, HashSet *set)
{
	if (HashSet_Find(set, state))
	{
		return;
	}
	
	HashSet_Insert(set, state);
	if (state->left.state && NFAEdgeConditions_Get(&state->left.conditions, 0))
	{
		DFA_BuildEtaSet(state->left.state, set);
	}
	if (state->right.state && NFAEdgeConditions_Get(&state->right.conditions, 0))
	{
		DFA_BuildEtaSet(state->right.state, set);
	}
}
static void DFA_ComputeClosure(Vector *edge, HashSet *set)
{
	for (size_t i = 0; i < edge->size; ++i)
	{
		DFA_BuildEtaSet((NFAState *)Vector_Get(edge, i), set);
	}
}
typedef struct DFAStateKey
{
	size_t size;
	NFAState **states;
} DFAStateKey;
static size_t DFA_HashStateKey(const void *data)
{
	const DFAStateKey *key = data;
	size_t hash = 17;
	for (size_t i = 0; i < key->size; ++i)
	{
		hash = hash * 37 + (size_t)key->states[i] * 2654435761;
	}
	return hash;
}
static bool DFA_CompareStateKey(const void *lhs, const void *rhs)
{
	const DFAStateKey *first = lhs;
	const DFAStateKey *second = rhs;
	
	if (first->size != second->size)
	{
		return false;
	}
	
	for (size_t i = 0; i < first->size; ++i)
	{
		if (first->states[i] != second->states[i])
		{
			return false;
		}
	}
	
	return true;
}
static size_t DFA_HashNFAState(const void *data)
{
	return (size_t)data * 2654435761;
}
static bool DFA_CompareNFAState(const void *lhs, const void *rhs)
{
	return lhs == rhs;
}
static int DFA_SortNFAState(const void *lhs, const void *rhs)
{
	NFAState *a = *(NFAState **)lhs;
	NFAState *b = *(NFAState **)rhs;
	
	if (a < b)
	{
		return -1;
	}
	else if (a > b)
	{
		return 1;
	}
	return 0;
}
static DFAState *DFA_FromEntriesRecurse(DFA *dfa, DFAEntry *entries, size_t entriesSize, StackAllocator *allocator, HashTable *stateKeyToDFAState, Vector *edge)
{
	/* Compute closure */
	HashSet closure;
	HashSet_Create(&closure, 16, 0.75f, DFA_HashNFAState, DFA_CompareNFAState);
	DFA_ComputeClosure(edge, &closure);
	
	/* Make key */
	DFAStateKey *key = StackAllocator_Allocate(allocator, sizeof(DFAStateKey));
	key->size = closure.size;
	key->states = StackAllocator_Allocate(allocator, key->size * sizeof(NFAState *));
	HashSet_ToArray(&closure, key->states);
	qsort(key->states, key->size, sizeof(NFAState *), DFA_SortNFAState);
	
	/* Look up state */
	DFAState *state = NULL;
	{
		void **statePtr = HashTable_Find(stateKeyToDFAState, key);
		if (statePtr)
		{
			state = *statePtr;
		}
	}
	
	if (!state)
	{
		/* Insert new state */
		state = DFA_AddState(dfa);
		DFAState_Initialize(state);
		HashTable_Insert(stateKeyToDFAState, key, state);
		
		/* Determine symbol */
		for (size_t i = 0; i < entriesSize; ++i)
		{
			if (HashSet_Find(&closure, entries[i].expression.end))
			{
				state->symbol = entries[i].symbol;
				break;
			}
		}

		/* Collect transitions */
		NFAEdgeConditions conditions;
		memset(&conditions, 0, sizeof(NFAEdgeConditions));
		for (size_t i = 0; i < key->size; ++i)
		{
			NFAEdgeConditions_Or(&conditions, &key->states[i]->left.conditions);
			NFAEdgeConditions_Or(&conditions, &key->states[i]->right.conditions);
		}
		
		/* Perform transitions */
		for (int c = 1; c < DFASTATE_EDGES_MAX; ++c)
		{
			if (NFAEdgeConditions_Get(&conditions, c))
			{
				/* Build edge */
				Vector_Clear(edge);
				for (size_t i = 0; i < key->size; ++i)
				{
					if (NFAEdgeConditions_Get(&key->states[i]->left.conditions, c))
					{
						Vector_Push(edge, key->states[i]->left.state);
					}
					if (NFAEdgeConditions_Get(&key->states[i]->right.conditions, c))
					{
						Vector_Push(edge, key->states[i]->right.state);
					}
				}
				
				/* Recurse */
				state->edges[c] = DFA_FromEntriesRecurse(dfa, entries, entriesSize, allocator, stateKeyToDFAState, edge);
			}
		}
	}
	else
	{
		/* Release key */
		StackAllocator_Free(allocator, key->size * sizeof(NFAState *));
		StackAllocator_Free(allocator, sizeof(DFAStateKey));
	}

	/* Clean up */
	HashSet_Destroy(&closure);
	
	return state;
}
DFAState *DFA_FromEntries(DFA *dfa, DFAEntry *entries, size_t entriesSize)
{
	/* Allocator */
	StackAllocator allocator;
	StackAllocator_Create(&allocator, StackAllocator_DefaultGetNextCapacity);
	
	/* Hash table from DFAStateKey * to DFAState * */
	HashTable stateKeyToDFAState;
	HashTable_Create(&stateKeyToDFAState, 16, 0.75f, DFA_HashStateKey, DFA_CompareStateKey);
	
	/* Vector of NFAState * */
	Vector edge;
	Vector_Create(&edge, 16);
	
	/* Initial States */
	for (size_t i = 0; i < entriesSize; ++i)
	{
		Vector_Push(&edge, entries[i].expression.start);
	}
	
	DFAState *start = DFA_FromEntriesRecurse(dfa, entries, entriesSize, &allocator, &stateKeyToDFAState, &edge);
	
	/* Clean up */
	StackAllocator_Destroy(&allocator);
	HashTable_Destroy(&stateKeyToDFAState);
	Vector_Destroy(&edge);
	
	return start;
}
static int DFA_SortDFAStateBySymbol(const void *lhs, const void *rhs)
{
	DFAState *a = *(DFAState **)lhs;
	DFAState *b = *(DFAState **)rhs;
	
	if (a->symbol < b->symbol)
	{
		return -1;
	}
	else if (a->symbol > b->symbol)
	{
		return 1;
	}
	return 0;
}
static size_t DFA_HashDFAState(const void *data)
{
	return (size_t)data * 2654435761;
}
static bool DFA_CompareDFAState(const void *lhs, const void *rhs)
{
	return lhs == rhs;
}
static size_t DFA_HashSizeT(const void *data)
{
	return (size_t)data * 2654435761;
}
static bool DFA_CompareSizeT(const void *lhs, const void *rhs)
{
	return (size_t)lhs == (size_t)rhs;
}
DFAState *DFA_Minimize(DFA *dfa, DFAState *start)
{
	if (dfa->states.size == 0)
	{
		return start;
	}
	
	/* Partitions */
	Vector partitions;
	Vector_Create(&partitions, 16);
	
	/* State to partition leader*/
	HashTable stateToPartitionLeader;
	HashTable_Create(&stateToPartitionLeader, dfa->states.size + dfa->states.size / 2, 1.0f, DFA_HashDFAState, DFA_CompareDFAState);
	
	/* Sort by symbol */
	qsort(dfa->states.data, dfa->states.size, sizeof(DFAState *), DFA_SortDFAStateBySymbol);
	
	/* Build initial partitions */
	{
		size_t partitionLeader = 0;
		Vector_Push(&partitions, (void *)partitionLeader);
		
		DFAState *lastState = Vector_Get(&dfa->states, 0);
		HashTable_Insert(&stateToPartitionLeader, lastState, (void *)partitionLeader);
		
		for (size_t i = 1; i < dfa->states.size; ++i)
		{
			DFAState *nextState = Vector_Get(&dfa->states, i);
			if (lastState->symbol != nextState->symbol)
			{
				partitionLeader = i;
				Vector_Push(&partitions, (void *)partitionLeader);
			}
			HashTable_Insert(&stateToPartitionLeader, nextState, (void *)partitionLeader);
			lastState = nextState;
		}
	}
	
	/* Refine partitions */
	{
		Vector newPartitions;
		Vector_Create(&newPartitions, partitions.size);
		
		bool done = false;
		while (!done)
		{
			done = true;
			
			/* Start building new partitions */
			for (size_t i = 0; i < partitions.size; ++i)
			{
				size_t partitionBegin = (size_t)Vector_Get(&partitions, i);
				size_t partitionEnd = (i < partitions.size - 1 ? (size_t)Vector_Get(&partitions, i + 1) : dfa->states.size);
				size_t newPartitionBegin = partitionEnd;
				
				/* Add current partition */
				Vector_Push(&newPartitions, (void *)partitionBegin);
				
				/* Run through the alphabet and stop if we run out of non-leader states in the partition */
				DFAState *leader = Vector_Get(&dfa->states, partitionBegin);
				for (int c = 0; c < DFASTATE_EDGES_MAX && partitionBegin + 1 < newPartitionBegin; ++c)
				{
					/* Get leader's next partition for the current character */
					DFAState *leaderNext = leader->edges[c];
					size_t leaderNextPartition;
					if (!leaderNext)
					{
						leaderNextPartition = (size_t)(-1);
					}
					else
					{
						leaderNextPartition = (size_t)*HashTable_Find(&stateToPartitionLeader, leaderNext);
					}
					
					/* Run through the other states in the partition */
					for (size_t j = partitionBegin + 1; j < newPartitionBegin; ++j)
					{
						/* Get the state's next partition for the current character */
						DFAState *state = Vector_Get(&dfa->states, j);
						DFAState *stateNext = state->edges[c];
						size_t stateNextPartition;
						if (!stateNext)
						{
							stateNextPartition = (size_t)(-1);
						}
						else
						{
							stateNextPartition = (size_t)*HashTable_Find(&stateToPartitionLeader, stateNext);
						}
						
						/* If they're different, swap the state to the end of the partition */
						if (leaderNextPartition != stateNextPartition)
						{
							--newPartitionBegin;
							DFAState *temp = Vector_Get(&dfa->states, newPartitionBegin);
							Vector_Set(&dfa->states, newPartitionBegin, state);
							Vector_Set(&dfa->states, j, temp);
							--j;
						}
					}
				}
				
				/* If a new partition was created, add it */
				if (newPartitionBegin != partitionEnd)
				{
					done = false;
					Vector_Push(&newPartitions, (void *)newPartitionBegin);
				}
			}
			
			/* If we're not done, fix up the state to partition leader table */
			if (!done)
			{
				for (size_t i = 1, j = 1; j < newPartitions.size; ++i, ++j)
				{
					size_t partitionLeader = (i < partitions.size ? (size_t)Vector_Get(&partitions, i) : partitions.size);
					size_t newPartitionLeader = (size_t)Vector_Get(&newPartitions, j);
					if (partitionLeader != newPartitionLeader)
					{
						for (size_t k = newPartitionLeader; k < partitionLeader; k++)
						{
							DFAState *state = Vector_Get(&dfa->states, k);
							*(size_t *)HashTable_Find(&stateToPartitionLeader, state) = newPartitionLeader;
						}
						++j;
					}
				}

				/* Swap partitions with new partitions */
				Vector temp = partitions;
				partitions = newPartitions;
				newPartitions = temp;
				Vector_Clear(&newPartitions);
			}
		}
		
		Vector_Destroy(&newPartitions);
	}
	
	/* Build new states */
	StackAllocator newAllocator;
	StackAllocator_Create(&newAllocator, StackAllocator_DefaultGetNextCapacity);
	
	Vector newStates;
	Vector_Create(&newStates, partitions.size);
	
	HashTable partitionLeaderToNewState;
	HashTable_Create(&partitionLeaderToNewState, partitions.size + partitions.size / 2, 1.0f, DFA_HashSizeT, DFA_CompareSizeT);
	
	for (size_t i = 0; i < partitions.size; ++i)
	{
		DFAState *newState = StackAllocator_Allocate(&newAllocator, sizeof(DFAState));
		HashTable_Insert(&partitionLeaderToNewState, Vector_Get(&partitions, i), newState);
		Vector_Push(&newStates, newState);
	}
	
	for (size_t i = 0; i < partitions.size; ++i)
	{
		size_t partitionLeader = (size_t)Vector_Get(&partitions, i);
		DFAState *leader = Vector_Get(&dfa->states, partitionLeader);
		DFAState *newState = *HashTable_Find(&partitionLeaderToNewState, (void *)partitionLeader);
		newState->symbol = leader->symbol;
		
		for (int c = 0; c < DFASTATE_EDGES_MAX; ++c)
		{
			DFAState *nextState = leader->edges[c];
			DFAState *newNextState = NULL;
			if (nextState)
			{
				size_t nextPartitionLeader = (size_t)*HashTable_Find(&stateToPartitionLeader, nextState);
				newNextState = *HashTable_Find(&partitionLeaderToNewState, (void *)nextPartitionLeader);
			}
			newState->edges[c] = newNextState;
		}
	}
	
	/* Find new start */
	DFAState *newStart = *HashTable_Find(&partitionLeaderToNewState, *HashTable_Find(&stateToPartitionLeader, start));
	
	/* Clean up */
	Vector_Destroy(&partitions);
	HashTable_Destroy(&stateToPartitionLeader);
	HashTable_Destroy(&partitionLeaderToNewState);
	
	/* Swap storage */
	StackAllocator_Destroy(&dfa->allocator);
	dfa->allocator = newAllocator;
	Vector_Destroy(&dfa->states);
	dfa->states = newStates;
	
	return newStart;
}
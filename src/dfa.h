#pragma once

#include "nfa.h"
#include "vector.h"

typedef struct DFAEntry DFAEntry;
typedef struct DFAState DFAState;
typedef struct DFA DFA;

struct DFAEntry
{
	NFAExpression expression;
	const char *symbol;
};
struct DFAState
{
	const char *symbol;
	DFAState *edges[DFASTATE_EDGES_MAX];
};
struct DFA
{
	StackAllocator allocator;
	Vector states;
};

void DFAState_Initialize(DFAState *state);

void DFA_Create(DFA *dfa);
void DFA_Destroy(DFA *dfa);
DFAState *DFA_AddState(DFA *dfa);
DFAState *DFA_FromEntries(DFA *dfa, DFAEntry *entries, size_t expressionCount);
DFAState *DFA_Minimize(DFA *dfa, DFAState *source);
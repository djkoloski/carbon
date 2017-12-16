#pragma once

#include "stack_allocator.h"

typedef struct NFAEdge NFAEdge;
typedef struct NFAState NFAState;
typedef struct NFAExpression NFAExpression;
typedef struct NFA NFA;

struct NFAEdge
{
	char symbol;
	NFAState *state;
};
struct NFAState
{
	NFAEdge left;
	NFAEdge right;
};
struct NFAExpression
{
	NFAState *start;
	NFAState *end;
};
struct NFA
{
	StackAllocator allocator;
};

void NFAState_Initialize(NFAState *state);

void NFA_Create(NFA *nfa);
void NFA_Destroy(NFA *nfa);
NFAState *NFA_AddState(NFA *nfa);
void NFA_ParseRegex(NFA *nfa, const char *regex, NFAExpression *expr);
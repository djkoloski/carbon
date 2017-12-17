#pragma once

#include "stack_allocator.h"

typedef struct NFAEdgeConditions NFAEdgeConditions;
typedef struct NFAEdge NFAEdge;
typedef struct NFAState NFAState;
typedef struct NFAExpression NFAExpression;
typedef struct NFA NFA;

#define DFASTATE_EDGES_MAX 128
struct NFAEdgeConditions
{
	unsigned char bits[DFASTATE_EDGES_MAX / 8];
};
struct NFAEdge
{
	NFAEdgeConditions conditions;
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

void NFAEdgeConditions_Set(NFAEdgeConditions *condition, size_t index);
void NFAEdgeConditions_Reset(NFAEdgeConditions *condition, size_t index);
bool NFAEdgeConditions_Get(const NFAEdgeConditions *condition, size_t index);
void NFAEdgeConditions_Or(NFAEdgeConditions *condition, const NFAEdgeConditions *other);
void NFAEdgeConditions_Invert(NFAEdgeConditions *condition);

void NFAState_Initialize(NFAState *state);

void NFA_Create(NFA *nfa);
void NFA_Destroy(NFA *nfa);
NFAState *NFA_AddState(NFA *nfa);
void NFA_ParseRegex(NFA *nfa, const char *regex, NFAExpression *expr);
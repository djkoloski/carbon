#include "nfa.h"

#include <string.h>

void NFAEdgeConditions_Set(NFAEdgeConditions *conditions, size_t index)
{
	conditions->bits[index / 8] |= (1 << (index % 8));
}
void NFAEdgeConditions_Reset(NFAEdgeConditions *conditions, size_t index)
{
	conditions->bits[index / 8] &= ~(1 << (index % 8));
}
bool NFAEdgeConditions_Get(const NFAEdgeConditions *conditions, size_t index)
{
	return (conditions->bits[index / 8] & (1 << (index % 8)));
}
bool NFAEdgeConditions_Empty(const NFAEdgeConditions *conditions)
{
	unsigned char result = 0;
	for (size_t i = 0; i < DFASTATE_EDGES_MAX / 8; ++i)
	{
		result |= conditions->bits[i];
	}
	return !result;
}
void NFAEdgeConditions_Or(NFAEdgeConditions *conditions, const NFAEdgeConditions *other)
{
	for (size_t i = 0; i < DFASTATE_EDGES_MAX / 8; ++i)
	{
		conditions->bits[i] |= other->bits[i];
	}
}

void NFAState_Initialize(NFAState *state)
{
	memset(state, 0, sizeof(NFAState));
}

void NFA_Create(NFA *nfa)
{
	StackAllocator_Create(&nfa->allocator, StackAllocator_DefaultGetNextCapacity);
}
void NFA_Destroy(NFA *nfa)
{
	StackAllocator_Destroy(&nfa->allocator);
}
NFAState *NFA_AddState(NFA *nfa)
{
	NFAState *state = StackAllocator_Allocate(&nfa->allocator, sizeof(NFAState));
	NFAState_Initialize(state);
	return state;
}
static void NFA_ParseRegexExpression(NFA *nfa, const char **regex, NFAExpression *expr);
void NFA_ParseRegex(NFA *nfa, const char *regex, NFAExpression *expr)
{
	NFA_ParseRegexExpression(nfa, &regex, expr);
}

static void NFA_BuildOrExpression(NFA *nfa, const NFAExpression *left, const NFAExpression *right, NFAExpression *result)
{
	result->start = NFA_AddState(nfa);
	result->start->left.state = left->start;
	NFAEdgeConditions_Set(&result->start->left.conditions, 0);
	result->start->right.state = right->start;
	NFAEdgeConditions_Set(&result->start->right.conditions, 0);
	
	result->end = NFA_AddState(nfa);
	left->end->left.state = result->end;
	NFAEdgeConditions_Set(&left->end->left.conditions, 0);
	right->end->left.state = result->end;
	NFAEdgeConditions_Set(&right->end->left.conditions, 0);
}
static void NFA_BuildAndExpression(const NFAExpression *prev, const NFAExpression *next, NFAExpression *result)
{
	result->start = prev->start;
	prev->end->left.state = next->start;
	NFAEdgeConditions_Set(&prev->end->left.conditions, 0);
	result->end = next->end;
}
static void NFA_BuildKleeneStarExpression(NFA *nfa, const NFAExpression *inner, NFAExpression *result)
{
	result->start = NFA_AddState(nfa);
	result->end = NFA_AddState(nfa);
	
	result->start->left.state = inner->start;
	NFAEdgeConditions_Set(&result->start->left.conditions, 0);
	result->start->right.state = inner->end;
	NFAEdgeConditions_Set(&result->start->right.conditions, 0);
	
	inner->end->left.state = inner->start;
	NFAEdgeConditions_Set(&inner->end->left.conditions, 0);
	inner->end->right.state = result->end;
	NFAEdgeConditions_Set(&inner->end->right.conditions, 0);
}
void NFA_ParseRegexConditions(const char **regex, NFAEdgeConditions *conditions)
{
	while (**regex && **regex != ']')
	{
		char from = **regex;
		++*regex;
		assert(**regex == '-');
		++*regex;
		char to = **regex;
		++*regex;
		
		for (int i = from; i <= to; ++i)
		{
			NFAEdgeConditions_Set(conditions, i);
		}
	}
	++*regex;
}
void NFA_ParseRegexBase(NFA *nfa, const char **regex, NFAExpression *expr)
{
	switch (**regex)
	{
		case '(':
			++*regex;
			NFA_ParseRegexExpression(nfa, regex, expr);
			assert(**regex == ')');
			++*regex;
			break;
		case '[':
			++*regex;
			expr->start = NFA_AddState(nfa);
			expr->end = NFA_AddState(nfa);
			NFA_ParseRegexConditions(regex, &expr->start->left.conditions);
			expr->start->left.state = expr->end;
			break;
		case '\\':
			++*regex;
		default:
			expr->start = NFA_AddState(nfa);
			expr->end = NFA_AddState(nfa);
			NFAEdgeConditions_Set(&expr->start->left.conditions, **regex);
			expr->start->left.state = expr->end;
			++*regex;
			break;
	}
}
void NFA_ParseRegexFactor(NFA *nfa, const char **regex, NFAExpression *expr)
{
	NFA_ParseRegexBase(nfa, regex, expr);
	if (**regex == '*')
	{
		++*regex;
		NFAExpression inner = *expr;
		
		NFA_BuildKleeneStarExpression(nfa, &inner, expr);
	}
}
void NFA_ParseRegexTerm(NFA *nfa, const char **regex, NFAExpression *expr)
{
	NFA_ParseRegexFactor(nfa, regex, expr);
	while (**regex && **regex != ')' && **regex != '|')
	{
		NFAExpression prev = *expr;
		NFAExpression next;
		NFA_ParseRegexFactor(nfa, regex, &next);
		
		NFA_BuildAndExpression(&prev, &next, expr);
	}
}
void NFA_ParseRegexExpression(NFA *nfa, const char **regex, NFAExpression *expr)
{
	NFA_ParseRegexTerm(nfa, regex, expr);
	while (**regex == '|')
	{
		++*regex;
		NFAExpression left = *expr;
		NFAExpression right;
		NFA_ParseRegexExpression(nfa, regex, &right);
		
		NFA_BuildOrExpression(nfa, &left, &right, expr);
	}
}
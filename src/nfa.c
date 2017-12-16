#include "nfa.h"

#include <string.h>

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
	result->start->right.state = right->start;
	
	result->end = NFA_AddState(nfa);
	left->end->left.state = result->end;
	right->end->left.state = result->end;
}
static void NFA_BuildAndExpression(const NFAExpression *prev, const NFAExpression *next, NFAExpression *result)
{
	result->start = prev->start;
	prev->end->left.state = next->start;
	result->end = next->end;
}
static void NFA_BuildKleeneStarExpression(NFA *nfa, const NFAExpression *inner, NFAExpression *result)
{
	result->start = NFA_AddState(nfa);
	result->end = NFA_AddState(nfa);
	
	result->start->left.state = inner->start;
	result->start->right.state = inner->end;
	
	inner->end->left.state = inner->start;
	inner->end->right.state = result->end;
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
		case '\\':
			++*regex;
		default:
			expr->start = NFA_AddState(nfa);
			expr->end = NFA_AddState(nfa);
			expr->start->left.symbol = **regex;
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
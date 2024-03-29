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
void NFAEdgeConditions_Invert(NFAEdgeConditions *conditions)
{
	for (size_t i = 0; i < DFASTATE_EDGES_MAX / 8; ++i)
	{
		conditions->bits[i] = ~conditions->bits[i];
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
static void NFA_BuildStarExpression(NFA *nfa, const NFAExpression *inner, NFAExpression *result)
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
static void NFA_BuildPlusExpression(NFA *nfa, const NFAExpression *inner, NFAExpression *result)
{
	result->start = NFA_AddState(nfa);
	result->end = NFA_AddState(nfa);
	
	result->start->left.state = inner->start;
	NFAEdgeConditions_Set(&result->start->left.conditions, 0);
	
	inner->end->left.state = inner->start;
	NFAEdgeConditions_Set(&inner->end->left.conditions, 0);
	inner->end->right.state = result->end;
	NFAEdgeConditions_Set(&inner->end->right.conditions, 0);
}
static void NFA_BuildQuestionExpression(NFA *nfa, const NFAExpression *inner, NFAExpression *result)
{
	result->start = NFA_AddState(nfa);
	result->end = NFA_AddState(nfa);
	
	result->start->left.state = inner->start;
	NFAEdgeConditions_Set(&result->start->left.conditions, 0);
	result->start->right.state = inner->end;
	NFAEdgeConditions_Set(&result->start->right.conditions, 0);
	
	inner->end->right.state = result->end;
	NFAEdgeConditions_Set(&inner->end->right.conditions, 0);
}
static bool NFA_ParseRegexCharacter(const char **regex, char *value)
{
	*value = **regex;
	++*regex;

	if (*value == '\\')
	{
		*value = **regex;
		++*regex;
		switch (*value)
		{
			case 't':
				*value = '\t';
				break;
			case 'r':
				*value = '\r';
				break;
			case 'n':
				*value = '\n';
				break;
			default:
				break;
		}
		return true;
	}

	return false;
}
static void NFA_ParseRegexConditions(const char **regex, NFAEdgeConditions *conditions)
{
	bool invert = false;
	if (**regex == '^')
	{
		invert = true;
		++*regex;
	}
	
	while (**regex && **regex != ']')
	{
		char from = 0;
		bool escapeFrom = NFA_ParseRegexCharacter(regex, &from);
		
		if (**regex == '-')
		{
			++*regex;
			char to = 0;
			bool escapeTo = NFA_ParseRegexCharacter(regex, &to);
			
			for (int i = from; i <= to; ++i)
			{
				NFAEdgeConditions_Set(conditions, i);
			}
		}
		else
		{
			NFAEdgeConditions_Set(conditions, from);
		}
	}
	++*regex;
	
	if (invert)
	{
		NFAEdgeConditions_Invert(conditions);
	}
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
		default:
			char value = 0;
			bool escaped = NFA_ParseRegexCharacter(regex, &value);
			
			expr->start = NFA_AddState(nfa);
			expr->end = NFA_AddState(nfa);
			
			if (!escaped && value == '.')
			{
				NFAEdgeConditions_Invert(&expr->start->left.conditions);
				NFAEdgeConditions_Reset(&expr->start->left.conditions, '\r');
				NFAEdgeConditions_Reset(&expr->start->left.conditions, '\n');
			}
			else
			{
				NFAEdgeConditions_Set(&expr->start->left.conditions, value);
			}
			expr->start->left.state = expr->end;
			break;
	}
}
void NFA_ParseRegexFactor(NFA *nfa, const char **regex, NFAExpression *expr)
{
	NFA_ParseRegexBase(nfa, regex, expr);
	switch (**regex)
	{
		case '*':
		{
			++*regex;
			NFAExpression inner = *expr;
			
			NFA_BuildStarExpression(nfa, &inner, expr);
			break;
		}
		case '+':
		{
			++*regex;
			NFAExpression inner = *expr;
			
			NFA_BuildPlusExpression(nfa, &inner, expr);
			break;
		}
		case '?':
		{
			++*regex;
			NFAExpression inner = *expr;
			
			NFA_BuildQuestionExpression(nfa, &inner, expr);
			break;
		}
		default:
			break;
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
#include "dfa.h"
#include "hash_table.h"

void WriteHeader(FILE *output, const char **symbols, size_t symbolsSize)
{
	const char *header = "/* Generated by CLex */\n\
\n\
typedef enum TokenType\n\
{\n\
	TokenType_CLex_Reject";
	
	fprintf(output, header);
	
	for (size_t i = 0; i < symbolsSize; ++i)
	{
		fprintf(output, ",\n\tTokenType_%s", symbols[i]);
	}
	fprintf(output, "\n} TokenType;\n\nTokenType CLex(const char **input);");
}
void WriteDFAState(const DFAState *state, HashTable *stateToIndex, FILE *output)
{
	fprintf(output, "\t{");
	if (state->symbol)
	{
		fprintf(output, "TokenType_%s, {[0] = 0", state->symbol);
	}
	else
	{
		fprintf(output, "TokenType_CLex_Reject, {[0] = 0");
	}
	
	bool first = true;
	for (int i = 0; i < DFASTATE_EDGES_MAX; ++i)
	{
		if (state->edges[i])
		{
			fprintf(output, ", [%i] = %zu", i, (size_t)*HashTable_Find(stateToIndex, state->edges[i]));
			first = false;
		}
	}
	
	fprintf(output, "}}");
}
static size_t DFA_HashDFAState(const void *data)
{
	return (size_t)data * 2654435761;
}
static bool DFA_CompareDFAState(const void *lhs, const void *rhs)
{
	return lhs == rhs;
}
void WriteSource(FILE *output, const DFA *dfa, const DFAState *state)
{
	HashTable stateToIndex;
	HashTable_Create(&stateToIndex, dfa->states.size + dfa->states.size / 2, 1.0f, DFA_HashDFAState, DFA_CompareDFAState);
	
	for (size_t i = 0; i < dfa->states.size; ++i)
	{
		HashTable_Insert(&stateToIndex, Vector_Get(&dfa->states, i), (void *)(i + 1));
	}
	
	const char *header = "/* Generated by CLex */\n\
\n\
#include \"output.clex.h\"\n\
\n\
typedef struct State State;\n\
struct State\n\
{\n\
	TokenType type;\n\
	size_t edges[128];\n\
};\n\
\n\
static const State k_states[] =\n\
{\n\
	{TokenType_CLex_Reject, {[0] = 0}},\n\
";
	fprintf(output, header);
	for (size_t i = 0; i < dfa->states.size; ++i)
	{
		if (i != 0)
		{
			fprintf(output, ",\n");
		}
		WriteDFAState(Vector_Get(&dfa->states, i), &stateToIndex, output);
	}
	const char *footer = "\n};\n";
	fprintf(output, footer);
	
	fprintf(output, "\nstatic const size_t k_initialState = %zu;\n", (size_t)*HashTable_Find(&stateToIndex, state));
	
	const char *clexDefinition = "\n\
TokenType CLex(const char **input)\n\
{\n\
	size_t lastState = k_initialState;\n\
	size_t state = k_states[lastState].edges[**input];\n\
	while (state)\n\
	{\n\
		++*input;\n\
		lastState = state;\n\
		state = k_states[lastState].edges[**input];\n\
	}\n\
	return k_states[lastState].type;\n\
}";
	fprintf(output, clexDefinition);
	
	HashTable_Destroy(&stateToIndex);
}

int main(int argc, char **argv)
{
	NFA nfa;
	NFA_Create(&nfa);
	
	const char *symbols[] = {
		"If",
		"Id",
		"Number",
		"Whitespace"
	};
	
	DFAEntry entries[4];
	entries[0].symbol = symbols[0];
	entries[1].symbol = symbols[1];
	entries[2].symbol = symbols[2];
	entries[3].symbol = symbols[3];
	NFA_ParseRegex(&nfa, "if", &entries[0].expression);
	NFA_ParseRegex(&nfa, "[a-z][a-z0-9]*", &entries[1].expression);
	NFA_ParseRegex(&nfa, "[0-9][0-9]*", &entries[2].expression);
	NFA_ParseRegex(&nfa, "( |\t|\r|\n)( |\t|\r|\n)*", &entries[3].expression);
	
	DFA dfa;
	DFA_Create(&dfa);
	DFAState *start = DFA_FromEntries(&dfa, entries, 4);
	start = DFA_Minimize(&dfa, start);
	
	FILE *outputHeader;
	fopen_s(&outputHeader, "test/output.clex.h", "wb");
	
	WriteHeader(outputHeader, symbols, 4);
	
	fclose(outputHeader);
	
	FILE *outputSource;
	fopen_s(&outputSource, "test/output.clex.c", "wb");
	
	WriteSource(outputSource, &dfa, start);
	
	fclose(outputSource);
	
	DFA_Destroy(&dfa);
	NFA_Destroy(&nfa);
	
	return 0;
}
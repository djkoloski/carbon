#include "codegen.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct InputRule InputRule;
struct InputRule
{
	const char *symbol;
	const char *regex;
};

void ParseInputRules(const char *inputPath, InputRule **inputRules, size_t *inputRulesSize)
{
	size_t capacity = 16;
	*inputRulesSize = 0;
	*inputRules = malloc(capacity * sizeof(InputRule));
	
	FILE *input;
	fopen_s(&input, inputPath, "rb");
	
	fseek(input, 0, SEEK_END);
	size_t inputSize = ftell(input);
	fseek(input, 0, SEEK_SET);
	
	char *file = malloc(inputSize + 1);
	fread(file, 1, inputSize, input);
	file[inputSize] = 0;
	
	fclose(input);
	
	const char *c = file;
	while (*c)
	{
		while (*c && isspace(*c))
		{
			++c;
		}
		if (!*c)
		{
			break;
		}
		
		const char *symbolStart = c;
		
		while (*c && !isspace(*c))
		{
			++c;
		}
		
		const char *symbolEnd = c;
		
		while (*c && isspace(*c))
		{
			++c;
		}
		
		const char *regexStart = c;
		
		while (*c && *c != '\r' && *c != '\n')
		{
			++c;
		}
		
		const char *regexEnd = c;
		while (isspace(*(regexEnd - 1)))
		{
			--regexEnd;
		}
		
		if (*inputRulesSize == capacity)
		{
			capacity += capacity / 2;
			*inputRules = realloc(*inputRules, capacity * sizeof(InputRule));
		}
		
		size_t symbolSize = symbolEnd - symbolStart;
		char *symbol = malloc(symbolSize + 1);
		memcpy(symbol, symbolStart, symbolSize);
		symbol[symbolSize] = 0;
		(*inputRules)[*inputRulesSize].symbol = symbol;
		
		size_t regexSize = regexEnd - regexStart;
		char *regex = malloc(regexSize + 1);
		memcpy(regex, regexStart, regexSize);
		regex[regexSize] = 0;
		(*inputRules)[*inputRulesSize].regex = regex;
		
		++*inputRulesSize;
	}
}

void PrintUsage()
{
	printf("Usage: clex input -o header source\n");
}

int main(int argc, char **argv)
{
	/* Parse arguments */
	const char *inputPath = NULL;
	const char *outputHeaderPath = NULL;
	const char *outputSourcePath = NULL;
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-o"))
		{
			++i;
			outputHeaderPath = argv[i];
			++i;
			outputSourcePath = argv[i];
		}
		else
		{
			inputPath = argv[i];
		}
	}
	
	if (!inputPath || !outputHeaderPath || !outputSourcePath)
	{
		PrintUsage();
		return -1;
	}
	
	InputRule *inputRules = NULL;
	size_t inputRulesSize = 0;
	ParseInputRules(inputPath, &inputRules, &inputRulesSize);
	
	/* Generate DFA */
	NFA nfa;
	NFA_Create(&nfa);
	
	DFAEntry *entries = malloc(inputRulesSize * sizeof(DFAEntry));
	for (size_t i = 0; i < inputRulesSize; ++i)
	{
		entries[i].symbol = inputRules[i].symbol;
		NFA_ParseRegex(&nfa, inputRules[i].regex, &entries[i].expression);
	}
	
	DFA dfa;
	DFA_Create(&dfa);
	DFAState *start = DFA_FromEntries(&dfa, entries, inputRulesSize);
	start = DFA_Minimize(&dfa, start);

	/* Write header */
	FILE *outputHeader;
	fopen_s(&outputHeader, outputHeaderPath, "wb");
	const char **symbols = malloc(inputRulesSize * sizeof(const char *));
	for (size_t i = 0; i < inputRulesSize; ++i)
	{
		symbols[i] = inputRules[i].symbol;
	}
	Codegen_WriteHeader(outputHeader, symbols, inputRulesSize);
	free((void *)symbols);
	fclose(outputHeader);
	
	/* Write source */
	FILE *outputSource;
	fopen_s(&outputSource, outputSourcePath, "wb");
	Codegen_WriteSource(outputSource, &dfa, start, outputHeaderPath);
	fclose(outputSource);
	
	/* Clean up */
	DFA_Destroy(&dfa);
	NFA_Destroy(&nfa);
	free(entries);
	for (size_t i = 0; i < inputRulesSize; ++i)
	{
		free((void *)inputRules[i].symbol);
		free((void *)inputRules[i].regex);
	}
	free(inputRules);
	
	return 0;
}
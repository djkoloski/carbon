#pragma once

#include "dfa.h"

void Codegen_WriteHeader(FILE *output, const char **symbols, size_t symbolsSize);
void Codegen_WriteSource(FILE *output, const DFA *dfa, const DFAState *start, const char *outputHeaderPath);
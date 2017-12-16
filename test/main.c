#include "output.clex.h"

#include <stdio.h>

void ParseSource(const char *c)
{
	TokenType type;
	while (*c && (type = CLex(&c)) != TokenType_CLex_Reject)
	{
		printf("Got token: %i\n", (int)type);
	}
}

int main()
{
	ParseSource("if a 4 ifelse\t\t\t\nwhat");
}
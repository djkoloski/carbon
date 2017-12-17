#include "test.clex.h"

#include <stdio.h>

void ParseSource(const char *c)
{
	TokenType type;
	while (*c)
	{
		const char *begin = c;
		type = CLex(&c);
		if (type == TokenType_CLex_Reject)
		{
			break;
		}
		
		const char *typeName = NULL;
		switch (type)
		{
			case TokenType_LeftParenthesis:
				typeName = "Left Parenthesis";
				break;
			case TokenType_RightParenthesis:
				typeName = "Right Parenthesis";
				break;
			case TokenType_LeftBrace:
				typeName = "Left Brace";
				break;
			case TokenType_RightBrace:
				typeName = "Right Brace";
				break;
			case TokenType_Semicolon:
				typeName = "Semicolon";
				break;
			case TokenType_KeywordInt:
				typeName = "Keyword int";
				break;
			case TokenType_KeywordReturn:
				typeName = "Keyword return";
				break;
			case TokenType_Number:
				typeName = "Number";
				break;
			case TokenType_Identifier:
				typeName = "Identifier";
				break;
			default:
				break;
		}
		
		if (type != TokenType_Whitespace)
		{
			printf("Token '%s':\n\t'%.*s'\n", typeName, (int)(c - begin), begin);
		}
	}
}

int main()
{
	ParseSource("// This is a good comment :)\n\
/* this is an evil \n mean comment from hell **/ /* this is a separate one */ int main()\n\
{\n\
	return 4;\n\
}");
}
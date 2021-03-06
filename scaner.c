//
//  scaner.c
//  RuC
//
//  Created by Andrey Terekhov on 04/08/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"

extern void error(int type);
extern void printf_char(int);

int getnext()
{
	// reads UTF-8

	unsigned char firstchar, secondchar;
	if (fscanf(input, "%c", &firstchar) == EOF)
		return EOF;
	else
	{
		if ((firstchar & /*0b11100000*/0xE0) == /*0b11000000*/0xC0)
		{
			fscanf(input, "%c", &secondchar);

			nextchar = ((int)(firstchar & /*0b11111*/0x1F)) << 6 | (secondchar & /*0b111111*/0x3F);

			//            unsigned char first;
			//            unsigned char second;
			//
			//            second = (nextchar & 0b111111) | 0b10000000;
			//            first = (nextchar >> 6) | 0b11000000;
		}
		else
			nextchar = firstchar;
		if (nextchar == 13 /* cr */)
			getnext();
		//                if(kw)
		//                    printf("nextchar %c %i\n", nextchar, nextchar);
	}
	return nextchar;
}

void onemore()
{
    curchar = nextchar;
    nextchar = getnext();
    if (kw)
        printf("curchar =%c %i nextchar=%c %i\n", curchar, curchar, nextchar, nextchar);
}

void endofline()
{
    int j;
    printf("line %i) ", line-1);
    for (j=lines[line-1]; j<lines[line]; j++)
        printf_char(source[j]);
}

void endnl()
{
	lines[++line] = charnum;
	lines[line + 1] = charnum;
	if (kw)
	{
		endofline();
	}
}

int nextch()
{
	int cont = 1, wascomm = 0;
	while (cont)
	{
		onemore();
		if (curchar == EOF)
		{
			onemore();
			lines[++line] = charnum;
			lines[line + 1] = charnum;
			if (kw)
			{
				endofline();
				printf("\n");
			}
			return EOF;
		}

		source[charnum++] = curchar;
		if (instring)
			return curchar;

		if (curchar == '/' && nextchar == '/')
		{
			wascomm = 1;
			do
			{
				onemore();
				source[charnum++] = curchar;
			} while (curchar != '\n');
			endnl();
			onemore();
			source[charnum++] = curchar;
		}

		if (curchar == '/' && nextchar == '*')
		{
			wascomm = 1;
			onemore();
			source[charnum++] = curchar;
			do
			{
				onemore();
				source[charnum++] = curchar;
				if (curchar == '\n')
					endnl();
			} while (curchar != '*' || nextchar != '/');
			onemore();
			source[charnum++] = curchar;
			onemore();
			source[charnum++] = curchar;
		}
		//        printf("nextch curchar=%c %i\n", curchar,curchar);
		if (curchar != ' ' && curchar != '\t' && curchar != '\n')
			return curchar;

		if (curchar == '\n')
			endnl();
		//        printf("nextch curchar=%i nextchar=%i\n", curchar, nextchar);
		cont = nextchar == ' ' || nextchar == 9 /*'\t'*/ || nextchar == '\n' || wascomm;
	}
	return curchar = ' ';
}

void next_string_elem()
{
	num = curchar;
	if (curchar == '\\')
	{
		nextch();
		if (curchar == 'n' || curchar == 1085 /* 'н' */)
			num = 10;
		else if (curchar == 't' || curchar == 1090 /* 'т' */)
			num = 9;
		else if (curchar == '0')
			num = 0;
		else if (curchar != '\'' && curchar != '\\' && curchar != '\"')
			error(bad_escape_sym);
		else
			num = curchar;
	}
	nextch();
}


int letter()
{
    return (curchar >= 'A' && curchar <= 'Z') || (curchar >='a' && curchar <= 'z') || curchar == '_' ||
    (curchar >= 0x410/*А */ && curchar <= 0x44F /*'я'*/);
}

int digit()
{
	return curchar >= '0' && curchar <= '9';
}

int ispower()
{
	return curchar == 'e' || curchar == 'E';  // || curchar == 'е' || curchar == 'Е') // это русские е и Е

}

int equal(int i, int j)
{
	++i;
	++j;
	while (reprtab[++i] == reprtab[++j])
		if (reprtab[i] == 0 && reprtab[j] == 0)
			return 1;
	return 0;
}

int scan()
{
	int cr;
	if (curchar == ' ' || curchar == '\t' || curchar == '\n')
		nextch();
	//    printf("scan curchar=%c %i\n", curchar, curchar);
	switch (curchar)
	{
	case EOF:
		return LEOF;
	case ',':
	{
		nextch();
		return COMMA;
	}

	case '=':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = EQEQ;
		}
		else
			cr = ASS;
		return cr;

	case '*':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = MULTASS;
		}
		else
			cr = LMULT;
		return cr;

	case '/':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = DIVASS;
		}
		else
			cr = LDIV;
		return cr;

	case '%':
		nextch();
		if (nextchar == '=')
		{
			nextch();
			cr = REMASS;
		}
		else
			cr = LREM;
		return cr;

	case '+':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = PLUSASS;
		}
		else if (curchar == '+')
		{
			nextch();
			cr = INC;
		}
		else
			cr = LPLUS;
		return cr;

	case '-':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = MINUSASS;
		}
		else if (curchar == '-')
		{
			nextch();
			cr = DEC;
		}
		else
			cr = LMINUS;
		return cr;

	case '<':
		nextch();
		if (curchar == '<')
		{
			nextch();
			if (curchar == '=')
			{
				nextch();
				cr = SHLASS;
			}
			else
				cr = LSHL;
		}
		else if (curchar == '=')
		{
			nextch();
			cr = LLE;
		}
		else
			cr = LLT;

		return cr;

	case '>':
		nextch();
		if (curchar == '>')
		{
			nextch();
			if (curchar == '=')
			{
				nextch();
				cr = SHRASS;
			}
			else
				cr = LSHR;
		}
		else if (curchar == '=')
		{
			nextch();
			cr = LGE;
		}
		else
			cr = LGT;
		return cr;

	case '&':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = ANDASS;
		}
		else if (curchar == '&')
		{
			nextch();
			cr = LOGAND;
		}
		else
			cr = LAND;
		return cr;

	case '^':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = EXORASS;
		}
		else
			cr = LEXOR;
		return cr;

	case '|':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = ORASS;
		}
		else if (curchar == '|')
		{
			nextch();
			cr = LOGOR;
		}
		else
			cr = LOR;
		return cr;

	case '!':
		nextch();
		if (curchar == '=')
		{
			nextch();
			cr = NOTEQ;
		}
		else
			cr = LOGNOT;
		return cr;
	case '\'':
	{
		instring = 1;
		nextch();
		next_string_elem();
		if (curchar != '\'')
			error(no_right_apost);
		nextch();
		instring = 0;
		ansttype = LCHAR;
		return NUMBER;
	}
	case '\"':
	{
		instring = 1;
		int n = 0;
		nextch();
		while (curchar != '\"' && n < MAXSTRINGL)
		{
			next_string_elem();
			lexstr[n++] = num;
		}
		if (n == MAXSTRINGL)
			error(too_long_string);
		lexstr[n] = 0;
		nextch();
		instring = 0;
		ansttype = ROWOFCHAR;
		return STRING;
	}
	case '(':
	{
		nextch();
		return LEFTBR;
	}

	case ')':
	{
		nextch();
		return RIGHTBR;
	}

	case '[':
	{
		nextch();
		return LEFTSQBR;
	}

	case ']':
	{
		nextch();
		return RIGHTSQBR;
	}

	case '~':
	{
		nextch();
		return LNOT;
	}
	case '{':
	{
		nextch();
		return BEGIN;
	}
	case '}':
	{
		nextch();
		return END;
	}
	case ';':
	{
		nextch();
		return SEMICOLON;
	}
	case '?':
	{
		nextch();
		return QUEST;
	}
	case ':':
	{
		nextch();
		return COLON;
	}

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '.':
	{
		int flag = 1;
		float k;
		num = 0;
		while (digit())
		{
			num = num * 10 + (curchar - '0');
			nextch();
			flag = 0;
		}
		if (curchar != '.' && !ispower())
		{
			ansttype = LINT;
			return NUMBER;
		}
		if (curchar == '.')
		{
			nextch();
			if (!digit() && flag)
				error(must_be_digit_after_dot);
		}
		k = 0.1;
		numfloat = num;
		while (digit())
		{
			numfloat += (curchar - '0') * k;
			k *= 0.1;
			nextch();
		}
		if (ispower())
		{
			int d = 0, k = 1;
			nextch();
			if (curchar == '-')
			{
				nextch();
				k = -1;
			}
			else if (curchar == '+')
				nextch();
			while (digit())
			{
				d = d * 10 + curchar - '0';
				nextch();
			}
			numfloat *= pow(10, k*d);
		}
		ansttype = LFLOAT;
		memcpy(&numr, &numfloat, sizeof(num));
		return NUMBER;
	}

	default:
		if (letter())
		{
			int oldrepr, r;
			oldrepr = rp;
			rp += 2;
			hash = 0;
			do
			{

				hash += curchar;
				reprtab[rp++] = curchar;
				nextch();
			} while (letter() || digit());

			hash &= 255;
			reprtab[rp++] = 0;
			r = hashtab[hash];
			if (r)
			{
				do
				{
					if (equal(r, oldrepr))
					{
						rp = oldrepr;
						return (reprtab[r + 1] < 0) ? reprtab[r + 1] : (repr = r, IDENT);
					}
					else
						r = reprtab[r];
				} while (r);
			}
			reprtab[oldrepr] = hashtab[hash];
			repr = hashtab[hash] = oldrepr;
			reprtab[repr + 1] = (keywordsnum) ? -((++keywordsnum - 2) / 4) : 1;  // 0 - только MAIN, < 0 - ключевые слова, 1 - обычные иденты
			return IDENT;
		}
		else
		{
			printf("плохой символ %c %i\n", curchar, curchar);
			nextch();
			exit(10);
		}
	}
}

int scaner()
{
    cur = next;
    next = scan();
    //    if(kw)
    //        printf("scaner cur %i next %i repr %i\n", cur, next, repr);
    return cur;
}

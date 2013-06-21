/*
 *   as16 - the chip16 assembler 
 *   Copied partly from mash16
 *   Copyright (C) 2012-2013 tykel
 *
 *   as16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   as16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with as16.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defs.h"
#include "strings.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Globals */

#define MAX_LINES 10000

static instr_t is[MAX_LINES];
static symbol_t ss[MAX_LINES];
static int symind;

/*
 * Program flow (w/ support for macros):
 * - read file into line array
 * - run "preprocessor" on lines, substitute macros by defs
 * - run tokeniser to generate instr_t array
 * - run lexer to substitute labels etc.
 * - run code generator
 */

void free_string(string_t *str)
{
    free(str->str);
    free(str);
}

int line_parse(instr_t *instr)
{
    string_t *toktemp;
    char *sp;
    
    sp = instr->str;
    
    /* Label */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    if(token_iscomment(toktemp))
    {
        instr->iscomment = 1;
        return 0;
    }
    if(token_islabel(toktemp))
    {
        instr->toklabel = token_getlabel(toktemp);
        ss[symind].str = instr->toklabel->str;
        ss[symind].val = -1;
        ss[symind++].islabel = 1;
    }

    /* Mnemonic */
    if(instr->toklabel != NULL)
    {
        if(*sp == '\0')
		{
			instr->islabel = 1;
            return 0;
		}
        instr->tokmnem = token_next(&sp);
    }
    else
        instr->tokmnem = toktemp;

    /* OP1: RX / N / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    if(token_iscomment(toktemp))
	return 0;
    instr->tokop1 = toktemp;

    /* OP2: RY / N / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    if(token_iscomment(toktemp))
        return 0;
    instr->tokop2 = toktemp;
    if(instr->tokop1->str[0] == 'e' &&
       instr->tokop1->str[1] == 'q' &&
       instr->tokop1->str[2] == 'u')
    {
        ss[symind].str = instr->tokmnem->str;
        ss[symind++].val = token_getnum(instr->tokop2);
    }

    /* OP3: RZ / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    if(token_iscomment(toktemp))
        return 0;
    instr->tokop3 = toktemp;

    return 0;
}

/* Read a binary file into an unallocated buffer. */
int file_read(const char *fn, char **buf)
{
    FILE *file = NULL;
    int len;

    if((file = fopen(fn, "rb")) == NULL)
        return ERR_FILE;
    fseek(file, 0, SEEK_END);
    len = ftell(file);
    rewind(file);

    *buf = malloc(len + 1);
    if(*buf == NULL)
        return ERR_MALLOC;
    if(fread(*buf, 1, len, file) < len)
        return ERR_FILE;
    fclose(file);

    return len;
}

int main(int argc, char *argv[])
{
    FILE *file = NULL;
    char line[200], *sp;
    int ln = 1, len = 0, i;

    /* TODO: Proper argument parsing */
    if(argc > 1)
    {
        file = fopen(argv[1], "r");
        fgets(line, 200, file);

        while(line != NULL && !feof(file))
        {
            sp = line;
            while(*sp != '\n') ++sp;
            len = (int)(sp - line);
            *sp = '\0';
            
            is[ln - 1].str = line;
            is[ln - 1].ln = ln;
            is[ln - 1].len = len;
            line_parse(&is[ln - 1]);
            if(is[ln - 1].iscomment)
            {
                printf("%02d: comment\n", is[ln - 1].ln);
            }
            else
            {
                if(!is[ln - 1].islabel && is[ln - 1].len > 0)
                    is[ln - 1].op = token_mnem2op(is[ln - 1].tokmnem);
                printf("%02d: l[%s] m[%s] 1[%s] 2[%s] 3[%s]    (op: %d)\n",
                    is[ln - 1].ln,
                    is[ln - 1].toklabel != NULL ? is[ln - 1].toklabel->str : "",
                    is[ln - 1].tokmnem != NULL ? is[ln - 1].tokmnem->str : "",
                    is[ln - 1].tokop1 != NULL ? is[ln - 1].tokop1->str : "",
                    is[ln - 1].tokop2 != NULL ? is[ln - 1].tokop2->str : "",
                    is[ln - 1].tokop3 != NULL ? is[ln - 1].tokop3->str : "",
                    is[ln - 1].op);
            }
            printf("%02d: '%s'\n", is[ln - 1].ln, is[ln - 1].str);
            ++ln;
            fgets(line, 200, file);
        }
        
        printf("\n\n---------\n");
        for(i = 0; i < symind; ++i)
        {
            printf("%02d: [ '%s' : %d / 0x%x ]\n", i, ss[i].str, ss[i].val, ss[i].val);
        }
    }
        
    exit(0);
}

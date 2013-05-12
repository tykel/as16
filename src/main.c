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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_string(string_t *str)
{
    free(str->str);
    free(str);
}

string_t* token_next(char **str)
{
    int len, i;
    char *p;
    string_t *token;

    p = *str;
    len = 0;
    /* Determine token length */
    while (*p != ' ' && *p != '\t' && *p != '\0' && *p != '\n' && *p != ',')
    {
        ++len;
        ++p;
    }

    /* Allocate and copy string */
    token = malloc(sizeof(string_t));
    token->str = malloc(len + 1);
    for (i = 0; i<len; ++i)
        token->str[i] = (*str)[i];
    token->str[len] = '\0';
    token->len = len + 1;

    *str = p;
    while(**str == ' ' || **str == '\t' || **str == ',') // || **str == '\n' || **str == '\0')
        ++*str;

    return token;
}

int token_islabel(string_t *str)
{
    int len = str->len;
    return (str->str[len - 2] == ':');
}

int token_iscomment(string_t *str)
{
    return (str->str[0] == ';');
}

int line_parse(instr_t *instr)
{
    string_t *toktemp;
    int totallen, wordlen, token, islabel;
    char *sp;
    
    wordlen = 0;
    token = 0;
    islabel = 0;
    sp = instr->str;
    totallen = strlen(sp);
    
    /* Label */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    //sp += toktemp->len;
    if(token_islabel(toktemp))
       instr->toklabel = toktemp;

    /* Mnemonic */
    if(instr->toklabel != NULL)
    {
        if(*sp == '\0')
            return ERR_NO_MNEMONIC;
        instr->tokmnem = token_next(&sp);
        sp += toktemp->len;
    }
    else
        instr->tokmnem = toktemp;

    /* OP1: RX / N / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    //sp += toktemp->len;
    if(!token_iscomment(toktemp))
        instr->tokop1 = toktemp;

    /* OP2: RY / N / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    //sp += toktemp->len;
    if(!token_iscomment(toktemp))
        instr->tokop2 = toktemp;

    /* OP3: RZ / HHLL */
    if(*sp == '\0')
        return 0;
    toktemp = token_next(&sp);
    //sp += toktemp->len;
    if(!token_iscomment(toktemp))
        instr->tokop3 = toktemp;
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
    char *buf = NULL, *line = NULL;
    int sz = 0, ln = 1;
    instr_t is[1000];

    /* TODO: Proper argument parsing */
    if(argc > 1)
    {
        sz = file_read(argv[1], &buf);
        buf[sz] = '\0';
        printf("file length: %d bytes\n", sz);
        printf("file contents:\n%s\n", buf);

        line = strtok(buf, "\n");
        while(line != NULL)
        {
            is[ln - 1].str = line;
            is[ln - 1].ln = ln;
            line_parse(&is[ln - 1]);
            printf("%02d: label: '%s' mnem: '%s' op1: '%s' op2: '%s' op3: '%s'\n",
                    is[ln - 1].ln,
                    is[ln - 1].toklabel != NULL ? is[ln - 1].toklabel->str : "",
                    is[ln - 1].tokmnem != NULL ? is[ln - 1].tokmnem->str : "",
                    is[ln - 1].tokop1 != NULL ? is[ln - 1].tokop1->str : "",
                    is[ln - 1].tokop2 != NULL ? is[ln - 1].tokop2->str : "",
                    is[ln - 1].tokop3 != NULL ? is[ln - 1].tokop3->str : "");
            printf("%02d: '%s' [%d]\n", is[ln - 1].ln, is[ln - 1].str, is[ln - 1].op);
            ++ln;
            line = strtok(NULL, "\n");
        }
        free(buf);
    }
        
    exit(0);
}

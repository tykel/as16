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
#include "format.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Globals */

typedef unsigned short uint16_t;

#define MAX_LINES 10000

static instr_t instrs[MAX_LINES];
static symbol_t syms[MAX_LINES];
static int symind;

/*
 * Program flow (w/ support for macros):
 * - read file into line array
 * - run "preprocessor" on lines, substitute macros by defs
 * - run tokeniser to generate instr_t array
 * - run lexer to substitute labels etc.
 * - run code generator
 */

void log_error(char *fn, int ln, err_t err, void *data)
{
    fprintf(stderr, "%s:%d: ", fn, ln);
    switch(err)
    {
    case ERR_FILE:
        fprintf(stderr, "error: could not open file '%s'\n", (char *)data);
        break;
    case ERR_MALLOC:
        fprintf(stderr, "error: malloc failed\n");
        break;
    case ERR_NO_MNEMONIC:
        fprintf(stderr, "error: instruction has no mnemonic\n");
        break;
    case ERR_NO_OP1:
        fprintf(stderr, "error: instruction missing operand 1\n");
        break;
    case ERR_NO_OP2:
        fprintf(stderr, "error: instruction missing operand 2\n");
        break;
    case ERR_NO_OP3:
        fprintf(stderr, "error: instruction missing operand 3\n");
        break;
    case ERR_LABEL_REDEF:
        fprintf(stderr, "error: label '%s' redefined\n", (char *)data);
        break;
    case ERR_NOT_REG:
        fprintf(stderr, "error: '%s' is not a register\n", (char *)data);
        break;
    default:
        fprintf(stderr, "error: unknown\n");
        break;
    }
}

string_t *string_alloc(char *cs, int len)
{
    string_t *str = NULL;
    
    if(cs != NULL)
    {
        str = malloc(sizeof(string_t));
        if(str != NULL)
        {
            str->str = NULL;
            str->len = 0;
            str->str = malloc(len + (len > 0));
            if(str->str != NULL)
            {
                memcpy(str->str, cs, len);
                str->len = len;
            }
            if(len == 0)
            {
                str->str[0] = '\0';
            }
        }
    }

    return str;
}

void string_free(string_t *str)
{
    free(str->str);
    free(str);
}

/* Return the input format of the passed opcode. */
instr_args_t op_getargsformat(instr_t *instr)
{
    instr_args_t af = ARGS_ERR;
    if(instr->op < 0 || instr->op > 255)
    {
        return af;
    }
    switch(op_argsformat[instr->op])
    {
    case ARGS_NONE:
    {
        if(instr->tokop1 == NULL &&
           instr->tokop2 == NULL &&
           instr->tokop3 == NULL)
        {
            af = ARGS_NONE;
        }
        break;
    }
    case ARGS_I:
    {
        if(instr->tokop1 != NULL &&
           !token_isreg(instr->tokop1) &&
           instr->tokop2 == NULL &&
           instr->tokop3 == NULL)
        {
            af = ARGS_I;
        }
        break;
    }
    case ARGS_I_I:
    {
        if(instr->tokop1 != NULL &&
           !token_isreg(instr->tokop1) &&
           instr->tokop2 != NULL &&
           !token_isreg(instr->tokop2) &&
           instr->tokop3 == NULL)
        {
            af = ARGS_I_I;
        }
        break;
    }
    case ARGS_R:
    {
        if(instr->tokop1 != NULL &&
           token_isreg(instr->tokop1) &&
           instr->tokop2 == NULL &&
           instr->tokop3 == NULL)
        {
            af = ARGS_R;
        }
        break;
    }
    case ARGS_R_I:
    {
        if(instr->tokop1 != NULL &&
           token_isreg(instr->tokop1) &&
           instr->tokop2 != NULL &&
           !token_isreg(instr->tokop2) &&
           instr->tokop3 == NULL)
        {
            af = ARGS_R_I;
        }
        break;
    }
    case ARGS_R_R:
    {
        if(instr->tokop1 != NULL &&
           token_isreg(instr->tokop1) &&
           instr->tokop2 != NULL &&
           token_isreg(instr->tokop2) &&
           instr->tokop3 == NULL)
        {
            af = ARGS_R_R;
        }
        break;
    }
    case ARGS_R_R_I:
    {
        if(instr->tokop1 != NULL &&
           token_isreg(instr->tokop1) &&
           instr->tokop2 != NULL &&
           token_isreg(instr->tokop2) &&
           instr->tokop3 != NULL &&
           !token_isreg(instr->tokop3))
        {
            af = ARGS_R_R_I;
        }
        break;
    }
    case ARGS_R_R_R:
    {
        if(instr->tokop1 != NULL &&
           token_isreg(instr->tokop1) &&
           instr->tokop2 != NULL &&
           token_isreg(instr->tokop2) &&
           instr->tokop3 != NULL &&
           token_isreg(instr->tokop3))
        {
            af = ARGS_R_R_R;
        }
        break;
    }
    case ARGS_ERR:
    /*default:*/
        printf("ARGS_ERR/unknown found!! Shouldn't happen...\n");
        printf("instruction: op=%02x, '%s'\n", instr->op, instr->line->str);
        break; 
    }
    return af;
}

/* Determine the instruction's operand values. */
void op_getops(instr_t *instr)
{
    string_t copy;
    instr_args_t at = instr->args;
    if(at == ARGS_NONE || at == ARGS_ERR)
        return;

    /* First argument. */
    if(at == ARGS_I || at == ARGS_I_I)
        instr->op1 = token_getnum(instr->tokop1);
    else if(at == ARGS_R || at == ARGS_R_I || at == ARGS_R_R ||
            at == ARGS_R_R_I || at == ARGS_R_R_R)
    {
        copy = *instr->tokop1;
        if(copy.len > 1)
        {
            copy.str++;
            instr->op1 = token_getnum(&copy);
        }
        else
            printf("error: %s is not a register\n", copy.str);
    }

    /* Second argument. */
    if(at == ARGS_I_I || at == ARGS_R_I)
        instr->op2 = token_getnum(instr->tokop2);
    else if(at == ARGS_R_R || at == ARGS_R_R_I || at == ARGS_R_R_R)
    {
        copy = *instr->tokop2;
        if(copy.len > 1)
        {
            copy.str++;
            instr->op2 = token_getnum(&copy);
        }
        else
            printf("error: %s is not a register\n", copy.str);
    }
    
    /* Third argument. */
    if(at == ARGS_R_R_I)
        instr->op2 = token_getnum(instr->tokop3);
    else if(at == ARGS_R_R_R)
    {
        copy = *instr->tokop3;
        if(instr->tokop3->len > 1)
        {
            copy.str++;
            instr->op3 = token_getnum(&copy);
        }
        else
            printf("error: %s is not a register\n", copy.str);
    }
}

/* When a mnemonic maps to multiple ops, ensure we use the right one. */
void op_fix(instr_t *instr)
{
    int op = instr->op;
    /* DRW */
    if(op == 0x05 && instr->tokop3 != NULL)
    {
        instr->args = ARGS_R_R_R;
        instr->op = 0x06;
    }
    else if(op == 0x20 && instr->tokop1 != NULL &&
            !strcmp(instr->tokop1->str, "sp"))
    {
        instr->op = 0x21;
    }
    else if(op == 0x22 && instr->tokop2 != NULL && instr->tokop2->len > 2 &&
            instr->tokop2->str[0] == 'r')
    {
        instr->args = ARGS_R_R;
        instr->op = 0x23;
    }
    else if(op == 0x30 && instr->args == ARGS_R_R)
        instr->op = 0x31;
    else if(op == 0x41 && instr->args == ARGS_R_R_R)
        instr->op = 0x42;
    else if(op == 0x51 && instr->args == ARGS_R_R_R)
        instr->op = 0x52;
    else if(op == 0x61 && instr->args == ARGS_R_R_R)
        instr->op = 0x62;
    else if(op == 0x71 && instr->args == ARGS_R_R_R)
        instr->op = 0x72;
    else if(op == 0x81 && instr->args == ARGS_R_R_R)
        instr->op = 0x82;
    else if(op == 0x91 && instr->args == ARGS_R_R_R)
        instr->op = 0x92;
    else if(op == 0xA1 && instr->args == ARGS_R_R_R)
        instr->op = 0xA2;
    else if(op == 0xA4 && instr->args == ARGS_R_R_R)
        instr->op = 0xA5;
    else if(op == 0xA7 && instr->args == ARGS_R_R_R)
        instr->op = 0xA8;
    else if(op == 0xB0 && instr->args == ARGS_R_R)
        instr->op = 0xB3;
    else if(op == 0xB1 && instr->args == ARGS_R_R)
        instr->op = 0xB4;
    else if(op == 0xB2 && instr->args == ARGS_R_R)
        instr->op = 0xB5;
    else if(op == 0xE1 && instr->args == ARGS_R_R)
        instr->op = 0xE2;
    else if(op == 0xE4 && instr->args == ARGS_R_R)
        instr->op = 0xE5;
}

/*
 * Replace all (unresolved) symbols with their values.
 * In the case of labels, if unassigned give it its initial value.
 */
int syms_replace(instr_t *instrs, int ni, symbol_t *syms, int ns)
{
    int i, s, ret;
    ret = 0;
    /* First loop: replace all labels with their offset values. */
    for(i = 0; i < ni; ++i)
    {
        for(s = 0; s < ns; ++s)
        {
            instr_t *instr = &instrs[i];
            if(instr->valid && instr->islabel && instr->toklabel != NULL &&
               strcmp(instr->toklabel->str, syms[s].str) == 0)
            {
                /* FIXME: Temporary placeholder */
                if(syms[s].val != -1)
                {
                    log_error("", instr->ln, ERR_LABEL_REDEF, syms[s].str);
                    ret = -1;
                }
                syms[s].val = 0xffff;
            }
        }
    }
    /* Second loop: replace all references to symbols with values. */
    for(i = 0; i < ni; ++i)
    {
        for(s = 0; s < ns; ++s)
        {
            instr_t *instr = &instrs[i];
            if(!instr->valid || instr->islabel || instr->iscomment)
            {
                continue;
            }
            if(instr->tokop1 && strcmp(instr->tokop1->str, syms[s].str) == 0)
            {
                instr->op1 = syms[s].val;
                if(instr->args != ARGS_I &&
                   instr->args != ARGS_I_I)
                {
                    log_error("", instr->ln, ERR_NOT_REG, syms[s].str);
                    ret = -1;
                }
            }
            if(instr->tokop2 && strcmp(instr->tokop2->str, syms[s].str) == 0)
            {
                instr->op2 = syms[s].val;
                if(instr->args != ARGS_I_I &&
                   instr->args != ARGS_R_I)
                {
                    log_error("", instr->ln, ERR_NOT_REG, syms[s].str);
                    ret = -1;
                }
            }
            if(instr->tokop3 && strcmp(instr->tokop3->str, syms[s].str) == 0)
            {
                instr->op3 = syms[s].val;
                if(instr->args != ARGS_R_R_I)
                {
                    log_error("", instr->ln, ERR_NOT_REG, syms[s].str);
                    ret = -1;
                }
            }
        }
    }
    return ret;
}

int instr_isequ(instr_t *instr)
{
    return (instr->valid &&
            instr->tokmnem != NULL && instr->tokop1 != NULL &&
            instr->tokop2 != NULL && instr->tokop1->len == 4 &&
            instr->tokop1->str[0] == 'e' &&
            instr->tokop1->str[1] == 'q' &&
            instr->tokop1->str[2] == 'u');
}

int instr_parse(instr_t *instr)
{
    string_t *toktemp;
    char *sp;
   
    if(!instr->valid)
        return -1;

    sp = instr->line->str;
    
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
        syms[symind].str = instr->toklabel->str;
        syms[symind].val = -1;
        syms[symind++].islabel = 1;
    }

    /* Mnemonic */
    if(instr->toklabel != NULL)
    {
        if(*sp == '\0')
        {
            instr->islabel = 1;
            return 0;
        }
        toktemp = token_next(&sp);
        if(token_islabel(toktemp))
        {
            instr->islabel = 1;
            return 0;
        }
        instr->tokmnem = toktemp;
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
        syms[symind].str = instr->tokmnem->str;
        syms[symind++].val = token_getnum(instr->tokop2);
        instr->isequ = 1;
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
    char line[200];
    int ln = 0, len = 0, i;
    
    /* TODO: Proper argument parsing */
    if(argc > 1)
    {
        file = fopen(argv[1], "r");
        fgets(line, 200, file);

        while(line != NULL && !feof(file))
        {
            int c;

            ++ln;
            len = strlen(line);
            line[len - 1] = '\0';
            instrs[ln - 1].valid = 0;
            for(c = 0; c < len; c++)
            {
                if(line[c] != ' ' && line[c] != '\t' && line[c] != '\0' )
                {
                    instrs[ln - 1].valid = 1;
                    break;
                }
            }
            
            instrs[ln - 1].line = string_alloc(line, len);
            instrs[ln - 1].ln = ln;
            if(instrs[ln - 1].valid)
            {
                instr_parse(&instrs[ln - 1]);
                /* Only consider lines with "proper" instructions. */
                if(!instrs[ln - 1].islabel && !instrs[ln - 1].iscomment &&
                    !instr_isequ(&instrs[ln - 1]) && instrs[ln - 1].tokmnem != 0)
                {
                    instrs[ln - 1].op = token_mnem2op(instrs[ln - 1].tokmnem);
                    instrs[ln - 1].args = op_getargsformat(&instrs[ln - 1]);
                    op_fix(&instrs[ln - 1]);
                    op_getops(&instrs[ln - 1]);
                }
            }
            fgets(line, 200, file);
        }
        
        syms_replace(instrs, ln, syms, symind);

        printf("Symbol table\n------------\n");
        for(i = 0; i < symind; ++i)
        {
            printf("%02d: [ '%s' : %d / 0x%x ]\n",
                   i, syms[i].str, (short)syms[i].val, (uint16_t)syms[i].val);
        }
        printf("\nInstruction table\n-----------------\n");
        for(i = 0; i < ln; ++i)
        {
            printf("%02d: ", instrs[i].ln);
            if(!instrs[i].valid)
                printf("[invalid          ]");
            else if(instrs[i].iscomment)
                printf("[comment          ]");
            else if(instrs[i].islabel)
                printf("[label            ]");
            else if(instrs[i].isequ)
                printf("[equ              ]");
            else
                printf("%d [%02x %4x %4x %4x]",
                       instrs[i].args,
                       (uint16_t)instrs[i].op,
                       (uint16_t)instrs[i].op1,
                       (uint16_t)instrs[i].op2,
                       (uint16_t)instrs[i].op3);
            
            printf(" %s\n", instrs[i].line->str);
        }
    }
        
    exit(0);
}

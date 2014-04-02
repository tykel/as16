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
    case ARGS_SP_I:
    {
        if(instr->tokop1 != NULL && instr->tokop1->len > 2 &&
           instr->tokop1->str[0] == 's' && instr->tokop1->str[1] == 'p' &&
           instr->tokop2 != NULL)
        {
            af = ARGS_SP_I;
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
    instr_args_t at = instr->args;
    if(at == ARGS_NONE || at == ARGS_ERR)
        return;

    /* First argument. */
    if(at == ARGS_I || at == ARGS_I_I)
        instr->op1 = token_getnum(instr->tokop1);
    else if(at == ARGS_R || at == ARGS_R_I || at == ARGS_R_R ||
            at == ARGS_R_R_I || at == ARGS_R_R_R)
    {
        if(instr->tokop1->len == 3)
            instr->op1 = token_getreg(instr->tokop1);
        else
            printf("error: %s is not a register\n", instr->tokop1->str);
    }

    /* Second argument. */
    if(at == ARGS_I_I || at == ARGS_R_I)
        instr->op2 = token_getnum(instr->tokop2);
    else if(at == ARGS_R_R || at == ARGS_R_R_I || at == ARGS_R_R_R)
    {
        if(instr->tokop2->len == 3)
            instr->op2 = token_getreg(instr->tokop2);
        else
            printf("error: %s is not a register\n", instr->tokop2->str);
    }
    
    /* Third argument. */
    if(at == ARGS_R_R_I)
        instr->op3 = token_getnum(instr->tokop3);
    else if(at == ARGS_R_R_R)
    {
        if(instr->tokop3->len == 3)
            instr->op3 = token_getreg(instr->tokop3);
        else
            printf("error: %s is not a register\n", instr->tokop3->str);
    }
}

/* When a mnemonic maps to multiple ops, ensure we use the right one. */
void op_fix(instr_t *instr)
{
    int op = instr->op;
    /* DRW */
    if(op == 0x20 && instr->tokop1 != NULL &&
            !strcmp(instr->tokop1->str, "sp"))
    {
        instr->op++;
        instr->args = ARGS_SP_I;
    }
    else if(op == 0x22 && instr->tokop2 != NULL && instr->tokop2->len > 2 &&
            instr->tokop2->str[0] == 'r')
    {
        instr->op++;
        instr->args = ARGS_R_R;
    }
    else if((op == 0x05 || op == 0x30 || op == 0x41 || op == 0x51 ||
             op == 0x61 || op == 0x71 || op == 0x81 || op == 0x91 ||
             op == 0xA1 || op == 0xA4 || op == 0xA8) && instr->tokop3 != NULL &&
            instr->tokop3->len > 2 && instr->tokop3->str[0] == 'r')
    {
        instr->op++;
        instr->args = ARGS_R_R_R;
    }
    else if((op == 0xB0 || op == 0xB1 || op == 0xB2) && instr->args == ARGS_R_R)
    {
        instr->op += 3;
        instr->args = ARGS_R_R;
    }
    else if((op == 0xE1 || op == 0xE4) && instr->tokop2 != NULL &&
            instr->tokop2->len > 2 && instr->tokop2->str[0] == 'r')
    {
        instr->op++;
        instr->args = ARGS_R_R;
    }
}

void op_gettype(instr_t *instr)
{
    switch(instr->args)
    {
    case ARGS_NONE:
        instr->type = INSTR_OP000000;
        break;
    case ARGS_I:
        if(instr->op1 < (1 << 4))
            instr->type = INSTR_OP000N00;
        else
            instr->type = INSTR_OP00LLHH;
        break;
    case ARGS_I_I:
        if(instr->op == 0x08)
            instr->type = INSTR_OP00000N;
        else if(instr->op == 0x0E)
            instr->type = INSTR_OPBBLLHH;
        else
            instr->type = INSTR_OPYXLLHH;
        break;
    case ARGS_R:
        instr->type = INSTR_OP0X0000;
        break;
    case ARGS_R_I:
        if((instr->op & 0xF0) == 0xB0)
            instr->type = INSTR_OP0X0N00;
        else
            instr->type = INSTR_OP0XLLHH;
        break;
    case ARGS_SP_I:
        instr->type = INSTR_OP0XLLHH;
        break;
    case ARGS_R_R:
        instr->type = INSTR_OPYX0000;
        break;
    case ARGS_R_R_I:
        instr->type = INSTR_OPYXLLHH;
        break;
    case ARGS_R_R_R:
        instr->type = INSTR_OPYX0Z00;
        break;
    default:
        break;
    }
}

/*
 * Replace all (unresolved) symbols with their values.
 * In the case of labels, if unassigned give it its initial value.
 */
int syms_replace(instr_t *instrs, int ni, symbol_t *syms, int ns)
{
    int i, s, ret;
    int cur;
    ret = 0;
    /* First loop: replace all labels with their offset values. */
    for(i = 0, cur = 0; i < ni; ++i)
    {
        instr_t *instr = &instrs[i];
        for(s = 0; s < ns; ++s)
        {
            if(instr->valid && instr->toklabel != NULL &&
               strcmp(instr->toklabel->str, syms[s].str) == 0)
            {
                if(syms[s].val != -1)
                {
                    log_error("", instr->ln, ERR_LABEL_REDEF, syms[s].str);
                    ret = -1;
                }
                syms[s].val = cur;
            }
        }
        if(instr->isdata)
            cur += instr->data_size;
        else if(instr->valid && !instr->iscomment && !instr->islabel &&
                !instr->isequ)
            cur += 4;
    }
    /* Second loop: replace all references to symbols with values. */
    for(i = 0; i < ni; ++i)
    {
        instr_t *instr = &instrs[i];
        if(!instr->valid || instr->islabel || instr->iscomment)
        {
            continue;
        }
        for(s = 0; s < ns; ++s)
        {
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
    int i, jc;
   
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
        if(token_iscomment(toktemp))
        {
            instr->islabel = 1;
            return 0;
        }
        instr->tokmnem = toktemp;
    }
    else
        instr->tokmnem = toktemp;

    /* Make first operand the condition code for jumps */
    jc = 0;
    if(instr->tokmnem->str[0] == 'j' || instr->tokmnem->str[0] == 'c')
    {
        for(i = 0; i < 16; ++i)
        {
            if(!strcmp(str_cond[i], instr->tokmnem->str + 1))
            {
                char temp[3] = { 'r', 0, 0 };
                temp[1] = i + (i < 10 ? '0' : 'a' - 10);
                instr->tokop1 = string_alloc(temp, 3);
                jc = 1;
            }
        }
    }
    for(i = jc; *sp != '\0' && i < MAX_OPS; ++i)
    {
        toktemp = token_next(&sp);
        if(token_iscomment(toktemp))
            return 0;
        
        instr->tokops[i] = toktemp;
    
        if(i == 0)
            instr->tokop1 = toktemp;
        else if(i == 1)
        {
            instr->tokop2 = toktemp;
            if((instr->tokop1->str[0] == 'e' || instr->tokop1->str[0] == 'E') &&
               (instr->tokop1->str[1] == 'q' || instr->tokop1->str[1] == 'Q') &&
               (instr->tokop1->str[2] == 'u' || instr->tokop1->str[2] == 'U'))
            {
                syms[symind].str = instr->tokmnem->str;
                syms[symind++].val = token_getnum(instr->tokop2);
                instr->isequ = 1;
            }
        }
        else if(i == 2)
            instr->tokop3 = toktemp;
    }
    instr->num_ops = i;

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

void expand_data(instr_t *instrs, int ni)
{
    int i;

    for(i = 0; i < ni; i++)
    {
        instr_t *it = &instrs[i];
        if(!it->valid || it->iscomment || it->islabel || it->isequ)
            continue;
        
        if(!strcmp(it->tokmnem->str, "db"))
        {
            int o;
            char *data;
            int size = it->num_ops;
            it->data = malloc(size);
            it->data_size = size;
            data = (char *) it->data;
            for(o = 0; o < it->num_ops; o++)
            {
                int n = token_getnum(it->tokops[o]);
                data[o] = (char) n;
            }
            
            it->isdata = 1;
        }
        else if(!strcmp(it->tokmnem->str, "dw"))
        {
            int o;
            short *data;
            int size = it->num_ops * 2;
            it->data = malloc(size);
            it->data_size = size;
            data = (short *) it->data;
            for(o = 0; o < it->num_ops; o++)
            {
                int n = token_getnum(it->tokops[o]);
                data[o] = (short) n;
            }

            it->isdata = 1;
        }
    }
}

void output_file(const char *fn, instr_t *instrs, int ni, symbol_t *syms,
     int ns)
{
    int i;
    FILE *file = NULL;

    if((file = fopen(fn, "wb")) == NULL)
    {
        fprintf(stderr, "error: could not open %s for writing\n", fn);
        exit(-1);
    }
    
    for(i = 0; i < ni; ++i)
    {
        instr_t *it = &instrs[i];
        if(!it->valid || it->iscomment || it->islabel || it->isequ)
            continue;
        if(it->isdata)
        {
            fwrite(it->data, 1, it->data_size, file);
        }
        else
        {
            unsigned char idw[4] = {0};
            idw[0] = it->op;
            switch(it->type)
            {
            case INSTR_OP000N00:
                idw[2] = (unsigned char) it->op1;
                break;
            case INSTR_OP00000N:
                idw[3] = (unsigned char) ((it->op1 << 1) | it->op2);
                break;
            case INSTR_OP00LLHH:
                *(short *)&idw[2] = it->op1;
                break;
            case INSTR_OP0X0000:
                idw[1] = (unsigned char) it->op1;
                break;
            case INSTR_OP0X0N00:
                idw[1] = (unsigned char) it->op1;
                idw[2] = (unsigned char) it->op2;
                break;
            case INSTR_OP0XLLHH:
                idw[1] = (unsigned char) it->op1;
                *(short *)&idw[2] = it->op2;
                break;
            case INSTR_OPYX0000:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                break;
            case INSTR_OPYXLLHH:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                *(short *)&idw[2] = it->op3;
                break;
            case INSTR_OPBBLLHH:
                idw[1] = (unsigned char) it->op1;
                *(short *)&idw[2] = it->op2;
                break;
            case INSTR_OPYX0Z00:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                idw[2] = it->op3;
                break;
            case INSTR_OP000000:
            default:
                break;
            }
            fwrite(idw, 1, 4, file);
        }
    }
    fclose(file);
}

int read_file(const char *fn, int base)
{
    FILE *file;
    char line[200];
    int ln, len;
    
    ln = 0;
    file = fopen(fn, "r");
    if(!file)
    {
        fprintf(stderr, "error: could not open %s for reading\n", fn);
        return ln;
    }

    fgets(line, 200, file);
    
    while(line != NULL && !feof(file))
    {
        int c;

        ++ln;
        len = strlen(line);
        line[len - 1] = '\0';
        instrs[ln + base - 1].valid = 0;
        for(c = 0; c < len; c++)
        {
            if(line[c] != ' ' && line[c] != '\t' && line[c] != '\0' )
            {
                instrs[ln + base - 1].valid = 1;
                break;
            }
        }
        
        instrs[ln + base - 1].line = string_alloc(line, len);
        instrs[ln + base - 1].ln = ln;
        if(instrs[ln + base - 1].valid)
        {
            instr_parse(&instrs[ln + base - 1]);
            if(instrs[ln + base - 1].tokmnem != NULL &&
               !strcmp(instrs[ln + base - 1].tokmnem->str, "include"))
                base += read_file(instrs[ln + base - 1].tokop1->str, ln + base);
            else
            {
                /* Only consider lines with "proper" instructions. */
                if(!instrs[ln + base - 1].islabel && !instrs[ln + base - 1].iscomment &&
                    !instr_isequ(&instrs[ln + base - 1]) && instrs[ln + base - 1].tokmnem != 0)
                {
                    instrs[ln + base - 1].op = token_mnem2op(instrs[ln + base - 1].tokmnem);
                    instrs[ln + base - 1].args = op_getargsformat(&instrs[ln + base - 1]);
                    op_fix(&instrs[ln + base - 1]);
                    op_getops(&instrs[ln + base - 1]);
                    op_gettype(&instrs[ln + base - 1]);
                }
            }
        }
        fgets(line, 200, file);
    }

    return ln;
}

int main(int argc, char *argv[])
{
    int verbose, ln, i;    

    if(argc > 1)
    {
        for(i = 1; i < argc; i++)
        {
            if(argv[i][0] == '-')
            {
                if(!strcmp(argv[i], "-v"))
                    verbose = 1;
            }
        }
        for(i = 1, ln = 0; i < argc; i++)
            if(argv[i][0] != '-')
                ln += read_file(argv[i], ln);
    }

    expand_data(instrs, ln);
    syms_replace(instrs, ln, syms, symind);

    if(verbose)
    {
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
                printf("  [invalid          ]");
            else if(instrs[i].iscomment)
                printf("  [comment          ]");
            else if(instrs[i].islabel)
                printf("  [label            ]");
            else if(instrs[i].isequ)
                printf("  [equ              ]");
            else if(instrs[i].isdata)
                printf("  [data: %3d bytes  ]", instrs[i].data_size);
            else
                printf("%d [%02x %4x %4x %4x]",
                       instrs[i].args,
                       (uint16_t)instrs[i].op,
                       (uint16_t)instrs[i].op1,
                       (uint16_t)instrs[i].op2,
                       (uint16_t)instrs[i].op3);
            
            printf(" %s\n", instrs[i].line->str);
        }
        
        output_file("output.c16", instrs, ln, syms, symind);
    }
        
    exit(0);
}

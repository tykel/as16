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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "instr.h"
#include "token.h"
#include "format.h"
#include "strings.h"

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
            log_error(instr->fn, instr->ln, ERR_NOT_REG, instr->tokop1->str);
    }
    else if(at == ARGS_SP_I)
    {
        instr->op1 = 0;
    }

    /* Second argument. */
    if(at == ARGS_I_I || at == ARGS_R_I || at == ARGS_SP_I)
        instr->op2 = token_getnum(instr->tokop2);
    else if(at == ARGS_R_R || at == ARGS_R_R_I || at == ARGS_R_R_R)
    {
        if(instr->tokop2->len == 3)
            instr->op2 = token_getreg(instr->tokop2);
        else
            log_error(instr->fn, instr->ln, ERR_NOT_REG, instr->tokop2->str);
    }
    
    /* Third argument. */
    if(at == ARGS_R_R_I)
        instr->op3 = token_getnum(instr->tokop3);
    else if(at == ARGS_R_R_R)
    {
        if(instr->tokop3->len == 3)
            instr->op3 = token_getreg(instr->tokop3);
        else
            log_error(instr->fn, instr->ln, ERR_NOT_REG, instr->tokop3->str);
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
    else if(op == 0x14 && instr->tokop1 != NULL &&
            instr->tokop1->len > 2 && instr->tokop1->str[0] == 'r')
    {
       instr->op = 0x18;
       instr->args = ARGS_R;
    }
    else if((op == 0x22 || op == 0x30) && instr->tokop2 != NULL &&
            instr->tokop2->len > 2 && instr->tokop2->str[0] == 'r')
    {
        instr->op++;
        instr->args = ARGS_R_R;
    }
    else if((op == 0x05 || op == 0x41 || op == 0x51 ||
             op == 0x61 || op == 0x71 || op == 0x81 || op == 0x91 ||
             op == 0xA1 || op == 0xA4 || op == 0xA8) && instr->tokop3 != NULL &&
            instr->tokop3->len > 2 && instr->tokop3->str[0] == 'r')
    {
        instr->op++;
        instr->args = ARGS_R_R_R;
    }
    else if((op == 0xB0 || op == 0xB1 || op == 0xB2) && instr->tokop2 != NULL && instr->tokop2->str[0] == 'r')
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

/* Match an argument format to instruction encoding. */
void op_gettype(instr_t *instr)
{
    switch(instr->args)
    {
    case ARGS_NONE:
        instr->type = INSTR_OP000000;
        break;
    case ARGS_I:
        if(instr->op1 == 0x03)
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

/* Returns whether the given instruction is a constant definition. */
int instr_isequ(instr_t *instr)
{
    return (instr->valid &&
            instr->tokmnem != NULL && instr->tokop1 != NULL &&
            instr->tokop2 != NULL && instr->tokop1->len == 4 &&
            instr->tokop1->str[0] == 'e' &&
            instr->tokop1->str[1] == 'q' &&
            instr->tokop1->str[2] == 'u');
}

/* Parse a line into an instruction. */
int instr_parse(instr_t *instr, symbol_t *syms, int *num_syms)
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
        string_free(toktemp);
        return 0;
    }
    if(token_islabel(toktemp))
    {
        instr->toklabel = token_getlabel(toktemp);
        syms[*num_syms].str = instr->toklabel->str;
        syms[*num_syms].val = INT_MIN;
        syms[(*num_syms)++].islabel = 1;
    }

    /* Mnemonic */
    if(instr->toklabel != NULL)
    {
        string_free(toktemp);
        if(*sp == '\0')
        {
            instr->islabel = 1;
            return 0;
        }
        toktemp = token_next(&sp);
        if(token_iscomment(toktemp) || token_iswhitespace(toktemp))
        {
            instr->islabel = 1;
            string_free(toktemp);
            return 0;
        }
        instr->tokmnem = toktemp;
    }
    else
        instr->tokmnem = toktemp;
    
    if(!strcmp(instr->tokmnem->str, "db") || !strcmp(instr->tokmnem->str, "dw"))
        instr->isdata = 1;
    if(!strcmp(instr->tokmnem->str, "resb") || !strcmp(instr->tokmnem->str, "resw"))
        instr->isdata = 1;

    /* Make first operand the condition code for jumps */
    jc = 0;
    if(instr->tokmnem->str[0] == 'j' || instr->tokmnem->str[0] == 'c')
    {
        for(i = 0; i < 16; ++i)
        {
            if(!strcmp(str_cond[i], instr->tokmnem->str + 1) ||
               !strcmp(str_cond_alt[i], instr->tokmnem->str + 1))
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
        if(token_iscomment(toktemp) || token_iswhitespace(toktemp))
        {
            string_free(toktemp);
            break;
        }
        
        instr->tokops[i] = toktemp;
    
        if(i == 0)
            instr->tokop1 = toktemp;
        else if(i == 1)
        {
            instr->tokop2 = toktemp;
            if(instr->tokop1->len > 3 &&
               (instr->tokop1->str[0] == 'e' || instr->tokop1->str[0] == 'E') &&
               (instr->tokop1->str[1] == 'q' || instr->tokop1->str[1] == 'Q') &&
               (instr->tokop1->str[2] == 'u' || instr->tokop1->str[2] == 'U'))
            {
                syms[*num_syms].str = instr->tokmnem->str;
                syms[*num_syms].strval = instr->tokop2->str;
                syms[(*num_syms)++].val = token_getnum(instr->tokop2);
                instr->isequ = 1;
            }
        }
        else if(i == 2)
            instr->tokop3 = toktemp;
    }
    
    instr->num_ops = i;

    return 0;
}

/*
 * Replace all (unresolved) symbols with their values.
 * In the case of labels, if unassigned give it its initial value.
 */
int syms_replace(instr_t *instrs, int ni, symbol_t *syms, int ns,
        import_t *imports, int *start)
{
    int i, s, ret;
    int cur;
    import_t *ipt = imports;
    ret = 0;

    /* First loop: replace all labels with their offset values. */
    for(i = 0, cur = 0; i < ni; ++i)
    {
        instr_t *instr = &instrs[i];
        for(s = 0; s < ns; ++s)
        {
            /* A straightforward label -- map to current offset. */
            if(instr->valid && instr->toklabel != NULL &&
               strcmp(instr->toklabel->str, syms[s].str) == 0)
            {
                if(syms[s].val != INT_MIN)
                {
                    log_error(instr->fn, instr->ln, ERR_LABEL_REDEF, syms[s].str);
                    ret = -1;
                }
                syms[s].val = cur;
                break;
            }
            /* A string length constant.
             * Find the instruction whose label is used, then find associated
             * string and use its length. */
            else if(instr->isequ && !strcmp(instr->tokmnem->str, syms[s].str) &&
                    instr->tokop2->len > 3 && instr->tokop2->str[0] == '$' &&
                    instr->tokop2->str[1] == '-')
            {
                int j;
                
                for(j = 0; j < ni; ++j)
                {
                    if(j == i)
                        continue;
                    if(instrs[j].toklabel &&
                       !strcmp(instrs[j].toklabel->str, instr->tokop2->str + 2))
                    {
                        if(instrs[j].islabel && ni > j + 1 &&
                           instrs[j + 1].isdata == DATA_STR)
                            syms[s].val = strlen(instrs[j + 1].data);
                        else if(instrs[j].isdata == DATA_STR)
                            syms[s].val = strlen(instrs[j].data);
                        
                        break;
                    }
                }
            }
        }
        /* Only advance if we have encountered either a data definition or an 
         * instruction. */
        if(instr->isdata)
            cur += instr->data_size;
        else if(instr->valid && !instr->iscomment && !instr->islabel &&
                !instr->isequ)
            cur += 4;
    }

    /* Second loop: put values for import label offsets. */
    while(ipt != NULL)
    {
        for(s = 0; s < ns; s++)
        {
            if(!strcmp(syms[s].str, ipt->label))
            {
                syms[s].val = cur;
                cur += ipt->len;
                break;
            }
        }
        ipt = ipt->next;
    }

    /* Third loop: replace all references to symbols with values. */
    for(i = 0; i < ni; ++i)
    {
        instr_t *instr = &instrs[i];
        if(!instr->valid || instr->islabel || instr->iscomment || instr->isequ)
        {
            if(!instr->isstart)
                continue;
        }
        if(instr->isdata == DATA_BIN)
        {
            int d;
            for(d = 0; d < instr->num_ops; ++d)
            {
                if(token_getnum(instr->tokops[d]) != INT_MIN)
                    continue;
                for(s = 0; s < ns; ++s)
                {
                    if(!strcmp(instr->tokops[d]->str, syms[s].str))
                    {
                        if(instr->data_size == instr->num_ops)
                            ((char *)instr->data)[d] = (char)syms[s].val;
                        else
                            ((short *)instr->data)[d] = (short)syms[s].val;
                        break;
                    }
                }
                if(s == ns)
                    log_error(instr->fn, instr->ln, ERR_NOT_LABEL,
                            instr->tokops[d]->str);
            }
        }
        else if(instr->isdata != DATA_STR)
        {
            for(s = 0; s < ns; ++s)
            {
                if(instr->tokop1 && strcmp(instr->tokop1->str, syms[s].str) == 0)
                {
                    instr->op1 = syms[s].val;
                    if(instr->isstart)
                        *start = instr->op1;
                    else if(instr->args != ARGS_I && instr->args != ARGS_I_I)
                    {
                        log_error(instr->fn, instr->ln, ERR_NOT_REG, syms[s].str);
                        ret = -1;
                    }
                }
                if(instr->tokop2 && strcmp(instr->tokop2->str, syms[s].str) == 0)
                {
                    instr->op2 = syms[s].val;
                    if(instr->args != ARGS_I_I &&
                       instr->args != ARGS_R_I)
                    {
                        log_error(instr->fn, instr->ln, ERR_NOT_REG, syms[s].str);
                        ret = -1;
                    }
                }
                if(instr->tokop3 && strcmp(instr->tokop3->str, syms[s].str) == 0)
                {
                    instr->op3 = syms[s].val;
                    if(instr->args != ARGS_R_R_I)
                    {
                        log_error(instr->fn, instr->ln, ERR_NOT_REG, syms[s].str);
                        ret = -1;
                    }
                }
            }
            if(instr->op1 == INT_MIN)
                log_error(instr->fn, instr->ln, ERR_NOT_LABEL, instr->tokop1->str);
            if(instr->op2 == INT_MIN)
                log_error(instr->fn, instr->ln, ERR_NOT_LABEL, instr->tokop2->str);
            if(instr->op3 == INT_MIN)
                log_error(instr->fn, instr->ln, ERR_NOT_LABEL, instr->tokop3->str);
        }
    }

    return ret;
}


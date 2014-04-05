#ifndef INSTR_H
#define INSTR_H

#define MAX_OPS 256

/* Possible argument combinations. */
typedef enum
{
    ARGS_ERR = -1,
    ARGS_NONE = 0,
    ARGS_I,
    ARGS_I_I,
    ARGS_R,
    ARGS_R_I,
    ARGS_SP_I,
    ARGS_R_R,
    ARGS_R_R_I,
    ARGS_R_R_R
} instr_args_t;

/* All the instruction encodings. */
typedef enum
{
    INSTR_OPERR = -1,
    INSTR_OP000000 = 0,
    INSTR_OP000N00,
    INSTR_OP00LLHH,
    INSTR_OP0X0000,
    INSTR_OP0X0N00,
    INSTR_OP00000N,
    INSTR_OP0XLLHH,
    INSTR_OPYX0000,
    INSTR_OPYXLLHH,
    INSTR_OPBBLLHH,
    INSTR_OPYX0Z00
} instr_type_t;

#include "tstring.h"
#include "defs.h"

/* Structure holding a mapping from label/constant to its
 * numerical value. */
typedef struct
{
    char *str;
    int val;
    int islabel;
} symbol_t;

/* Structure holding state of an instruction, both in textual
 * form and compiled form. */
typedef struct
{
    int valid;
    int ln;
    const char *fn;
    int iscomment, islabel, isequ, isdata;

    instr_args_t args;
    instr_type_t type;

    string_t *line;
    string_t *toklabel, *tokmnem, *tokops[MAX_OPS], *tokop1, *tokop2, *tokop3;
    int op, op1, op2, op3, num_ops, data_size;
    void *data;

    int addr;

} instr_t;


instr_args_t op_getargsformat(instr_t *instr);

void op_getops(instr_t *instr);

void op_fix(instr_t *instr);

void op_gettype(instr_t *instr);

int instr_isequ(instr_t *instr);

int instr_parse(instr_t *instr, symbol_t *syms, int *num_syms);

int syms_replace(instr_t *instrs, int ni, symbol_t *syms, int ns,
        import_t *imports);

#endif

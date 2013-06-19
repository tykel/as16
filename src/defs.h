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

#ifndef DEFS_H
#define DEFS_H


/* Error codes for the program. */
typedef enum
{
    ERR_FILE = -1,
    ERR_MALLOC = -2,
    ERR_NO_MNEMONIC = -3,
    ERR_NO_OP1 = -4,
    ERR_NO_OP2 = -5,
    ERR_NO_OP3 = -6
} err_t;


/* Possible argument combinations. */
typedef enum
{
    INSTR_ERR = -1,
    INSTR_NONE = 0,
    INSTR_IMM,
    INSTR_IMM_IMM,
    INSTR_REG,
    INSTR_REG_IMM,
    INSTR_REG_REG,
    INSTR_REG_REG_IMM,
    INSTR_REG_REG_REG
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
    INSTR_OP0XLLHH,
    INSTR_OPYX0000,
    INSTR_OPYXLLHH,
    INSTR_OPYX0Z00
} instr_type_t;

/* Help out with the string manip */
typedef struct
{
    char *str;
    int len;
} string_t;


/* Structure holding state of an instruction, both in textual
 * form and compiled form. */
typedef struct
{
    char *str;
    int len;
    int ln;
    int iscomment, islabel;

    instr_args_t args;
    instr_type_t type;

    string_t *toklabel, *tokmnem, *tokop1, *tokop2, *tokop3;
    int op, x, y, z, hhll, n;

    int addr;

} instr_t;

extern const instr_args_t op_args[256];
extern const instr_type_t op_types[256];

#endif


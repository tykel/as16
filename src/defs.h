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
    ERR_MALLOC = -2
} err_t;


/* Possible argument combinations. */
typedef enum
{
    INSTR_NONE,
    INSTR_IMM,
    INSTR_REG,
    INSTR_REG_IMM,
    INSTR_REG_REG,
    INSTR_REG_REG_IMM,
    INSTR_REG_REG_REG
} instr_args_t;


/* All the instruction encodings. */
typedef enum
{
    INSTR_OP000000,
    INSTR_OP000N00,
    INSTR_OP00LLHH,
    INSTR_OP0X0000,
    INSTR_OP0X0N00,
    INSTR_OP0XLLHH,
    INSTR_OPYX0000,
    INSTR_OPYXLLHH,
    INSTR_OPYX0Z00
} instr_type_t;


/* Structure holding state of an instruction, both in textual
 * form and compiled form. */
typedef struct
{
    const char *str;
    int ln;

    instr_args_t args;
    instr_type_t type;

    int x, y, z;
    int hhll, n;

    int addr;

} instr_t;


#endif

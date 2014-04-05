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

#include <inttypes.h>

#define DATA_BIN 1
#define DATA_STR 2

/* Error codes for the program. */
typedef enum
{
    ERR_NONE = 0,
    ERR_FILE = -1,
    ERR_MALLOC = -2,
    ERR_NO_MNEMONIC = -3,
    ERR_NO_OP1 = -4,
    ERR_NO_OP2 = -5,
    ERR_NO_OP3 = -6,
    ERR_LABEL_REDEF = -7,
    ERR_NOT_REG = -8,
    ERR_INVALID_OP = -9,
    ERR_BAD_ARGS = -10,
    ERR_NOT_LABEL = -11
} err_t;


/* Linked list for tracking binary imports in a file. */
struct import;
typedef struct import
{
    char *fn;
    int start;
    int len;
    char *label;

    struct import *next;
} import_t;

/* Output header format. */
typedef struct
{
    uint32_t magic;
    uint8_t  reserved;
    uint8_t  spec_ver;
    uint32_t rom_size;
    uint16_t start_addr;
    uint32_t crc32_sum;

} __attribute__((packed)) header_t;

#include "instr.h"

extern const instr_args_t op_args[256];
extern const instr_type_t op_types[256];

void log_error(const char *fn, int ln, err_t err, void *data);

#endif


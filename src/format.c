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

#include "format.h"

instr_args_t op_argsformat[256] =
{
    /* 0x */
    ARGS_NONE, ARGS_NONE, ARGS_NONE, ARGS_I, ARGS_I, ARGS_R_R_I, ARGS_R_R_R,
    ARGS_R_I, ARGS_I_I, ARGS_NONE, ARGS_I, ARGS_I, ARGS_I, ARGS_R_I, ARGS_I_I,
    /* 1x */
    ARGS_I, ARGS_R_R_I, ARGS_I, ARGS_R_R_I, ARGS_I, ARGS_NONE, ARGS_R, ARGS_I,
    ARGS_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 2x */
    ARGS_R_I, ARGS_R_I, ARGS_R_I, ARGS_R_R, ARGS_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 3x */
    ARGS_R_I, ARGS_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 4x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 5x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R, ARGS_R_I, ARGS_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 6x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R, ARGS_R_I, ARGS_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 7x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 8x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* 9x */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* Ax */
    ARGS_R_I, ARGS_R_R, ARGS_R_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* Bx */
    ARGS_R_I, ARGS_R_I, ARGS_R_I, ARGS_R_R, ARGS_R_R, ARGS_R_R,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, 
    /* Cx */
    ARGS_R, ARGS_R, ARGS_NONE, ARGS_NONE, ARGS_NONE, ARGS_NONE,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, 
    /* Dx */
    ARGS_I, ARGS_R, 
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    /* Ex */
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR,
    /* Fx */
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR, ARGS_ERR,
    ARGS_ERR, ARGS_ERR
};


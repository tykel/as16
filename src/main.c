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

#include "strings.h"

#include <stdio.h>
#include <stdlib.h>

#define ERR_FILE   -1
#define ERR_MALLOC -2

/* Read a binary file into an unallocated buffer. */
int file_read(const char *fn, void *buf)
{
    FILE *file = NULL;
    int len;

    if((file = fopen(fn, "rb")) == NULL)
        return ERR_FILE;
    fseek(fn, 0, SEEK_END);
    len = ftell(fn);
    fseek(fn, 0, SEEK_SET);

    if((buf = malloc(len)) == NULL)
        return ERR_MALLOC;
    if(fread(buf, 1, len, fn) < len)
        return ERR_FILE;
    fclose(fn);

    return len;
}

int main(int argc, char *argv[])
{
    printf("as16: hello world\n");
    exit(0);
}

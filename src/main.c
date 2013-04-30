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

    *buf = malloc(len);
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
        printf("file length: %d bytes\n", sz);

        line = strtok(buf, "\n");
        printf("file: %s\n", buf); 
        while(line != NULL)
        {
            is[ln - 1].str = line;
            is[ln - 1].ln = ln;
            printf("%02d: %s\n", is[ln - 1].ln, is[ln - 1].str);
            ++ln;
            line = strtok(NULL, "\n");
        }
        free(buf);
    }
        
    exit(0);
}

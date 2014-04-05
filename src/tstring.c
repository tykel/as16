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
#include <string.h>

#include "tstring.h"

/* Copy a string into a string_t structure, allocating memory as necessary. */
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

/* Free the string_t structure and its underlying C string. */
void string_free(string_t *str)
{
    free(str->str);
    free(str);
}


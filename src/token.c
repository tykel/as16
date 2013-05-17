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
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t* token_next(char **str)
{
    int len, i, offs;
    char *p;
    string_t *token;

    p = *str;
    len = offs = 0;
    /* Allocate string */
    token = malloc(sizeof(string_t));
    token->str = NULL;
    while (*p == ' ' || *p == '\t')
    {
        ++offs;
        ++p;
    }
    /* Determine token length */
    while (*p != ' ' && *p != '\t' && *p != '\0' && *p != '\n' && *p != ',')
    {
        ++len;
        ++p;
    }

    /* Copy string */
    token->str = malloc(len + 1);
    for (i = 0; i<len; ++i)
        token->str[i] = (*str)[offs+i];
    token->str[len] = '\0';
    token->len = len + 1;

    *str = p;
    while(**str == ' ' || **str == '\t' || **str == ',')
        ++*str;

    return token;
}

int token_islabel(string_t *str)
{
    int len = str->len;
    return (str->str[len - 2] == ':');
}

int token_iscomment(string_t *str)
{
    return (str->str[0] == ';');
}

int token_mnem2op(string_t *str)
{
    int i, c;
    
    for (i = 0; i < 256; ++i)
    {
        if (strcmp(str->str, str_ops[i]) == 0)
            return i;
        else if (str->str[i] == 'j' || str->str[i] == 'c')
        {
            for (c = 0; c < 16; ++c)
            {
                if (strcmp(str->str + 1, str_cond[c]) == 0)
                    return (str->str[i] == 'j' ? 0x12 : 0x17);
            }
        }
    }
    return -1;
}

string_t* token_getlabel(string_t *str)
{
    int i;
    string_t *label = NULL;

    label = malloc(sizeof(string_t));
    label->len = 0;
    if (!token_islabel(str))
        return label;
    label->str = malloc(str->len - 1);
    label->len = str->len - 1;
    for (i = 0; i < str->len-2; ++i)
        label->str[i] = str->str[i];
    label->str[str->len - 2] = '\0';
    return label;
}



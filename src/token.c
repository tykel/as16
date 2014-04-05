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
#include <limits.h>

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
    while (*p != ' ' && *p != '\t' && *p != '\0' && *p != '\n' && *p != ',' && *p != '\r')
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

int token_iswhitespace(string_t *str)
{
    char *sp = str->str;
    while(*sp != '\0')
        if(*sp != ' ' && *sp != '\n' && *sp != '\t' && *sp != '\r')
            return 0;
        else
            ++sp;
    return 1;
}

int token_islabel(string_t *str)
{
    int len = str->len;
    return (len > 1 && str->str[len - 2] == ':');
}

int token_iscomment(string_t *str)
{
    return (str->len > 1 && str->str[0] == ';');
}

int token_isreg(string_t *str)
{
    return (str->len == 3 && str->str[0] == 'r' &&
            ((str->str[1] >= '0' && str->str[1] <= '9') ||
             (str->str[1] >= 'a' && str->str[1] <= 'f')));
}

int token_mnem2op(string_t *str)
{
    int i, c;
    
    for (i = 0; i < 256; ++i)
    {
        if (strcmp(str->str, str_ops[i]) == 0)
            return i;
        else if (str->str[0] == 'j' || str->str[0] == 'c')
        {
            for (c = 0; c < 16; ++c)
            {
                if (!strcmp(str->str + 1, str_cond[c]) ||
                    !strcmp(str->str + 1, str_cond_alt[c]))
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

int token_getregindex(string_t *str)
{
    if (str == NULL || str->len <= 2)
        return -1;
    return str->str[1];
}

int token_getnum(string_t *str)
{
    int base, offs, result, neg;
    char *p;

    if (str == NULL || str->str == NULL || str->len < 2)
        return INT_MIN;
    /* Check for negative sign */
    if (str->str[0] == '-')
        neg = 1;
    else
        neg = 0;

    /* Determine base */
    offs = 0, base = 10;
    if (str->str[0 + neg] == '0' && str->len > 2)
    {
        /* Hexadecimal or binary (0x0\0, 0b0\0) */
        if (str->len >= 4 + neg)
        {
            if (str->str[1 + neg] == 'x')
            {
                base = 16;
                offs = 2;
            }
            else if (str->str[1 + neg] == 'b')
            {
                base = 2;
                offs = 2;
            }
            else if (str->str[1 + neg] >= '0' &&
                     str->str[1 + neg] < '8')
            {
                base = 8;
                offs = 1;
            }
            else
                return INT_MIN;
        }
        else if(str->len > 2 + neg)
        {
            base = 8;
            offs = 1;
        }
    }

    /* Set pointer to start of number proper */
    p = str->str + offs + neg;

    /* Convert number */
    for (result = 0; *p != '\0'; ++p)
    {
        result *= base;
        if ((base == 2 && *p >= '0' && *p < '2') ||
            (base == 8 && *p >= '0' && *p < '8') ||
            (base == 10 && *p >= '0' && *p <= '9'))
            result += (*p - '0');
        else if (base == 16)
        {
            if(*p >= '0' && *p <= '9')
                result += (*p - '0');
            else if(*p >= 'a' && *p <= 'f')
                result += 10 + (*p - 'a');
            else if(*p >= 'A' && *p <= 'F')
                result += 10 + (*p - 'A');
            else
                return INT_MIN;
        }
        else
            return INT_MIN;
    }
    return neg ? -result : result;
}

int token_getreg(string_t *str)
{
    int result = 0;
    if(str->len == 3)
    {
        char index = str->str[1];
        if(index >= '0' && index <= '9')
            result = index - '0';
        else if(index >= 'a' && index <= 'f')
            result = index - 'a' + 10;
    }
    return result;
}

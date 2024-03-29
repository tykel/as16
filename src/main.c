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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>

#include "defs.h"
#include "strings.h"
#include "format.h"
#include "tstring.h"
#include "token.h"
#include "crc.h"

/* Globals */
typedef unsigned short uint16_t;

#define VER_MAJOR 1
#define VER_MINOR 0
#define VER_REV   0

#define MAX_FILES 256
#define MAX_LINES 10000

static import_t *imports;
static include_t *includes;
static instr_t instrs[MAX_LINES];
static int num_instrs;
static symbol_t syms[MAX_LINES];
static int num_syms;

static char *files[MAX_FILES];
static int f;

static int start;
static unsigned char version = 0x13;


void log_error(const char *fn, int ln, err_t err, void *data)
{
    fprintf(stderr, "%s:%d: ", fn, ln);
    switch(err)
    {
    case ERR_FILE:
        fprintf(stderr, "error: could not open file '%s'\n", (char *)data);
        break;
    case ERR_MALLOC:
        fprintf(stderr, "error: malloc failed\n");
        break;
    case ERR_NO_MNEMONIC:
        fprintf(stderr, "error: instruction has no mnemonic\n");
        break;
    case ERR_NO_OP1:
        fprintf(stderr, "error: instruction missing operand 1\n");
        break;
    case ERR_NO_OP2:
        fprintf(stderr, "error: instruction missing operand 2\n");
        break;
    case ERR_NO_OP3:
        fprintf(stderr, "error: instruction missing operand 3\n");
        break;
    case ERR_LABEL_REDEF:
        fprintf(stderr, "error: label '%s' redefined\n", (char *)data);
        break;
    case ERR_NOT_REG:
        fprintf(stderr, "error: '%s' is not a register\n", (char *)data);
        break;
    case ERR_BAD_ARGS:
        fprintf(stderr, "error: incorrect argument(s)\n");
        break;
    case ERR_NOT_LABEL:
        fprintf(stderr, "error: unknown constant '%s'\n", (char *)data);
        break;
    case ERR_BAD_VER:
        fprintf(stderr, "error: invalid start address '%s' (should be M.m)\n",
                (char *)data);
        break;
    case ERR_INC_CYCLE:
        fprintf(stderr, "error: include cycle with file '%s'\n", (char *)data);
        break;
    default:
        fprintf(stderr, "error: unknown\n");
        break;
    }
}

/* Write the data and instructions in binary format to buf. */
static int output_file(unsigned char *buf, instr_t *instrs, int ni, symbol_t *syms,
     int ns, import_t *imports)
{
    int i, p;
    import_t *ipt;

    p = 0;
    for(i = 0; i < ni; ++i)
    {
        instr_t *it = &instrs[i];
        if(!it->valid || it->iscomment || it->islabel || it->isequ || it->isstart)
            continue;
        if(it->isdata)
        {
            memcpy(buf + p, it->data, it->data_size);
            p += it->data_size;
        }
        else
        {
            unsigned char idw[4] = {0};
            idw[0] = it->op;
            switch(it->type)
            {
            case INSTR_OP000N00:
                idw[2] = (unsigned char) it->op1;
                break;
            case INSTR_OP00000N:
                idw[3] = (unsigned char) ((it->op1 << 1) | it->op2);
                break;
            case INSTR_OP00LLHH:
                *(short *)&idw[2] = it->op1;
                break;
            case INSTR_OP0X0000:
                idw[1] = (unsigned char) it->op1;
                break;
            case INSTR_OP0X0N00:
                idw[1] = (unsigned char) it->op1;
                idw[2] = (unsigned char) it->op2;
                break;
            case INSTR_OP0XLLHH:
                idw[1] = (unsigned char) it->op1;
                *(short *)&idw[2] = it->op2;
                break;
            case INSTR_OPYX0000:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                break;
            case INSTR_OPYXLLHH:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                *(short *)&idw[2] = it->op3;
                break;
            case INSTR_OPBBLLHH:
                idw[1] = (unsigned char) it->op1;
                *(short *)&idw[2] = it->op2;
                break;
            case INSTR_OPYX0Z00:
                idw[1] = (unsigned char) (it->op1 | (it->op2 << 4));
                idw[2] = it->op3;
                break;
            case INSTR_OP000000:
            default:
                break;
            }

            memcpy(buf + p, idw, 4);
            p += 4;
        }
    }

    ipt = imports;
    while(ipt != NULL)
    {
        int rlen;
        FILE *ifile = NULL;
        
        ifile = fopen(ipt->fn, "rb");
        if(ifile == NULL)
        {
            fprintf(stderr, "error: could not import %s\n", ipt->fn);
            ipt = ipt->next;
            continue;
        }
        
        if(fseek(ifile, ipt->start, SEEK_SET))
            fprintf(stderr, "warning: invalid start position %d in %s\n",
                    ipt->start, ipt->fn);
        if((rlen = fread(buf + p, 1, ipt->len, ifile)) != ipt->len)
            fprintf(stderr, "warning: read %d bytes from %s (requested %d)\n",
                    rlen, ipt->fn, ipt->len);
        p += rlen;
        fclose(ifile);

        ipt = ipt->next;
    }

    return p;
}

/* Parse a whole file at a time -- recursively calling itself for includes. */
static int file_parse(char *fn, int base, import_t **imports)
{
    FILE *file;
    include_t *prev, *head;
    int ln, len;
    char line[200] = {0};
    
    ln = 0;
    file = fopen(fn, "r");
    if(!file)
    {
        fprintf(stderr, "error: could not open %s for reading\n", fn);
        return ln;
    }

    fgets(line, 200, file);
    
    while(!feof(file))
    {
        int c;
        char *s, *t;
        instr_t *it;

        ++ln;
        len = strlen(line);
        line[len - 1] = '\0';
        it = &instrs[ln + base - 1];
        it->valid = 0;
        it->fn = fn;
        t = fn;
        while((s = strchr(t, '/')) != NULL)
            t = s + 1;
        it->basedir = malloc((int)(t - fn) + 1);
        it->basedir[(int)(t - fn)] = '\0';
        strncpy(it->basedir, fn, (int)(t - fn));

        for(c = 0; c < len; c++)
        {
            if(line[c] != ' ' && line[c] != '\t' && line[c] != '\0' &&
               line[c] != '\r' && line[c] != '\n')
            {
                it->valid = 1;
                break;
            }
        }
        
        it->line = string_alloc(line, len);
        it->ln = ln;
        if(it->valid)
        {
            instr_parse(it, syms, &num_syms);
            /* Handle a start directive. */
            if(it->tokmnem != NULL &&
               !strcmp(it->tokmnem->str, "start"))
            {
                it->isstart = 1;
                start = token_getnum(it->tokop1);
                it->valid = 0;
            }
            /* Handle a version directive. */
            else if(it->tokmnem != NULL &&
                    !strcmp(it->tokmnem->str, "version"))
            {
                if(!it->tokop1 || it->tokop1->len <= 3)
                    log_error(it->fn, it->ln, ERR_BAD_VER, it->tokop1->str);
                else
                    version = (it->tokop1->str[0] - '0') << 4 |
                              (it->tokop1->str[2] - '0');
                it->valid = 0;
            }
            /* Handle an include directive. */
            else if(it->tokmnem != NULL &&
               !strcmp(it->tokmnem->str, "include"))
            {
                char *efn;
                /* Check for include cycle. */
                prev = head = includes;
                while(includes != NULL)
                {
                    prev = includes;
                    if(!strcmp(includes->fn, it->tokop1->str))
                    {
                        log_error(it->fn, it->ln, ERR_INC_CYCLE, includes->fn);
                        return 0;
                    }
                    includes = includes->next;
                }
                includes = malloc(sizeof(include_t));
                includes->fn = it->tokop1->str;
                includes->next = NULL;
                if(prev != NULL)
                    prev->next = includes;
                if(head != NULL)
                    includes = head;
    
                efn = calloc((t - fn) + it->tokop1->len, 1);
                strcat(efn, it->basedir);
                strcat(efn + (t - fn), it->tokop1->str);
                base += file_parse(efn, ln + base, imports);
                it->valid = 0;
            }
            /* Handle a binary import directive. */
            else if(it->tokmnem != NULL &&
                    !strcmp(it->tokmnem->str, "importbin"))
            {
                char *efn;
                import_t *ipt, *prev;
                ipt = *imports, prev = NULL;
                while(*imports != NULL)
                {
                    prev = *imports;
                    *imports = (*imports)->next;
                }
                *imports = malloc(sizeof(import_t));
                if(prev)
                    prev->next = *imports;
                
                efn = calloc((t - fn) + it->tokop1->len, 1);
                strcat(efn, it->basedir);
                strcat(efn + (t - fn), it->tokop1->str);
                (*imports)->fn = efn;
                (*imports)->start = token_getnum(it->tokop2);
                (*imports)->len = token_getnum(it->tokop3);
                (*imports)->label = it->tokops[3]->str;
                (*imports)->next = NULL;
               
                syms[num_syms].str = (*imports)->label;
                syms[num_syms].val = INT_MIN;
                syms[num_syms++].islabel = 1;

                if(ipt != NULL)
                    *imports = ipt;
                it->valid = 0;
            }
            /* Handle an inline data definition. */
            else if(it->isdata)
            {
                if(it->tokop1->str[0] == '"')
                {
                    char *string, *end;
                    int size;
                    string = strchr(it->line->str, '"');
                    end = strchr(string + 1, '"');
                    size = end - string - 1;
                    it->data = calloc(size, 1);
                    it->data_size = size;
                    memcpy(it->data, string + 1, size);
                    it->isdata = DATA_STR;
                }
                else
                {
                    int o, bs, size;
                    bs = 1;
                    if(it->tokmnem->str[1] == 'w')
                        bs = 2;
                    size = it->num_ops * bs;
                    it->data = malloc(size);
                    it->data_size = size;
                    if(bs == 1)
                    {
                        char *data;
                        data = (char *) it->data;
                        for(o = 0; o < it->num_ops; o++)
                        {
                            int n = token_getnum(it->tokops[o]);
                            data[o] = (char) n;
                        }
                    }
                    else
                    {
                        short *data;
                        data = (short *) it->data;
                        for(o = 0; o < it->num_ops; o++)
                        {
                            int n = token_getnum(it->tokops[o]);
                            data[o] = (short) n;
                        }
                    }
                    
                    it->isdata = DATA_BIN;
                }
            }
            /* Handle other cases: instructions, etc. */
            else
            {
                /* Only consider lines with "proper" instructions. */
                if(!it->islabel && !it->iscomment &&
                    !it->isequ && it->tokmnem != 0)
                {
                    it->op = token_mnem2op(it->tokmnem);
                    it->args = op_getargsformat(it);
                    op_fix(it);
                    if(it->args == ARGS_ERR)
                        log_error(it->fn, it->ln, ERR_BAD_ARGS, it->line->str);
                    op_getops(it);
                    op_gettype(it);
                }
            
            }
        }
        fgets(line, 200, file);
    }

    return ln + base;
}

/* Print the help text. */
static void print_help(void)
{
    printf("Usage: as16 SOURCE... [OPTION]...\n\n"
           "    Assemble SOURCE(s) to produce a Chip16 binary.\n\n");
    printf("File options:\n\n"
           "    -o DEST: output file to DEST\n"
           "    -z, --zero: if assembled code < 64KB, zero rest up to 64KB\n"
           "    -r, --raw: omit header, write only raw Chip16 ROM\n\n");
    printf("Information options:\n\n"
           "    -m, --mmap: output mmap.txt which displays the address of each"
           " label\n"
           "    -v, --verbose: switch to verbose output (default is silent)\n");
    printf("Miscellaneous options:\n\n"
           "    -h, --help: display this help text\n"
           "    --version: display version information and exit\n\n");
    printf("Please report bugs to <https://github.com/tykel/as16/issues>\n");
}

/* Print the version information. */
static void print_ver(void)
{
    printf("as16 %d.%d.%d -- a Chip16 assembler\n",
            VER_MAJOR, VER_MINOR, VER_REV);
}

/* Free the imports linked list. */
static void imports_free(void)
{
    import_t *cur, *next;

    cur = imports;
    if(cur == NULL)
        return;
    
    while(cur)
    {
        next = cur->next;
        free(cur->fn);
        free(cur);
        cur = next;
    }
}

/* Free the instructions array. */
static void instrs_free(void)
{
    int i;
    instr_t *it;
   
    it = &instrs[0];
    for(i = 0; i < num_instrs; ++i, ++it)
    {
        int o;

        string_free(it->line);
        string_free(it->toklabel);
        string_free(it->tokmnem);
        for(o = 0; o < it->num_ops; ++o)
            string_free(it->tokops[o]);
        if(it->data)
            free(it->data);
        free(it->basedir);
    }
    for(i = 0; i < f; ++i)
        free(files[i]);
}

/* Free the includes linked list. */
static void includes_free(void)
{
    include_t *cur, *next;

    cur = includes;
    while(cur != NULL)
    {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

int main(int argc, char *argv[])
{
    crc_t crc;
    header_t hdr;
    FILE *outfile;
    int verbose, raw, zero, mmap, help, ver, i, size;
    unsigned char *outbuf;
    char *output = "output.c16";
    int longindex, opt;
    struct option longopts[] = {
        { "--verbose", 0, NULL, 0 },
        { "--raw", 0, NULL, 0 },
        { "--zero", 0, NULL, 0 },
        { "--mmap", 0, NULL, 0 },
        { "--help", 0, NULL, 0 },
        { 0, 0, 0, 0 },
    };

    f = raw = verbose = zero = mmap = help = ver = size = 0;
    
    /* Get options and filenames, and parse the latter. */
    while ((opt = getopt_long(argc, argv, "vrzmho:", longopts, &longindex)) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                ;
            case 'r':
                raw = 1;
                break;
            case 'z':
                zero = 1;
                break;
            case 'm':
                mmap = 1;
                break;
            case 'h':
                help = 1;
                break;
            case 'o':
                output = optarg;
                break;
            default:
                print_help();
                exit(0);
        }
    }
    while (optind < argc)    
    {
        files[f] = malloc(strlen(argv[optind]) + 1);
        strcpy(files[f++], argv[optind++]);
    }
    /* If help text was requested, print that and do nothing else. */
    if(help)
    {
        print_help();
        exit(0);
    }
    /* Similarly for version information. */
    else if(ver)
    {
        print_ver();
        exit(0);
    }
    /* Otherwise, parse each file in order. */
    for(i = 0; i < f; i++)
        num_instrs += file_parse(files[i], num_instrs, &imports);
    /* If no file was supplied, complain and exit. */
    if(f == 0)
    {
        fprintf(stderr, "error: no input file specified\n");
        exit(1);
    }
    
    /* Replace symbols by their numeric values. */
    syms_replace(instrs, num_instrs, syms, num_syms, imports, &start);

    /* Register the cleanup functions at exit. */
    atexit(instrs_free);
    atexit(imports_free);
    atexit(includes_free);
    
    /* Output a listing of label mappings, if requested. */
    if(mmap)
    {
        FILE *mmfile = NULL;
        FILE *mmbfile = NULL;
        char mmbfn[100];
        char *cp = &mmbfn[99];

        /* Write human-readable file, first. */
        if((mmfile = fopen("mmap.txt", "w")) == NULL)
            fprintf(stderr, "error: could not write to mmap.txt\n");
        else
        {
            fprintf(mmfile, "Label memory mapping:\n");
            fprintf(mmfile, "---------------------\n\n");
            for(i = 0; i < num_syms; ++i)
            {
                if(syms[i].islabel)
                    fprintf(mmfile, " 0x%04x : %s\n", syms[i].val, syms[i].str);
            }
            fprintf(mmfile, "\n---------------------\n");
            fclose(mmfile);
        }
        
        /* Now write symbol file for use with emulators. */
        memset(mmbfn, 0, 100);
        strncpy(mmbfn, output, 100);
        while(cp > mmbfn && *cp != '.')
           cp--;
        if(cp <= mmbfn + 97)
        {
            *++cp = 's';
            *++cp = 'y';
            *++cp = 'm';
        }

        if((mmbfile = fopen(mmbfn, "wb")) == NULL)
           fprintf(stderr, "error: could not write to %s\n", mmbfn);
        else
        {
           int numlabels = 0;
           int j;
           struct {
              union {
                 uint16_t w[2];
                 uint32_t q;
              } d;
           } data;
           struct {
              char *str;
              uint16_t str_offs;
              uint16_t value;
           } labels[1024];
           
           for(j = 0; j < num_syms; j++)
           {
              if(syms[j].islabel)
              {
                 labels[numlabels].str = syms[j].str;
                 labels[numlabels].value = syms[j].val;
                 labels[numlabels].str_offs = numlabels > 0
                    ? labels[numlabels-1].str_offs + strlen(labels[numlabels-1].str) + 1: 0;
                 numlabels++;
              }
           }
           data.d.q = numlabels * 4 + 4;
           fwrite(&data, sizeof(data), 1, mmbfile);
           for(j = 0; j < numlabels; j++)
           {
              data.d.w[0] = labels[j].value;
              data.d.w[1] = labels[j].str_offs;
              fwrite(&data, sizeof(data), 1, mmbfile);
           }
           for(j = 0; j < numlabels; j++)
           {
              char zero = 0;
              size_t len = strlen(labels[j].str);
              fwrite(labels[j].str, len, 1, mmbfile);
              fwrite(&zero, 1, 1, mmbfile);
           }

           fclose(mmbfile);
        }
    }
    /* Output the symbol table, and internal representation of instructions, if
     * requested. */
    if(verbose)
    {
        printf("Symbol table\n------------\n");
        for(i = 0; i < num_syms; ++i)
        {
            printf("%02d: [ '%s' : %d / 0x%x ]\n",
                   i, syms[i].str, (short)syms[i].val, (uint16_t)syms[i].val);
        }

        printf("\nInstruction table\n-----------------\n");
        for(i = 0; i < num_instrs; ++i)
        {
            if(instrs[i].ln == 0)
                continue;

            printf("%02d: ", instrs[i].ln);
            if(!instrs[i].valid)
                printf("  [invalid          ]");
            else if(instrs[i].iscomment)
                printf("  [comment          ]");
            else if(instrs[i].islabel)
                printf("  [label            ]");
            else if(instrs[i].isequ)
                printf("  [equ              ]");
            else if(instrs[i].isdata)
                printf("  [data: %3d bytes  ]", instrs[i].data_size);
            else
                printf("%d [%02x %4x %4x %4x]",
                       instrs[i].args,
                       (uint16_t)instrs[i].op,
                       (uint16_t)instrs[i].op1,
                       (uint16_t)instrs[i].op2,
                       (uint16_t)instrs[i].op3);
            
            printf(" %s\n", instrs[i].line ? instrs[i].line->str : "");
        }
    }

    /* Write the program machine code. */
    outbuf = malloc(64 * 1024);
    if(outbuf == NULL)
    {
        fprintf(stderr, "error: could not allocate %d bytes for output\n",
                64*1024);
        exit(1);
    }
    size = output_file(outbuf, instrs, num_instrs, syms, num_syms, imports);
    
    /* Write the header. */
    crc = crc_init();
    crc = crc_update(crc, outbuf, size);
    crc = crc_finalize(crc);

    hdr.magic = 0x36314843;
    hdr.reserved = 0;
    hdr.spec_ver = version;
    hdr.rom_size = size;
    hdr.start_addr = start;
    hdr.crc32_sum = crc;

    /* Write the program, and header unless requested, to disk. */
    outfile = fopen(output, "wb");
    if(outfile == NULL)
    {
        fprintf(stderr, "error: could not write to %s\n", output);
        exit(0);
    }
    if(!raw)
        fwrite(&hdr, sizeof(header_t), 1, outfile);
    fwrite(outbuf, 1, size, outfile);
    if(zero && size < 64*1024)
    {
        uint8_t b = 0;
        for(i = size; i < 64*1024; ++i)
            fwrite(&b, 1, 1, outfile);
    }
    fclose(outfile);
        
    free(outbuf);
    
    exit(0);
}

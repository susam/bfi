/* Brainfuck interpreter */

/*
 * Copyright (c) 2008 Susam Pal
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VERSION "0.1"
#define COPYRIGHT "Copyright (c) 2008 Susam Pal"
#define AUTHOR      "Susam Pal"
#define LICENSE \
  "This is free software. You are permitted to redistribute and use it in\n" \
  "source and binary forms, with or without modification, under the terms\n" \
  "of the Simplified BSD License. See <http://susam.in/licenses/bsd/> for\n" \
  "the complete license."

#define ARRAY_SIZE          30000 /* Default number of cells */
#define STACK_SIZE          1000  /* Default loop stack size */
#define STACK_GROWTH_FACTOR 0.1   /* How much stack to add when its full */

void interpret(char *filename);
void help();
void version();

struct info_t {
    char *pname;         /* Process name */
    char *ifilename;     /* Input source code file name */
    size_t arr_size;     /* Number of byte array cells allocated */
} info;

/*
 * Parses the command line and invokes the functions to carry out the
 * requested operation.
 */
int main(int argc, char **argv)
{
    size_t i = 0;
    int verbose = 0;

    /* Set default interpreter options */
    if ((info.pname = strrchr(argv[0], '/')) != NULL) {
        info.pname++;         /* Pointer to basename part of the path */
    } else {
        info.pname = argv[0]; /* argv[0] contains the basename only */
    }
    info.ifilename = NULL;
    info.arr_size = ARRAY_SIZE;

    /* Parse command line and set options */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            info.arr_size = strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-v") == 0 ||
                   strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0) {
            help();
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-V") == 0 ||
                   strcmp(argv[i], "--version") == 0) {
            version();
            exit(EXIT_SUCCESS);
        } else {
            info.ifilename = argv[i];
        }
    }

    /* If input source code file name is not specified, exit */
    if (info.ifilename == NULL) {
        fprintf(stderr, "%s: No input files\n", info.pname);
        exit(EXIT_FAILURE);
    }

    interpret(info.ifilename);
    return 0;
}

/* 
 * Interprets the brainfuck source code in the given file and run it.
 */ 
void interpret(char *filename)
{
    FILE *src;                      /* Source code file */
    size_t loop = 0;                /* Nesting level of loops skipped */
    unsigned char *ptr;             /* Byte array cell pointer */
    fpos_t *stack;                  /* Loop stack for loops entered */
    size_t top = 0;                 /* Next free location in stack */
    size_t stack_size = STACK_SIZE; /* Size of the loop stack */
    unsigned int c;
    
    /* Initialize data structures */
    if ((ptr = calloc(info.arr_size, sizeof *ptr)) == NULL) {
        fprintf(stderr, "%s: Out of memory while creating byte array "
                        "cells of size %lu\n", info.pname, info.arr_size);
        exit(EXIT_FAILURE);
    }
    if ((stack = malloc(STACK_SIZE * sizeof *stack)) == NULL) {
        fprintf(stderr, "%s: Out of memory while creating loop stack "
                        "of size %lu\n", info.pname, STACK_SIZE);
        exit(EXIT_FAILURE);
    }

    /* Open source code file */
    if ((src = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "%s: %s: Could not read file\n",
                info.pname, filename);
        exit(EXIT_FAILURE);
    }

    /* Interpret and execute */
    while ((c = fgetc(src)) != EOF) {
        switch (c) {
        case '>':
            ptr++; 
            break;
        case '<':
            ptr--;
            break;
        case '+':
            (*ptr)++;
            break;
        case '-':
            (*ptr)--;
            break;
        case '.':
            putchar(*ptr);
            break;
        case ',':
            *ptr = getchar();
            break;
        case '[':
            if (*ptr) {
                if (top == stack_size) {
                    stack_size *= 1 + STACK_GROWTH_FACTOR;
                    if ((stack = realloc(stack, sizeof *stack *
                                                stack_size)) == NULL) {
                        fprintf(stderr, "%s: Out of memory while increasing "
                                        "loop stack to size: %lu\n",
                                info.pname, stack_size);
                        exit(EXIT_FAILURE);
                    }
                }
                /* Save the pointer to the command after [ */
                fgetpos(src, stack + top);
                top++;
            } else {
                /* Skip everything in the loop */
                loop++;
                while (loop && (c = fgetc(src)) != EOF) {
                    if (c == '[')
                        loop++;
                    else if (c == ']')
                        loop--;
                }
            }
            break;
        case ']':
            if (*ptr) {
                /* Loop back to the command after the previous [ */
                fsetpos(src, stack + top - 1);
            } else {
                /* Exit the current loop */
                top--;
            }
            break;
        }
    }
}

/*
 * Displays help.
 */
void help()
{
    printf("Usage: %s [OPTION] ... FILE\n\n", info.pname);
    printf("Options:\n");
    printf("  " "-s SIZE        "
           "Size of the array of byte cells\n");
    printf("  " "-d DEPTH       "
           "Size of the loop stack used for nested loops\n");
    printf("  " "-v, --verbose  "
           "Display functions and commands invoked\n");
    printf("  " "-h, --help     "
           "Display this help and exit\n");
    printf("  " "-V, --version  "
           "Output version information and exit\n");
    printf("\n");
    printf("Report bugs to <susam@susam.in>.\n");
}

/*
 * Displays version and copyright details.
 */
void version()
{
    printf("%s " VERSION "\n", info.pname);
    printf(COPYRIGHT "\n\n");
    printf(LICENSE "\n\n");
    printf("Written by " AUTHOR ".\n");
}

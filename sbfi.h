/** Simple BrainFuck Interpreter V3.0 -- Written by Maxime Rinoldo **/

#ifndef __SBFI_H__
#define __SBFI_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define ERROR_NO_ARGS       "you must specify a file"
#define ERROR_TOO_MANY_ARGS "you can't specify more than one file"
#define ERROR_ALLOC         "the memory could not be allocated"
#define ERROR_OPEN_FILE     "the file %s could not be opened"
#define ERROR_READ_FILE     "the file %s could not be read"
#define ERROR_ARRAY_SIZE    "the initial array size must be at least 1 cell"
#define ERROR_BRACKETS      "unmatched bracket at position %d"
#define ERROR_MEMORY        "attempt to reach the cell %d which is outside of the memory (0 - %d)"

#define NONE    0
#define EXTEND  1
#define ABORT   2
#define WRAP    3
#define BLOCK   4

#define CHUNK_SIZE 1024
#define PRINT_BUFFER(size) { write(1, &buffer, size); buffer_index = 0; }
#define MOVE_TO_NEXT goto *(instr[(int)code[++i]]);

#endif

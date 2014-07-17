/** Simple BrainFuck Interpreter V3.1 -- Written by Maxime Rinoldo **/

#ifndef __SBFI_H__
#define __SBFI_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

// Error messages

#define ERROR_NO_ARGS       "you must specify a file"
#define ERROR_TOO_MANY_ARGS "you can't specify more than one file"
#define ERROR_ALLOC         "the memory could not be allocated"
#define ERROR_OPEN_FILE     "the file %s could not be opened"
#define ERROR_READ_FILE     "the file %s could not be read"
#define ERROR_ARRAY_SIZE    "the initial array size must be at least 1 cell"
#define ERROR_BRACKETS      "unmatched bracket at position %d"
#define ERROR_MEMORY        "attempt to reach the cell %d which is outside of the memory (0 - %d)"

// Possible values for the MEMORY_BEHAVIOR macro

#define NONE    0
#define EXTEND  1
#define ABORT   2
#define WRAP    3
#define BLOCK   4

#if (MEMORY_BEHAVIOR == EXTEND)
    #define MOVE_POINTER extend_memory(&ptr0, &ptr, &array_size, mov[++i]);
#elif (MEMORY_BEHAVIOR == ABORT)
    #define MOVE_POINTER abort_memory(ptr0, &ptr, array_size, mov[++i]);
#elif (MEMORY_BEHAVIOR == WRAP)
    #define MOVE_POINTER wrap_memory(ptr0, &ptr, array_size, mov[++i]);
#elif (MEMORY_BEHAVIOR == BLOCK)
    #define MOVE_POINTER block_memory(ptr0, &ptr, array_size, mov[++i]);
#else
    #define MOVE_POINTER ptr += mov[++i];
#endif

// Macros used for the output buffer

#define CHUNK_SIZE 1024
#define PRINT_BUFFER(size) { write(1, &buffer, size); buffer_index = 0; }

/*
 * The magical computed goto : code[i] reads the next bytecode instruction,
 * which is then used as an index for instr, the array of label addresses,
 * and the address we get is accessed by the goto * (computed goto), which
 * allows to execute the program without conditional branches or function
 * call overheads. It's cool and fast *_*
 */

#define NEXT_INSTRUCTION MOVE_POINTER goto *(instr[(int)code[i]]);

#endif

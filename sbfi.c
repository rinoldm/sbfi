/** Simple BrainFuck Interpreter V3.1 -- Written by Maxime Rinoldo **/

/*
 * Use these macros to define the Brainfuck behavior.
 *
 * CELL can be any integer type (default : unsigned char).
 *
 * INITIAL_ARRAY_SIZE can be any positive integer (until
 * the machine's limitations are reached) (default : 30000).
 *
 * MEMORY_BEHAVIOR can be NONE, EXTEND, ABORT, WRAP or BLOCK
 * (default : NONE).
 *
 * EOF_INPUT_BEHAVIOR can be either NO_CHANGE, or the
 * integer that will be written to the cell if EOF is
 * encountered on input (default : NO_CHANGE).
 */

#define CELL                unsigned char
#define INITIAL_ARRAY_SIZE  30000
#define MEMORY_BEHAVIOR     NONE
#define EOF_INPUT_BEHAVIOR  NO_CHANGE

#include "sbfi.h"

int *mov;

void error(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stderr, "\nError : ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

void *xcalloc(size_t nmemb, size_t size)
{
    void *p;

    if (!(p = calloc(nmemb, size)))
        error(ERROR_ALLOC);
    return (p);
}

void *xrealloc(void *p, size_t size)
{
    if (!(p = realloc(p, size)))
       error(ERROR_ALLOC);
    return (p);
}

char *get_src(const char *filename)
{
    FILE *file;
    char *code;
    size_t char_nb;

    // We try to open the file with the given filename

    if (!(file = fopen(filename, "r")))
        error(ERROR_OPEN_FILE, filename);

   /*
    * We count the number of characters in the file
    * to allocate the correct amount of memory for
    * the char * that will contain the code
    */

    for (char_nb = 0; fgetc(file) != EOF; ++char_nb);
    rewind(file);
    code = xcalloc(char_nb + 1, sizeof(char));

    // We copy the content of the file into the char *code

    if (fread(code, 1, char_nb, file) < char_nb)
        error(ERROR_READ_FILE, filename);
    fclose(file);

    return (code);
}

void check_src(const char *code)
{
    int i;
    int n;

    // The cell array must be at least one cell long

    if (INITIAL_ARRAY_SIZE <= 0)
        error(ERROR_ARRAY_SIZE);

   /*
    * This counts the number of left and right brackets in the
    * Brainfuck program code.
    *
    * We first check if there is more right than left brackets
    * at any point in the program. If we do, it's a right bracket
    * mismatch and we raise an error. If we don't, but at the end
    * of the check the number of left and right brackets are not
    * the same, it means it's a left bracket mismatch, so we check
    * the count from the end to the beginning of the code in the
    * same way, to find the position of the mismatching bracket
    * (when we have more left than right brackets) and raise an
    * error.
    */

    for (i = 0, n = 0; n >= 0 && code[i]; ++i)
        n += (code[i] == '[' ? 1 : code[i] == ']' ? -1 : 0);
    if (n < 0)
        error(ERROR_BRACKETS, i);
    else if (n > 0)
    {
        for (n = 0; n <= 0; --i)
            n += (code[i] == '[' ? 1 : code[i] == ']' ? -1 : 0);
        error(ERROR_BRACKETS, i);
    }
}

void strip_comments(char *code)
{
    int i;
    int j;

    // Every character besides Brainfuck commands is a comment

    for (i = 0, j = 0; code[i]; ++i)
        if (strchr("+-<>[],.", code[i]))
            code[j++] = code[i];
    code[j] = '\0';
}

int match_pattern(const char *codeptr, const char *pattern)
{
    size_t i;
    for (i = 0; i < strlen(pattern) && codeptr[i] == pattern[i]; ++i);
    return (i == strlen(pattern));
}

int *optim_code(char *code)
{
    // These int arrays will allow us to compress commands

    int *coeff = xcalloc(strlen(code), sizeof(int));
    mov = xcalloc(strlen(code), sizeof(int));

   /*
    * Compress consecutive +/-/</> into single commands
    * associated with the real count in the coeff array
    *
    * For example, +++++++ becomes + with 7
    *
    * We can compress a bit more by exploiting the symmetry
    * of similar commands, by changing +/- and >/< into a
    * single command and negating the associated integer in
    * the coeff array if the "direction" is negative
    *
    * For example, + with 7 becomes c with 7, while - with 5
    * becomes c with -5
    *
    * +/- and >/< are transformed into c and p respectively
    */

    size_t i;
    size_t j;

    for (i = 0; code[i]; ++i)
    {
        if (code[i] == '-' || code[i] == '<')
            coeff[i] = -1;
        else
            coeff[i] = 1;
        if (code[i] == '+' || code[i] == '-')
            code[i] = 'c';
        else if (code[i] == '<' || code[i] == '>')
            code[i] = 'p';
    }

    for (i = 0, j = 0; code[i]; ++j)
    {
        code[j] = code[i];
        coeff[j] = coeff[i];
        if (strchr("cp", code[++i]))
            for (; code[j] == code[i]; coeff[j] += coeff[i++]);
    }
    code[j] = '\0';

   /*
    * Optimization of simple Brainfuck constructs that
    * we can replace with a single command with a coeff.
    * We overwrite the excess characters with spaces that
    * we will remove afterwards.
    *
    * TODO: optimize more complex constructs. Note : I tried
    * to optimize "simple loops" since it should be a huge
    * improvement, but unfortunately it made the program
    * far slower. I'll continue to search a better solution.
    *
    * TODO: make the parser cleaner (without spaces)
    */

    for (i = 0; code[i]; ++i)
    {
        // [-] sets a cell to zero

        if (match_pattern(code + i, "[c]") && coeff[i + 1] == -1)
        {
            code[i] = '0';
            code[i + 1] = code[i + 2] = ' ';
        }

        // [>] / [<] stop at the first zero cell they encounter

        else if (match_pattern(code + i, "[p]"))
        {
            code[i] = 's';
            code[i + 1] = code[i + 2] = ' ';
            coeff[i] = coeff[i + 1];
        }

       /*
        * Loops of the form [-p+p], with the two pointer movements
        * being opposite, add the current cell to the one accessed
        * by the pointer movements and set the current cell to zero.
        *
        * Here, the coeff will be the size of the pointer movement.
        *
        * TODO: add [p-p+] without destroying the performance for long.b :c
        */

        else if (match_pattern(code + i, "[cpcp]") && coeff[i + 1] == -1 && coeff[i + 3] == 1 && coeff[i + 2] == -coeff[i + 4])
        {
            code[i] = 'm';
            code[i + 1] = code[i + 2] = code[i + 3] = code[i + 4] = code[i + 5] = ' ';
            coeff[i] = coeff[i + 2];
        }

        /*
        else if (match_pattern(code + i, "[pcpc]") && coeff[i + 4] == -1 && coeff[i + 2] == 1 && coeff[i + 1] == -coeff[i + 3])
        {
            code[i] = 'm';
            code[i + 1] = code[i + 2] = code[i + 3] = code[i + 4] = code[i + 5] = ' ';
            coeff[i] = coeff[i + 1];
        }
        */

       /*
        * Moving the pointer is almost always needed to do anything
        * in a Brainfuck program, so we can couple pointer movements
        * with the command next to it to treat the latter as one
        * "movement + command" instruction. The movement is stored
        * in the "mov" array.
        *
        * TODO: For now, mov is a global variable, I should try to
        * change that
        */

        else if (match_pattern(code + i, "p"))
        {
            code[i] = ' ';
            mov[i + 1] += coeff[i];
        }
   }

    // Removing all the spaces

    for (i = 0, j = 0; code[i]; ++i)
    {
        if (code[i] != ' ')
        {
            code[j] = code[i];
            coeff[j] = coeff[i];
            mov[j++] = mov[i];
        }
    }
    code[j] = '\0';
    coeff[j] = 0;

   /*
    * Transform commands into bytecode
    *  \0  c  p  [  ]  0  s  m  .  ,
    *   0  1  2  3  4  5  6  7  8  9
    */

    for (i = 0; code[i]; ++i)
        code[i] = strchr("cp[]0sm.,", code[i]) - "cp[]0sm.," + 1;

    return (coeff);
}

int match_brackets(const char *code, int *coeff, const int left)
{
   /*
    * Recursive function to match the brackets. Each time we
    * encounter a left bracket, we call the function with the
    * substring of the code starting from the bracket. Thus,
    * the first right bracket we encounter has to be the
    * matching bracket.
    *
    * As coeff, we store with each bracket the offset needed to
    * reach its counterpart.
    */

    int i;

    for (i = left + 1; code[i]; ++i)
    {
        if (code[i] == 3)       // left bracket
            i = match_brackets(code, coeff, i);
        else if (code[i] == 4)  // right bracket
        {
            coeff[left] = i - left;
            coeff[i] = left - i;
            return (i);
        }
    }
    return (0);
}

/*
 * The following functions implement different behaviors
 * regarding the checking of the cell array bounds. They
 * are conditionally compiled if the corresponding macro
 * is defined, in order to not burden the program.
 *
 * If the Brainfuck program tries to access cells outside
 * the cell array, the interpreter can EXTEND the array,
 * WRAP to the other end, BLOCK the pointer movement at
 * the bound, or ABORT the program. Define the
 * MEMORY_BEHAVIOR macro with one of these words to enable
 * the corresponding setting.
 *
 * If NONE of these settings is enabled, the interpreter
 * will be faster since it won't do any checking. If the
 * program tries to access a cell outside the array, the
 * behavior is undefined (the program may or may not crash
 * or behave unexpectedly).
 *
 * As for the details of the memory behavior functions,
 * it's entirely pointer arithmetic, AKA magic.
 */

#if (MEMORY_BEHAVIOR == EXTEND)
static void extend_memory(CELL **ptr0, CELL **ptr, size_t *size, int shift)
{
    if (*ptr + shift >= *ptr0 + *size)
    {
        int old_size;

        old_size = *size;
        *size = *ptr + shift - *ptr0 + 1;
        *ptr0 = xrealloc(*ptr0, (*size + 1) * sizeof(CELL));
        *ptr = *ptr0 + *size - shift - 1;
        memset(*ptr0 + old_size, 0, *size - old_size);
    }
    else if (*ptr + shift < *ptr0)
    {
        int old_size;
        int i;

        old_size = *size;
        *size += *ptr0 - (*ptr + shift);
        *ptr0 = xrealloc(*ptr0, (*size + 1) * sizeof(CELL));
        *ptr = *ptr0 - shift;
        for (i = old_size; i >= 0; --i)
            (*ptr0)[i + *size - old_size] = (*ptr0)[i];
        memset(*ptr0, 0, *size - old_size);
    }
    *ptr += shift;
}
#elif (MEMORY_BEHAVIOR == ABORT)
static void abort_memory(CELL *ptr0, CELL **ptr, size_t size, int shift)
{
    if (*ptr + shift >= ptr0 + size || *ptr + shift < ptr0)
        error(ERROR_MEMORY, *ptr - ptr0 + shift, size - 1);
    *ptr += shift;
}
#elif (MEMORY_BEHAVIOR == WRAP)
static void wrap_memory(CELL *ptr0, CELL **ptr, size_t size, int shift)
{
    *ptr += shift + ((*ptr + shift >= ptr0 + size) ? -size : (*ptr + shift < ptr0) ? size : 0);
}
#elif (MEMORY_BEHAVIOR == BLOCK)
static void block_memory(CELL *ptr0, CELL **ptr, size_t size, int shift)
{
    *ptr = (*ptr + shift >= ptr0 + size) ? ptr0 + size - 1 : (*ptr + shift < ptr0) ? ptr0 : *ptr + shift;
}
#endif

void exec_prog(const char *code, const int *coeff)
{
    size_t array_size = INITIAL_ARRAY_SIZE;

    // ptr0 is where the cell array begins, ptr is the current pointer

    CELL *ptr0 = xcalloc(array_size, sizeof(CELL));
    CELL *ptr = ptr0;

    /*
     * Thanks to a GCC extension, we can use a special operator "&&" to get
     * label addresses and store them in an array of pointers. Thus, we will
     * only need to lookup the next bytecode instruction in this array to
     * go to the corresponding label without branching or looping at all,
     * which makes this approach very efficient. You can think of it a bit
     * like an inline function pointer array.
     */

    static const void *instr[10] =
    {
        &&end,
        &&changevalue,
        &&movepointer,
        &&leftbracket,
        &&rightbracket,
        &&zerocell,
        &&seekzerocell,
        &&movecell,
        &&output,
        &&input
    };

    // The buffer used for outputs

    char buffer[CHUNK_SIZE];
    int buffer_index = 0;

    /*
     * i is the index we use to read the Brainfuck program, now converted
     * into our bytecode. The NEXT_INSTRUCTION macro increments i by one, so
     * we initialize it with -1 to execute the instruction at position 0.
     * NEXT_INSTRUCTION will move directly to the next instruction without
     * branching, thanks to the "computed gotos" made available by GCC.
     */

    int i = -1;
    MOVE_POINTER
    NEXT_INSTRUCTION

    changevalue:
        *ptr += coeff[i];
        MOVE_POINTER
        NEXT_INSTRUCTION

/*
 * WTF :c weird bug tests :
 *
 *                    | long.b | counter.b | prime.b | mandelbrot.b
 * ----------------------------------------------------------------
 *   ptr += coeff[i]  |  3.7s  |    6.5s   |   5.7s  |     3.4s
 *   NEXT_INSTRUCTION |        |           |         |
 * ----------------------------------------------------------------
 *   ptr += coeff[i]  |  2.5s  |    7.7s   |   5.7s  |     3.1s
 * ----------------------------------------------------------------
 *  NEXT_INSTRUCTION  |  5.2s  |    7.5s   |   6.4s  |     3.7s
 * ----------------------------------------------------------------
 *   only the label   |  3.7s  |    6.5s   |   5.7s  |     3.1s
 * ----------------------------------------------------------------
 *  without the label |  2.9s  |    7.1s   |   6.2s  |     3.7s
 *
 * So I have to choose between the second and the fourth solution to
 * optimize either long.b or counter.b.
 *
 * UPDATE A FEW HOURS LATER I still don't understand but I managed to
 * change a bit the bug so that the second solution is the faster for
 * both long.b and counter.b (--> 2.5s 6.5s 5.7s 3.1s). Enjoy c:
 */

// In short, if this (now useless) label is removed, the performance drops.
// Plz help :c

    movepointer:
        ptr += coeff[i];
        //NEXT_INSTRUCTION

    /*
     * The tests with coeff[i] seem redundant (they're here to determine if
     * it's a left or right bracket) but grouping the bracket instructions
     * like this generates fewer asm instructions.
     */

    leftbracket:
    rightbracket:
        i += ((coeff[i] > 0) && !(*ptr)) || ((coeff[i] < 0) && *ptr) ? coeff[i] : 0;
        MOVE_POINTER
        NEXT_INSTRUCTION

    zerocell:
        *ptr = 0;
        MOVE_POINTER
        NEXT_INSTRUCTION

    seekzerocell:
        for (; *ptr; ptr += coeff[i]);
        MOVE_POINTER
        NEXT_INSTRUCTION

    movecell:
        *(ptr + coeff[i]) += *ptr;
        *ptr = 0;
        MOVE_POINTER
        NEXT_INSTRUCTION

    // If we encounter an output instruction, we put it in our buffer.
    // When its size reaches CHUNK_SIZE, we print it, then reset it.

    output:
        buffer[buffer_index++] = *ptr;
        if (buffer_index == CHUNK_SIZE)
            PRINT_BUFFER(CHUNK_SIZE)
        MOVE_POINTER
        NEXT_INSTRUCTION

    // In case of input, we first print and reset the output buffer.

    input:
        PRINT_BUFFER(buffer_index)
        int tmp;
        if ((tmp = getchar()) != EOF)
            *ptr = tmp;
#if (EOF_INPUT_BEHAVIOR != NO_CHANGE)
        else
            *ptr = EOF_INPUT_BEHAVIOR;
#endif
        MOVE_POINTER
        NEXT_INSTRUCTION

    // When the program ends, we print the output buffer and free the cell array.

    end:
        PRINT_BUFFER(buffer_index)
        free(ptr0);
}

int main(int ac, char **av)
{
    char *code;
    int *coeff;

    // Usage: ./sbfi filename

    if (ac < 2)
        error(ERROR_NO_ARGS);
    else if (ac > 2)
        error(ERROR_TOO_MANY_ARGS);

    code = get_src(av[1]);
    check_src(code);
    strip_comments(code);
    coeff = optim_code(code);
    match_brackets(code, coeff, -1);
    exec_prog(code, coeff);
    free(code);
    free(coeff);
    free(mov);
    return (EXIT_SUCCESS);
}

/** Simple BrainFuck Interpreter V3.0 -- Written by Maxime Rinoldo **/

#include "sbfi.h"

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
 * EOF_INPUT_BEHAVIOR can be either LEAVE_UNCHANGED, or the
 * integer that will be written to the cell if EOF is
 * encountered on input (default : LEAVE_UNCHANGED).
 */

#define CELL                unsigned char
#define INITIAL_ARRAY_SIZE  30000
#define MEMORY_BEHAVIOR     NONE
#define EOF_INPUT_BEHAVIOR  LEAVE_UNCHANGED

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

char *get_src(char *filename)
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

void check_src(char *code)
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

int *optim_code(char *code)
{
    // This int array will allow us to compress commands

    int *coeff = xcalloc(strlen(code), sizeof(int));

   /*
    * Compress consecutive +/-/</> into single commands
    * associated with the real count in the coeff array
    *
    * For example, +++++++ becomes + with 7
    */

    size_t i;
    size_t j;
    int count;

    for (i = 0, j = 0; code[i]; ++j)
    {
        count = 1;
        code[j] = code[i++];
        if (strchr("+-<>", code[i]))
            for (; code[j] == code[i]; ++i, ++count);
        coeff[j] = count;
    }
    code[j] = '\0';

   /*
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

    for (i = 0; code[i]; ++i)
    {
        if (code[i] == '-' || code[i] == '<')
            coeff[i] *= -1;
        if (code[i] == '+' || code[i] == '-')
            code[i] = 'c';
        else if (code[i] == '<' || code[i] == '>')
            code[i] = 'p';
    }

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
    */

    for (i = 0; code[i + 2]; ++i)
    {
        // [-] sets a cell to zero

        if (!strncmp(code + i, "[c]", 3) && coeff[i + 1] == -1)
        {
            code[i] = '0';
            code[i + 1] = code[i + 2] = ' ';
        }

        // [>] / [<] stop at the first zero cell they encounter

        else if (!strncmp(code + i, "[p]", 3))
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
        */

        else if (!strncmp(code + i, "[cpcp]", 6) && coeff[i + 1] == -1 && coeff[i + 3] == 1 && coeff[i + 2] == -coeff[i + 4])
        {
            code[i] = 'm';
            code[i + 1] = code[i + 2] = code[i + 3] = code[i + 4] = code[i + 5] = ' ';
            coeff[i] = coeff[i + 2];
        }
   }

    // Removing all the spaces

    for (i = 0, j = 0; code[i]; ++i)
    {
        if (code[i] != ' ')
        {
            code[j] = code[i];
            coeff[j++] = coeff[i];
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

int match_brackets(char *code, int *coeff, int left)
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

void exec_prog(char *code, int *coeff)
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
     * into our bytecode. The MOVE_TO_NEXT macro increments i by one, so
     * we initialize it with -1 to execute the instruction at position 0.
     * MOVE_TO_NEXT will move directly to the next instruction without
     * branching, thanks to the "computed gotos" made available by GCC.
     */

    int i = -1;
    MOVE_TO_NEXT

    changevalue:
        *ptr += coeff[i];
        MOVE_TO_NEXT

    movepointer:
#if (MEMORY_BEHAVIOR == EXTEND)
        extend_memory(&ptr0, &ptr, &array_size, coeff[i]);
#elif (MEMORY_BEHAVIOR == ABORT)
        abort_memory(ptr0, &ptr, array_size, coeff[i]);
#elif (MEMORY_BEHAVIOR == WRAP)
        wrap_memory(ptr0, &ptr, array_size, coeff[i]);
#elif (MEMORY_BEHAVIOR == BLOCK)
        block_memory(ptr0, &ptr, array_size, coeff[i]);
#else
        ptr += coeff[i];
#endif
        MOVE_TO_NEXT

    /*
     * The tests with coeff[i] seem redundant (they're here to determine if
     * it's a left or right bracket) but grouping the bracket instructions
     * like this generates fewer asm instructions.
     */

    leftbracket:
    rightbracket:
        i += ((coeff[i] > 0) && !(*ptr)) || ((coeff[i] < 0) && *ptr) ? coeff[i] : 0;
        MOVE_TO_NEXT

    zerocell:
        *ptr = 0;
        MOVE_TO_NEXT

    seekzerocell:
        for (; *ptr; ptr += coeff[i]);
        MOVE_TO_NEXT

    movecell:
        *(ptr + coeff[i]) += *ptr;
        *ptr = 0;
        MOVE_TO_NEXT

    // If we encounter an output instruction, we put it in our buffer.
    // When its size reaches CHUNK_SIZE, we print it, then reset it.

    output:
        buffer[buffer_index++] = *ptr;
        if (buffer_index == CHUNK_SIZE)
            PRINT_BUFFER(CHUNK_SIZE)
        MOVE_TO_NEXT

    // In case of input, we first print and reset the output buffer.

    input:
        PRINT_BUFFER(buffer_index)
        int tmp;
        if ((tmp = getchar()) != EOF)
            *ptr = tmp;
#if (EOF_INPUT_BEHAVIOR != LEAVE_UNCHANGED)
        else
            *ptr = EOF_INPUT_BEHAVIOR;
#endif
        MOVE_TO_NEXT

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
    return (EXIT_SUCCESS);
}

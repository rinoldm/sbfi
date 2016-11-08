# sbfi

Simple BrainFuck Interpreter written in C by [Maxime Rinoldo](mailto:maxime.rinoldo@epitech.eu). The aim of this interpreter is to be as fast as possible while remaining easy to understand.

Explanations about how my program works can be found in the comments !

## Compilation

I compile with `gcc -Wall -Wextra -Ofast -s`, which doesn't output any warning.

`-pedantic` complains about label addresses / computed gotos, C++ comments, and declarations mixed with code.

Note that Clang supports the *computed gotos* extension as well and compiles fine.

## Implementation details

In the original Brainfuck specification by Urban Müller in 1993, a lot of details were left unspecified or unclear, which means they are **implementation-defined**. As such, any Brainfuck interpreter or compiler is free to do whatever it wants with them, as long as the choices are documented.

The following is a list of implementation-defined behaviors in sbfi.

### Cell size

In the reference implementation, cells can hold an integer **between 0 and 255**. **In sbfi, the default setting is that each cell is of the type `unsigned char`**, which has almost always the same range. You can change this setting by modifying the **`CELL`** macro and defining with any other integer type.

### Cell bounds

The reference implementation has **wrapping cells** : if a cell that contains the maximal value for its type is incremented, the value is set to the minimal value for its type, and vice versa.

With a C example, it means :
```
unsigned char c = 255;

++c; // c = 0
--c; // c = 255
```
**Cells in sbfi have this behavior too**, since it's a natural way of dealing with overflows in many languages.

### Array size

The cell array contains **30000 cells** in the reference implementation, and **so is the default in sbfi**. However, you can change this setting by modifying the **`INITIAL_ARRAY_SIZE`** macro, as long as you define it with at least 1 cell.

### Array bounds

The reference implementation **raises an error and stops the execution** if a Brainfuck program tries to access a cell outside of the array. **sbfi implements five different behaviors regarding the bound checking** ; you can enable them by setting the **`MEMORY_BEHAVIOR`** macro to one of the following macros :

- **`NONE`**   : the default setting. No bound checking is performed at all, to improve performance. It is assumed that the program will not try to access a cell outside of the array. It it does, the behavior is **undefined**.
- **`EXTEND`** : if needed, the array is extended during runtime to be able to run the program correctly. 
- **`ABORT`**  : raises an error and stops the execution.
- **`WRAP`**   : the array wraps around, like the cells. If a bound is reached, the array pointer goes to the other bound.
- **`BLOCK`**  : if the pointer is on a bound and tries to go further, it is blocked and stays here. The program execution doesn't stop and the pointer can still move in the other direction. 

### End-Of-Line in input / output

ASCII defines `10` as a *line-feed* (`LF`), and `13` as a *carriage-return* (`CR`). Different conventions for representing the line break are in use today, among them `LF` (Unix systems), `CR` (Mac OS), and even `CR LF` (Windows), which hinders portability. However, **the general consensus (and the reference implementation) favors `LF`, and that's what sbfi implements too.** In other words, inputting a line break will set the current cell to `10`, and outputting a `10` will display a line break.

### End-Of-File in input

When `EOF` is encountered in the input, there is no definite decision about what to do. Three behaviors are generally considered :

- Set the current cell to `-1`
- Set the current cell to `0`
- Leave the current cell unchanged

**The last one is often considered more portable, that's why it's the default setting in sbfi.** However, you can chose the behavior by changing the **`EOF_INPUT_BEHAVIOR`** macro to either **`NO_CHANGE`** or the value you want to set the current cell to (**`0`**, **`-1`** or any other integer value).

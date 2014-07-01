sbfi
====

Simple BrainFuck Interpreter written in C by Maxime Rinoldo.

For the moment, explanations about how my program works can be found in the comments !



I compile with gcc -Wall -Wextra -O3 -s, which doesn't output any warning. -pedantic complains about C++ comments, label addresses / computed gotos, and a declaration mixed with code.

Clang should be able to compile this program too, I don't know about other compilers.

There are a lot of tests available, but Mandelbrot and Hanoi are usually good to estimate the speed of an interpreter. I'll add test files soon.

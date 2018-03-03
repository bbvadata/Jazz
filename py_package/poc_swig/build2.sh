#!/bin/bash

export LD_LIBRARY_PATH=~/anaconda3/lib:$LD_LIBRARY_PATH

swig -python example.i
gcc -c -fpic example.c example_wrap.c -I/usr/include/python2.7
gcc -shared example.o example_wrap.o -o _example.so

python2 -c "import example
print example.fact(6)
print example.my_mod(23, 7)
print example.cvar.My_variable"

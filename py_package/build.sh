#!/bin/bash

py_package=$(pwd)

cd pyjazz

rm -rf dist/
rm -rf pyjazz.egg-info/
rm -rf dist/

cd pyjazz

export LD_LIBRARY_PATH=~/anaconda3/lib:$LD_LIBRARY_PATH

swig -python jazz_blocks.i

g++ -I../../../server -I/usr/include -DNDEBUG -c -o jazz_miscutils.o ../../../server/src/jazz_utils/jazz_miscutils.cpp
g++ -c -fpic jazz_blocks_wrap.c -I/usr/include/python2.7
g++ -shared jazz_miscutils.o jazz_blocks_wrap.o -o _jazz_blocks2.so

rm *.o
rm *.so

g++ -I../../../server -I/usr/include -DNDEBUG -c -o jazz_miscutils.o ../../../server/src/jazz_utils/jazz_miscutils.cpp
g++ -c -fpic jazz_blocks_wrap.c -I/usr/include/python3.6
g++ -shared jazz_miscutils.o jazz_blocks_wrap.o -o _jazz_blocks3.so

rm *.o
rm *.so

rm jazz_blocks_wrap.c

cd $py_package

python setup.py sdist
twine upload dist/*

rm -rf dist/
rm -rf pyjazz.egg-info/

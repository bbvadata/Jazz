#!/bin/bash

set -e

cd server || return 1

make clean

cd src/lmdb || return 1

make

mkdir testdb

./test_1
./test_2
./test_3
./test_4
./test_5
./test_6

./mdb_stat -a testdb

printf "\n\nAll tests passed.\n"

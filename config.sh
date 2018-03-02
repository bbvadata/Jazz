#!/bin/bash

# This script creates:

    server/makefile                 // To compile the server with make
    server/main.dox                 // The landing page of the doxygen site of the server
    server/src/include/platform.h   // The name of the Linux distro with which it was built as constant LINUX_PLATFORM
    server/src/include/version.h    // The version string as constant JAZZ_VERSION
    pyjazz/build.sh                 // To build the Python 2.7 3.x package
    rjazz/build.sh                  // To build the R package


cwd=$(pwd)

cd server

vpath=`echo src/*`
jzpat=`echo $vpath | sed 's/\ /\n/g' | grep jazz`

cpps=`find src/ | grep '.*jazz_.*cpp$'`
objs=`echo $cpps | sed 's/\ /\n/g' | sed 's/.*\(jazz_.*cpp\)$/\1/' | sed 's/cpp/o/'`


echo $vpath
echo $jzpat
echo $cpps
echo $objs


for cpp in $cpps; do
  obj=`echo $cpp | sed 's/.*\(jazz_.*cpp\)$/\1/' | sed 's/cpp/o/'`
  hea=`echo $cpp | sed 's/cpp$/h/'`

  if [ -e $hea ]; then
    dep=`grep -rnw $cpp $hea -e '^#include.*\(jazz.*h\)' | sed 's/.*\(jazz.*h\).*/\1/'`
  else
    dep=`grep -rnw $cpp -e '^#include.*\(jazz.*h\)' | sed 's/.*\(jazz.*h\).*/\1/'`
  fi

  echo $obj: $dep
done


cd $cwd

if [ ! -e "./curl-7.54.0/lib/.libs/libcurl.so.4.4.0" ]; then
  echo "File ./curl-7.54.0/lib/.libs/libcurl.so.4.4.0 not found."
  exit 1
fi

echo "Finished build.sh"
exit 0

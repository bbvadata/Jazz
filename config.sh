#!/bin/bash

if [[ $1 = '--help' ]]; then
  cat _config_/help_on_config.txt
  exit 0
fi

printf "(usage: ./config.sh --help)\n\nGathering information ...\n\n"

jazz_pwd=$(pwd)

jazz_version=`cat _config_/version`
jz_processor=`uname -p`
jazz_distro1=`cat /etc/*-release | grep '^DISTRIB_ID=' | sed 's/^DISTRIB_ID=//'`
jazz_distro2=`cat /etc/*-release | grep '^DISTRIB_RELEASE=' | sed 's/^DISTRIB_RELEASE=//'`

if [ -e '_config_/mhd_include_path' ]; then mhd_inclpath=`cat _config_/mhd_include_path`; else mhd_inclpath='/usr/include'; fi

if [ ! -e "$mhd_inclpath/microhttpd.h" ]; then
  echo "** File $mhd_inclpath/microhttpd.h was not found. **"
  cat _config_/help_mhd_not_found.txt
  exit
fi

if [ -e '_config_/mhd_library_path' ]; then mhd_libpath=`cat _config_/mhd_library_path`
else
  if [ -e '/usr/lib/x86_64-linux-gnu/libmicrohttpd.so' ]; then mhd_libpath='/usr/lib/x86_64-linux-gnu'; else mhd_libpath='/usr/lib'; fi
fi

if [ ! -e "$mhd_libpath/libmicrohttpd.so" ]; then
  echo "** File $mhd_libpath/libmicrohttpd.so was not found. **"
  cat _config_/help_mhd_not_found.txt
  exit 1
fi

cd server

vpath=`echo src/*`
jzpat=`echo $vpath | sed 's/\ /\n/g' | grep jazz | tr '\n' ' '`

cpps=`find src/ | grep '.*jazz_.*cpp$' | tr '\n' ' '`
objs=`echo $cpps | sed 's/\ /\n/g' | sed 's/.*\(jazz_.*cpp\)$/\1/' | sed 's/cpp/o/' | tr '\n' ' '`

depends ( )
{
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
}

jazz_depends=`depends`

cd $jazz_pwd

printf "You will now override:\n\n"
cat _config_/help_on_config.txt | grep '  - '
printf "\n"

read -p "Do you want to continue? [y/N] " -n 1 -r
echo    # (optional) move to a new line

if [[ $REPLY =~ ^[Yy]$ ]]; then printf "\n"; else exit 1; fi

echo "jazz_pwd     = $jazz_pwd"
echo "jazz_version = $jazz_version"
echo "jz_processor = $jz_processor"
echo "jazz_distro1 = $jazz_distro1"
echo "jazz_distro2 = $jazz_distro2"
echo "mhd_inclpath = $mhd_inclpath"
echo "mhd_libpath  = $mhd_libpath"
echo "vpath        = $vpath"
echo "jzpat        = $jzpat"
echo "cpps         = $cpps"
echo "objs         = $objs"
echo "jazz_depends = $jazz_depends"

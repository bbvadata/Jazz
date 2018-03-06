#!/bin/bash

#    (c) 2018 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


mode='RUN'	# Set mode='DEBUG' for echoing all variables before confirmation.


# Section 1. Gathering all necessary information. 1. Fail if necessary. 2. Do nothing without confirmation
# ----------

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

# End of section 1: Dump all variables if debugging
if [[ $mode =~ 'DEBUG' ]]; then
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

  printf "\n"
fi


# Section 2. Ask for confirmation. Do nothing if not.
# ----------

printf "You will now override:\n\n"
cat _config_/help_on_config.txt | grep '  - '
printf "\n"

read -p "Do you want to continue? [y/N] " -n 1 -r
echo    # (optional) move to a new line

if [[ $REPLY =~ ^[Yy]$ ]]; then printf "\n"; else exit 1; fi


# Section 3. Start the Kung-fu
# ----------

printf "Writing: server/src/include/jazz_platform.h ... "

echo "`cat _config_/copyright_notice`

#define JAZZ_VERSION \"$jazz_version\"
#define JAZZ_DEBUG_HOME \"$jazz_pwd\"

#define LINUX_DISTRO \"$jazz_distro1\"
#define LINUX_VERSION \"$jazz_distro2\"
#define LINUX_PROCESSOR \"$jz_processor\"

#define LINUX_PLATFORM LINUX_DISTRO \"_\" LINUX_VERSION \"_\" LINUX_PROCESSOR" > server/src/include/jazz_platform.h

printf "Ok.\n"


printf "Writing: server/Makefile ... "

echo "`cat _config_/makefile_head`

CXXFLAGS    := -std=c++11 -I. -I$mhd_inclpath
LINUX       := ${jazz_distro1}_${jazz_distro2}
HOME        := $jazz_pwd
VERSION     := $jazz_version
mhd_libpath := $mhd_libpath

VPATH = $vpath

objects = $objs

# Header file dependencies:

$jazz_depends

`cat _config_/makefile_tail`" > server/Makefile

printf "Ok.\n"


printf "Writing: server/src/main.dox ... "

echo "/**
\mainpage [Programming documentation for the Jazz Server version \"$jazz_version\"]

`cat _config_/main_dox_tail`" > server/src/main.dox

printf "Ok.\n"


printf "Writing: r_package/rjazz/DESCRIPTION ... "

echo "`cat _config_/description_head`
Version: $jazz_version
Date: $(date +%F)
`cat _config_/description_tail`" > r_package/rjazz/DESCRIPTION

printf "Ok.\n"


printf "Writing: r_package/build.sh ... "

echo "`cat _config_/build_r_head`
R CMD check rjazz_$jazz_version.tar.gz
R CMD INSTALL rjazz_$jazz_version.tar.gz
rm -rf rjazz.Rcheck" > r_package/build.sh

chmod 777 r_package/build.sh

printf "Ok.\n"


printf "Writing: py_package/pyjazz/jazz_version.py ... "

echo "JAZZ_VERSION = \"$jazz_version\"" > py_package/pyjazz/jazz_version.py

printf "Ok.\n"


printf "Writing: py_package/build.sh ... "

echo "#!/bin/bash

export LD_LIBRARY_PATH=~/anaconda3/lib:$LD_LIBRARY_PATH

swig -python example.i" > py_package/build.sh

chmod 777 py_package/build.sh

printf "Ok.\n"


printf "Writing: docker/upload_docker.sh ... "

echo "`cat _config_/upload_docker_head`

sudo docker tag jazz_ref_stable kaalam/jazz_neat:$jazz_version
sudo docker push kaalam/jazz_neat:$jazz_version

# docker run -ti kaalam/jazz_neat:0.2.1 /bin/bash
# docker run -b kaalam/jazz_neat:0.2.1
# docker run kaalam/jazz_neat:0.2.1" > docker/upload_docker.sh

chmod 777 docker/upload_docker.sh

printf "Ok.\n"


cat _config_/help_on_done.txt
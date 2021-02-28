#!/bin/bash

#    (c) 2018-2021 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#

pushd server || exit 1

make clean

if ! scan-build -o ./static_analysis_reports/ make jazz
then
  echo "make failed."
  exit 1
fi

if ! scan-build -o ./static_analysis_reports/ make tjazz
then
  echo "make failed."
  exit 1
fi

cppcheck src/ -i src/catch2/ -i src/curl/ --force --inline-suppr --xml 2>report.xml
cppcheck-htmlreport --file=report.xml --title=Jazz --report-dir=static_analysis_reports --source-dir=.

rm -f report.xml

popd || exit 1

reports=$(find server/static_analysis_reports/ | grep "index.html")

printf "\nDone.\n"
printf "\n** See the reports in: **"
printf "\n---===================---\n\n"
echo "$reports"
printf "\n"

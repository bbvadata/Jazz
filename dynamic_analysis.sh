#!/bin/bash

#    (c) 2018 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#

rm -rf server/dynamic_analysis_reports/

pushd server

make tjazz

mkdir dynamic_analysis_reports

valgrind --leak-check=yes --log-file=dynamic_analysis_reports/memcheck.txt ./tjazz
#valgrind --tool=callgrind --log-file=dynamic_analysis_reports/callgrind.txt ./tjazz

make clean

popd

reports=$(find server/dynamic_analysis_reports/ | grep ".txt")

printf "\nDone.\n"
printf "\n** See the reports in: **"
printf "\n---===================---\n\n"
echo "$reports"
printf "\n"

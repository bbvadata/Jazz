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

cd server

make tjazz

valgrind --leak-check=yes ./tjazz

make clean

cd ..

reports=`find server/dynamic_analysis_reports/ | grep "index.html"`

printf "\nDone.\n"
printf "\n** See the reports in: **"
printf "\n---===================---\n\n"
echo "$reports"
printf "\n"

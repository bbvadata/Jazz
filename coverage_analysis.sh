#!/bin/bash

#    (c) 2018-2026 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#

root_dir=$(pwd)

pushd server || exit 1

make cjazz

./cjazz

lcov --capture --directory . --output-file coverage.info --rc geninfo_unexecuted_blocks=1
lcov --remove coverage.info '/usr/include/*' '*catch2*' --output-file coverage.info

genhtml coverage.info --output-directory coverage_html

popd || exit 1

reports=$(find "$root_dir/server/coverage_html" -name "index.html")

printf "\nDone.\n"
printf "\n** See the reports in: **"
printf "\n---===================---\n\n"
while IFS= read -r report; do
  echo "file://$report"
done <<< "$reports"
printf "\n"

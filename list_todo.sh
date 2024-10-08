#!/bin/bash

#    (c) 2018-2024 kaalam.ai (The Authors of Jazz)
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

grep -rnw server/src/ -e "^//TODO:\(.*\)$" > neat_todos.txt
grep -rnw server/src/ -e "TODO:" > sloppy_todos.txt

printf "List of sloppy TODOs (in server/src/):\n"
diff neat_todos.txt sloppy_todos.txt

grep -rnw uplifts/ -e "^//TODO:\(.*\)$" > neat_todos.txt
grep -rnw uplifts/ -e "TODO:" > sloppy_todos.txt

printf "List of sloppy TODOs (in uplifts/):\n"
diff neat_todos.txt sloppy_todos.txt

rm neat_todos.txt sloppy_todos.txt

printf "\nList of neat TODOs (in server/src/):\n\n"
grep -rnw server/src/ -e "^//TODO:\(.*\)$"

printf "\nNumber of neat TODOs (in server/src/):\n\n"
grep -rnw server/src/ -e "^//TODO:\(.*\)$" | wc -l

printf "\nList of neat TODOs (in uplifts/):\n\n"
grep -rnw uplifts/ -e "^//TODO:\(.*\)$"

printf "\nNumber of neat TODOs (in uplifts/):\n\n"
grep -rnw uplifts/ -e "^//TODO:\(.*\)$" | wc -l

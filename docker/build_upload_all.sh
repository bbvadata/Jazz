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

set -e

docker_pwd=$(pwd)

cd base || return 1

./build_upload.sh

cd "$docker_pwd" || return 1

cd lss || return 1

./build_upload.sh

cd "$docker_pwd" || return 1

cd tng || return 1

./build_upload.sh

cd "$docker_pwd" || return 1

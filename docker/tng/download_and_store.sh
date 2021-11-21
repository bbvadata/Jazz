#!/bin/bash

cd /home/jazz/jazz_dbg_mdb || return 1

mkdir -p github_repos

cd github_repos || return 1

git clone https://github.com/kaalam/tng-data-facts.git
git clone https://github.com/kaalam/tng-data-grammar.git
git clone https://github.com/kaalam/tng-data-qa.git
git clone https://github.com/kaalam/tng-data-readings.git
git clone https://github.com/kaalam/tng-data-wikipedia.git

cd tng-data-facts || return 1

./expand.sh

cd ../tng-data-grammar || return 1

./expand.sh

cd ../tng-data-qa || return 1

./expand.sh

cd ../tng-data-readings || return 1

./expand.sh

cd ../tng-data-wikipedia || return 1

./expand.sh

cd /home/jazz || return 1


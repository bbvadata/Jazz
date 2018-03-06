# BBVA - Jazz: A lightweight analytical web server for data-driven applications. (R client)
# ------------
#
#	Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.
#
# This product includes software developed at
#
# BBVA (https://www.bbva.com/)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
options(warn=2)


run	  <- c('library(rjazz)', '')
todos <- FALSE


check_todo <- function(doc)
{
	cat(doc, '\n')

	txt <- readLines(doc)

	ti <- grepl('^//TODO:', txt)
	if (any(ti))
	{
		todos <<- TRUE
		cat(txt[ti], '\n')
	}

	ix <- which(grepl('^\\\\examples\\{.*', txt))

	if (length(ix) == 0) return(TRUE)
	if (length(ix) != 1) stop('Many examples')

	if (which(grepl('^\\\\dontrun\\{.*', txt)) != ix + 1) stop('\\dontrun expected')

	ex <- which(grepl('^\\}$', txt))
	ex <- ex[ex > ix + 2]

	if (length(ex) < 2) stop('unexpected closing')

	run <<- c(run, '', 'rm(list = ls())', '', txt[(ix + 2):(ex[1] - 1)])
}

docs <- sort(list.files('./rjazz/man', full.names = T))

for (doc in docs) check_todo(doc)

writeLines(run, './all_examples.R')

if (todos) stop('TODOs found.')

source('./all_examples.R')

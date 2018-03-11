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
get_server_version <- function(host = .host., full = FALSE)
{
	downlo <- basicTextGatherer()
	if (curlPerform(url = paste0(host, '//sys//server_vers'), writefunction = downlo[[1]]) != 0) stop ('Http error status.')

	vers <- downlo$value()
	rex	 <- '^([0-9]+)\\.([0-9]+)\\.([0-9]+)$'

	if (!grepl(rex, vers)) stop ('Unexpected format in server response.')

	if (gsub(rex, '\\1', vers) != gsub(rex, '\\1', .version.) || gsub(rex, '\\2', vers) != gsub(rex, '\\2', .version.))
		warning(paste('Jazz server version', vers, 'is incompatible with client version', .version., 'update both to latest.'))

	if (!full) return(vers)

	downlo$reset()

	if (curlPerform(url = paste0(host, '//sys//server_vers/full'), writefunction = downlo[[1]]) != 0) stop ('Http error status.')

	rex	 <- '^[[:blank:]]*([[:alnum:]]+)[[:blank:]]*:[[:blank:]]*([^[:blank:]].*[^[:blank:]])[[:blank:]]*$'
	ss <- strsplit(downlo$value(), '\n')[[1]]
	ss <- ss[grepl(rex, ss)]
	vers <- gsub(rex, '\\2', ss)
	names(vers) <-	gsub(rex, '\\1', ss)

	as.list(vers)
}

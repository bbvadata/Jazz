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
get_block_attributes <- function(source, block_key, host = .host.)
{
	downlo <- basicTextGatherer()
	if (curlPerform(url			  = paste0(host, '//', source, '.', block_key, '.header'),
					writefunction = downlo[[1]],
					customrequest = 'GET') != 0) stop ('Http error status.')

	rexi <- '^([[:alnum:]]+):([0-9]+)*$'
	rexh <- '^([[:alnum:]]+):([0-9a-f]+)*$'

	ss	<- strsplit(downlo$value(), '\n')[[1]]

	if (any(!grepl(rexh, ss))) stop ('Unexpected format.')

	ix <- which(grepl(rexi, ss))

	vv <- as.numeric(gsub(rexi, '\\2', ss[ix]))
	names(vv) <- gsub(rexi, '\\1', ss[ix])

	ll <- as.list(vv)
	ss <- ss[-ix]
	vv <- gsub(rexh, '\\2', ss)
	names(vv) <- gsub(rexh, '\\1', ss)

	c(ll, as.list(vv))
}

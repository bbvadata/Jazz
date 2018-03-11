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
create_block_rep <- function(source, block_key, val, times, host = .host.)
{
	fun <- .rep_fun.[class(val)]

	if (is.na(fun)) stop ('Wrong class in val.')

	downlo <- basicTextGatherer()

	if (fun == 'C_chars_rep')
	{
		qq <- paste0(host, '//', source, '.', block_key, '.', fun, '/', .niceFmt(times))
		upload <- charToRaw(val)

		if (curlPerform(url			  = qq,
						infilesize	  = length(upload),
						readfunction  = upload,
						writefunction = downlo[[1]],
						upload		  = TRUE,
						customrequest = 'PUT') != 0) stop ('Http error status.')

	} else {
		if (fun == 'C_bool_rep')
		{
			qq <- paste0(host, '//', source, '.', block_key, '.', fun, '/', 1*val[1], ',', .niceFmt(times))
		} else {
			qq <- paste0(host, '//', source, '.', block_key, '.', fun, '/', .niceFmt(val), ',', .niceFmt(times))
		}

		if (curlPerform(url			  = qq,
						infilesize	  = 0,
						writefunction = downlo[[1]],
						customrequest = 'PUT') != 0) stop ('Http error status.')
	}

	if (downlo$value() != '0') stop('Http PUT Not Acceptable.')

	TRUE
}

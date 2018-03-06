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
put_strings_as_block <- function(source, block_key, txt, type, fmt = NA, host = .host.)
{
	auxkey <- new_key()

	put_raw_block(source, auxkey, txt, host)
	set_compatible_data_type(source, auxkey, type_const[['BLOCKTYPE_RAW_STRINGS']], host)

	if (is.na(fmt)) fmt <- .autofmt.[type]

	downlo <- basicTextGatherer()
	if (curlPerform(url			  = paste0(host, '//', source, '.', block_key, '.from_text/', type, ',', auxkey, ',', fmt),
					infilesize	  = 0,
					writefunction = downlo[[1]],
					customrequest = 'PUT') != 0) stop ('Http error status.')

	if (downlo$value() != '0') stop('Http PUT Not Acceptable.')

	delete_block(source, auxkey, host, silent = T)

	TRUE
}

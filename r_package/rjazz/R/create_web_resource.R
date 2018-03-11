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
create_web_resource <- function(web_source, url, type, raw_object, lang = NULL, host = .host.)
{
	if (class(raw_object) == 'character') raw_object <- charToRaw(raw_object)
	if (class(raw_object) != 'raw') stop("Don't know how to PUT raw_object.")

	downlo <- basicTextGatherer()
	upload <- charToRaw(web_source)
	if (curlPerform(url			  = paste0(host, '//www//new_websource'),
					infilesize	  = length(upload),
					readfunction  = upload,
					writefunction = downlo[[1]],
					upload		  = TRUE,
					customrequest = 'PUT') != 0) stop ('Http error status.')
	downlo$reset()
	upload <- charToRaw(url)
	block  <- new_key()
	if (curlPerform(url			  = paste0(host, '//www.', block,'.assign_url/', web_source),
					infilesize	  = length(upload),
					readfunction  = upload,
					writefunction = downlo[[1]],
					upload		  = TRUE,
					customrequest = 'PUT') != 0) stop ('Http error status.')
	if (downlo$value() != '0') stop('PUT function assign_url failed.')

	downlo$reset()
	upload <- raw(4)
	upload[1] <- as.raw(type)
	if (curlPerform(url			  = paste0(host, '//www.', block,'.assign_mime_type/', web_source),
					infilesize	  = 4,
					readfunction  = upload,
					writefunction = downlo[[1]],
					upload		  = TRUE,
					customrequest = 'PUT') != 0) stop ('Http error status.')
	if (downlo$value() != '0') stop('PUT function assign_mime_type failed.')

	if (!is.null(lang))
	{
		downlo$reset()
		upload <- charToRaw(lang)
		if (curlPerform(url			  = paste0(host, '//www.', block,'.assign_language/', web_source),
						infilesize	  = length(upload),
						readfunction  = upload,
						writefunction = downlo[[1]],
						upload		  = TRUE,
						customrequest = 'PUT') != 0) stop ('Http error status.')
		if (downlo$value() != '0') stop('PUT function assign_language failed.')
	}

	downlo$reset()
	if (curlPerform(url			  = paste0(host, '//www.', block),
					infilesize	  = length(raw_object),
					readfunction  = raw_object,
					writefunction = downlo[[1]],
					upload		  = TRUE,
					customrequest = 'PUT') != 0) stop ('Http error status.')

	if (downlo$value() != '0')
	{
		if (downlo$value() == '1') stop('Http PUT Not Acceptable.')

		stop ('Unexpected format in server response.')
	}

	return(block)
}

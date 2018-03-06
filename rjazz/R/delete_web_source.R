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
delete_web_source <- function(web_source, host = .host., silent = FALSE)
{
	downlo <- basicTextGatherer()
	upload <- charToRaw(web_source)
	if (curlPerform(url			  = paste0(host, '//www//delete_websource'),
					infilesize	  = length(upload),
					readfunction  = upload,
					writefunction = downlo[[1]],
					upload		  = TRUE,
					customrequest = 'PUT') != 0) stop ('Http error status.')

	if (downlo$value() != '0')
	{
		if (downlo$value() == '1')
		{
			if (silent) return(FALSE)

			stop('Http DELETE Not Acceptable.')
		}

		stop ('Unexpected format in server response.')
	}

	TRUE
}

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
library(rjazz)
library(RCurl)

options(warn=2)

# set_jazz_host('192.168.1.30:8888')
# set_jazz_host('192.168.1.19:8888')
# set_jazz_host('20.1.71.31:8888')


min_date <- NA
max_date <- NA

checkdate <- function(dat)
{
	idate <- strptime(dat, '%A, %d %b %Y %H:%M:%S GMT')

	if (is.na(idate) || class(idate)[1] != 'POSIXlt') stop('Wrong date.')

	if (is.na(min_date)) min_date <<- idate
	else				 min_date <<- min(min_date, idate)

	if (is.na(max_date)) max_date <<- idate
	else				 max_date <<- max(max_date, idate)
}


testhea <- function(hh, connection = 'Keep-Alive', contlength = '^[0-9]+$', conttype = 'text/plain; charset=utf-8', status = '200', statusmessage = 'OK')
{
	if (!grepl(contlength, hh['Content-Length'])) stop(paste('testhea() failed on Content-Length = ', hh['Content-Length']))

	if (connection	  != hh['Connection'])		  stop(paste('testhea() failed on Connection = ',	  hh['Connection']))
	if (status		  != hh['status'])			  stop(paste('testhea() failed on status = ',		  hh['status']))
	if (statusmessage != hh['statusMessage'])	  stop(paste('testhea() failed on statusMessage = ',  hh['statusMessage']))

	if (!is.na(conttype))
	{
		if (conttype != hh['Content-Type']) stop(paste('testhea() failed on Content-Type = ', hh['Content-Type']))
	} else {
		if (!is.na(hh['Content-Type'])) stop(paste('testhea() failed on Content-Type = ', hh['Content-Type']))
	}

	checkdate(hh['Date'])
}


get_head_from_url <- function(url)
{
	hea	 <- basicHeaderGatherer()
	down <- basicTextGatherer()

	if (curlPerform(url			   = paste0(.host., url),
					headerfunction = hea$update,
					writefunction  = down[[1]],
					nobody = T,
					customrequest  = 'HEAD') != 0) stop ('Http error status.')

	if (down$value() != '') stop ('HEAD should not download content.')

	hea$value()
}


.host. <- rjazz:::.host.


create_web_resource('test_head', '/test_head/index.html', type_const[['BLOCKTYPE_RAW_MIME_HTML']], '<html><body>Hello.</body></html>')

create_source('test_head')
create_block_seq('test_head', 'blk', 1L, 3L)

testhea(get_head_from_url('/test_head/index.html'), conttype = 'text/html')
testhea(get_head_from_url('/test_head/index.htm'), conttype = NA, status = '404', statusmessage = 'Not Found')

testhea(get_head_from_url('//www'), conttype = NA, status = '400', statusmessage = 'Bad Request')
testhea(get_head_from_url('//www/ls'), conttype = NA, status = '400', statusmessage = 'Bad Request')

testhea(get_head_from_url('//'))

testhea(get_head_from_url('//sys//server_vers'))
testhea(get_head_from_url('//sys//server_vers/full'))
testhea(get_head_from_url('//sys//ls'))

testhea(get_head_from_url('//www//ls'))
testhea(get_head_from_url('//test_head.blk'), conttype = 'application/octet-stream')
testhea(get_head_from_url('//test_head.blk.header'))
testhea(get_head_from_url('//test_head.blk.as_text/%d'))
testhea(get_head_from_url('//test_head.blk.as_R'), conttype = 'application/octet-stream')

testhea(get_head_from_url('//www/$'), conttype = NA, status = '400', statusmessage = 'Bad Request')
testhea(get_head_from_url('//www.key'), conttype = NA, status = '404', statusmessage = 'Not Found')
testhea(get_head_from_url('//www.key.fun'), conttype = NA, status = '406', statusmessage = 'Not Acceptable')

delete_source('test_head')
delete_web_source('test_head')
testhea(get_head_from_url('/test_head/index.html'), conttype = NA, status = '404', statusmessage = 'Not Found')


if (difftime(max_date, min_date, units = 'secs') > 5) stop('Slow or wrong.')

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


testopt <- function(hh, connection = 'Keep-Alive', allow = 'OPTIONS', server = '^Jazz .*', status = '204', statusmessage = 'No Content')
{
	if (connection	  != hh['Connection'])	  stop(paste('testopt() failed on Connection = ',	 hh['Connection']))
	if (allow		  != hh['Allow'])		  stop(paste('testopt() failed on Allow = ',		 hh['Allow']))
	if (!grepl(server, hh['Server']))		  stop(paste('testopt() failed on Server = ',		 hh['Server']))
	if (status		  != hh['status'])		  stop(paste('testopt() failed on status = ',		 hh['status']))
	if (statusmessage != hh['statusMessage']) stop(paste('testopt() failed on statusMessage = ', hh['statusMessage']))

	checkdate(hh['Date'])
}


get_option_from_url <- function(url)
{
	hea <- basicHeaderGatherer()

	if (curlPerform(url			   = paste0(.host., url),
					headerfunction = hea$update,
					customrequest  = 'OPTIONS') != 0) stop ('Http error status.')
	hea$value()
}


.host. <- rjazz:::.host.


create_web_resource('test_options', '/test_options/index.html', type_const[['BLOCKTYPE_RAW_MIME_HTML']], '<html><body>Hello.</body></html>')


testopt(get_option_from_url('/test_options/index.html'), allow = 'HEAD,GET,OPTIONS')
testopt(get_option_from_url('/test_options/index.htm'))

testopt(get_option_from_url('//'), allow = 'HEAD,GET,OPTIONS')
testopt(get_option_from_url('//www'), allow = 'DELETE,OPTIONS')
testopt(get_option_from_url('//www/'), allow = 'HEAD,GET,OPTIONS')
testopt(get_option_from_url('//www/$'))
testopt(get_option_from_url('//www.key'), allow = 'HEAD,GET,PUT,DELETE,OPTIONS')
testopt(get_option_from_url('//www.key.fun'), allow = 'HEAD,GET,PUT,OPTIONS')

delete_web_source('test_options')
testopt(get_option_from_url('/test_options/index.html'))


if (difftime(max_date, min_date, units = 'secs') > 5) stop('Slow or wrong.')

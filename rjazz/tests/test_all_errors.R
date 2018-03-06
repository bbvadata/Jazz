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


status <- function (fun, url, upload = NULL)
{
	hea	 <- basicHeaderGatherer()
	down <- basicTextGatherer()

	if (curlPerform(url			   = paste0(rjazz:::.host., url),
					headerfunction = hea$update,
					writefunction  = down[[1]],
					readfunction   = upload,
					infilesize	   = length(upload),
					nobody		   = fun == 'HEAD',
					upload		   = !is.null(upload),
					customrequest  = fun) != 0) stop ('Http error status.')

	as.integer(hea$value()['status'])
}


if (!('test_allerrors' %in% list_sources())) create_source('test_allerrors')


# SUCCESSFUL CODES:

if (status('GET', '//sys//server_vers/full') != 200) stop ('status(GET, *) failed.')

put_raw_block('test_allerrors', 'bbb', 'Hi!')
if (status('DELETE', '//test_allerrors.bbb') != 204) stop ('status(DELETE, *) failed.')
if (status('DELETE', '//test_allerrors.bbb') != 404) stop ('status(DELETE, *) failed.')

if (status('PUT', '//test_allerrors.bbb', charToRaw('Bye.')) != 201) stop ('status(PUT, *) failed.')

if (status('OPTIONS', '//test_allerrors.bbb')	  != 204) stop ('status(OPTIONS, *) failed.')
if (status('OPTIONS', '//test_allerrors.bbb.foo') != 204) stop ('status(OPTIONS, *) failed.')
if (status('OPTIONS', '//sys...')				  != 204) stop ('status(OPTIONS, *) failed.')
if (status('OPTIONS', '/s.htm')					  != 204) stop ('status(OPTIONS, *) failed.')
if (status('OPTIONS', '/&%$$$')					  != 204) stop ('status(OPTIONS, *) failed.')

rawToChar(get_raw_block('test_allerrors', 'bbb'))


# 400 (MHD_HTTP_BAD_REQUEST) - Syntactical error at top level. (Malformed URI)

if (status('GET',	 '//cis//server_vers/full') != 400) stop ('status(GET, *) failed.')
if (status('HEAD',	 '//cis//server_vers/full') != 400) stop ('status(HEAD, *) failed.')
if (status('PUT',	 '//cis..bbb', raw(1))		!= 400) stop ('status(PUT, *) failed.')
if (status('DELETE', '//cis..bbb')				!= 400) stop ('status(DELETE, *) failed.')


# 401 (MHD_HTTP_UNAUTHORIZED) - *RESERVED FOR* Client may be authorized after correct authentication.

# 403 (MHD_HTTP_FORBIDDEN)	  - Attempt the operate with the www API when www is disabled.

if (status('GET',	 '//sys.aaa') != 403) stop ('status(GET, *) failed.')
if (status('HEAD',	 '//sys.aaa') != 403) stop ('status(HEAD, *) failed.')
if (status('DELETE', '//sys')	  != 403) stop ('status(DELETE, *) failed.')
if (status('DELETE', '//www')	  != 403) stop ('status(DELETE, *) failed.')


# 404 (MHD_HTTP_NOT_FOUND) - Resource does not exist in the www API for HEAD or GET.

if (status('GET',	 '//www.aaa') != 404) stop ('status(GET, *) failed.')
if (status('HEAD',	 '//www.aaa') != 404) stop ('status(HEAD, *) failed.')
if (status('DELETE', '//www.aaa') != 404) stop ('status(DELETE, *) failed.')

if (status('GET',  '//test_allerrors.ccc.header') != 404) stop ('status(GET, *) failed.')
if (status('HEAD', '//test_allerrors.ccc.header') != 404) stop ('status(HEAD, *) failed.')


# 405 (MHD_HTTP_METHOD_NOT_ALLOWED) - Always for methods other than OPTIONS, HEAD, GET, PUT and DELETE.

create_web_resource('test_errors', url = '/index.html', type = type_const[['BLOCKTYPE_RAW_MIME_HTML']], raw_object = '<html>Hi.</html>')

if (status('GET',	 '/index.html') != 200) stop ('status(GET, *) failed.')
if (status('HEAD',	 '/index.html') != 200) stop ('status(HEAD, *) failed.')
if (status('PUT',	 '/index.html') != 405) stop ('status(PUT, *) failed.')
if (status('POST',	 '/index.html') != 405) stop ('status(POST, *) failed.')
if (status('DELETE', '/index.html') != 405) stop ('status(DELETE, *) failed.')

delete_web_source('test_errors')

if (status('GET',  '/index.html') != 404) stop ('status(GET, *) failed.')
if (status('HEAD', '/index.html') != 404) stop ('status(HEAD, *) failed.')


# 406 (MHD_HTTP_NOT_ACCEPTABLE) - PUT operation errors. PUT failed in some upload stage.

if (status('GET',  '//test_allerrors.bbb.foo')		  != 406) stop ('status(GET, *) failed.')
if (status('HEAD', '//test_allerrors.bbb.foo')		  != 406) stop ('status(HEAD, *) failed.')
if (status('GET',  '//test_allerrors.bbb.astext/foo') != 406) stop ('status(GET, *) failed.')
if (status('HEAD', '//test_allerrors.bbb.astext/foo') != 406) stop ('status(HEAD, *) failed.')

if (status('PUT', '//sys.aaa', raw(1)) != 406) stop ('status(PUT, *) failed.')


# 500 (MHD_HTTP_INTERNAL_SERVER_ERROR) - *RESERVED FOR* Errors during the processing of an answer such as unexpected error codes.

# 501 (MHD_HTTP_NOT_IMPLEMENTED)	   - This functionality is part of the API, but not yet implemented (Instrumental API with correct syntax).

if (status('GET',	 '//test_allerrors/bbb/foo') != 501) stop ('status(GET, *) failed.')
if (status('HEAD',	 '//test_allerrors/bbb/foo') != 501) stop ('status(GET, *) failed.')
if (status('DELETE', '//test_allerrors/bbb/foo') != 501) stop ('status(GET, *) failed.')

# 503 (MHD_HTTP_SERVICE_UNAVAILABLE)	  - All calls where enter_persistence() failed in the only call (GET, KILL) or any call (PUT)
# 509 (MHD_HTTP_BANDWIDTH_LIMIT_EXCEEDED) - *RESERVED FOR* User resource profiling.


delete_source('test_allerrors')

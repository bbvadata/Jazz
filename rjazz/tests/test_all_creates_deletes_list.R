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


compare_block <- function(blockid, fmt, with)
{
	tx1 <- strsplit(get_block_as_string('test_create', blockid, fmt), '\n')[[1]]
	tx2 <- as.character(with)

	if (length(tx1) != length(tx2)) stop('compare_block() falied: wrong length.')
	if (any(tx1 != tx2)) stop('compare_block() falied: wrong content.')
}


check_resource <- function (url, block_key, raw_obj, mime, lang = NA, exists = TRUE)
{
	err <- try (blk <- get_raw_block('www', block_key), silent = T)

	if (class(err) == 'try-error')
	{
		if (!exists) return (TRUE)

		stop('Block not found')
	}
	if (!exists) stop('Block should not exist.')
	if (any(blk != raw_obj)) stop('Wrong block in direct read.')

	hea <- basicHeaderGatherer()
	ret <- getBinaryURL(paste0(rjazz:::.host., url), .len = 1024, headerfunction = hea$update)

	if (any(ret != raw_obj)) stop('Wrong block in www API read.')

	hea <- hea$value()
	if (hea['status'] != 200) stop('Wrong status')
	if (hea['Content-Type'] != mime) stop('Wrong mime type')
	if (is.na(hea['Content-Language']))
	{
		if (!is.na(lang)) stop('Wrong language')
		return(TRUE)
	}

	if (hea['Content-Language'] != lang) stop('Wrong language')

	TRUE
}


raw_object <- function(int)
{
	charToRaw(as.character(paste0('i=', int)))
}


# Test sources
# ------------

ls <- list_sources()

while (length(ls) > 2)
{
	s  <- ls[ length(ls)]
	ls <- ls[-length(ls)]

	delete_source(s)
}

ss <- paste0('test_create', 1:9)

if (any(!sapply(ss, create_source))) stop('create_source() failed')

if (any(!(ss %in% list_sources()))) stop('source not found')

ls <- list_sources()

while (length(ls) > 2)
{
	s  <- ls[ length(ls)]
	ls <- ls[-length(ls)]

	delete_source(s)
}

if (!create_source('test_create')) stop('create failed (2)')


# Test blocks
# ------------

ok	<- (create_block_rep('test_create', 'boo01', TRUE, 5)
	&&	create_block_rep('test_create', 'boo02', FALSE, 3)
	&&	create_block_rep('test_create', 'int01', 4L, 7)
	&&	create_block_rep('test_create', 'rea01', 3.14, 6)
	&&	create_block_rep('test_create', 'str01', 'Hello world!', 4)
	&&	create_block_rep('test_create', 'str02', '', 2)
	&&	create_block_seq('test_create', 'int02', 1L, 9L)
	&&	create_block_seq('test_create', 'int03', 3L, 15L, 3L)
	&&	create_block_seq('test_create', 'int04', 8L, 0L, -2L)
	&&	create_block_seq('test_create', 'rea02', 1, 11)
	&&	create_block_seq('test_create', 'rea03', 1, 2, 0.1)
	&&	create_block_seq('test_create', 'rea04', 3, 2, -0.2))

if (!ok) stop('create_block_* failed')

compare_block('boo01', '%d\\n', rep(1, 5))
compare_block('boo02', '%d\\n', rep(0, 3))

compare_block('int01', '%d\\n', rep(4, 7))
compare_block('int02', '%d\\n', seq(1, 9))
compare_block('int03', '%d\\n', seq(3, 15, 3))
compare_block('int04', '%d\\n', seq(8, 0, -2))

compare_block('rea01', '%1.2f\\n', rep('3.14', 6))
compare_block('rea02', '%1.2f\\n', sprintf('%1.2f', 1:11))
compare_block('rea03', '%1.2f\\n', sprintf('%1.2f', seq(1, 2, 0.1)))
compare_block('rea04', '%1.2f\\n', sprintf('%1.2f', seq(3, 2, -0.2)))

compare_block('str01', '%s\\n', rep('Hello world!', 4))
compare_block('str02', '%s\\n', rep('', 2))

ok	<- (create_block_rep('test_create', 'boo01', FALSE, 1)
	&&	create_block_rep('test_create', 'int01', 1L, 1)
	&&	create_block_rep('test_create', 'rea01', 2, 1)
	&&	create_block_rep('test_create', 'str01', '1', 1))

if (!ok) stop('create_block_* failed')

compare_block('boo01', '%d\\n', 0)
compare_block('int01', '%d\\n', 1)
compare_block('rea01', '%1.2f\\n', '2.00')
compare_block('str01', '%s\\n', 1)


# Test web sources and web resources
# ----------------------------------

ws <- list_web_sources()

while (length(ws) > 0)
{
	delete_web_source(ws[1])
	ws <- list_web_sources()
}

blid <- ''
blid[1] <- create_web_resource('aaa', '/a/home', type_const[['BLOCKTYPE_RAW_MIME_HTML']], raw_object(1), lang = NULL)

if (list_web_sources() != 'aaa') stop('')

blid[2] <- create_web_resource('bbb', '/b/home',	type_const[['BLOCKTYPE_RAW_MIME_HTML']], raw_object(2), lang = 'en-US')
blid[3] <- create_web_resource('ccc', '/c/home',	type_const[['BLOCKTYPE_RAW_MIME_HTML']], raw_object(3), lang = 'es-ES')
blid[4] <- create_web_resource('aaa', '/a/pic.png', type_const[['BLOCKTYPE_RAW_MIME_PNG']],	 raw_object(4))
blid[5] <- create_web_resource('aaa', '/a/auto.js', type_const[['BLOCKTYPE_RAW_MIME_JS']],	 raw_object(5))
blid[6] <- create_web_resource('bbb', '/b/pic.jpg', type_const[['BLOCKTYPE_RAW_MIME_JPG']],	 raw_object(6))
blid[7] <- create_web_resource('xyz', '/xyz',		type_const[['BLOCKTYPE_RAW_MIME_TXT']],	 raw_object(7), lang = 'tr-TR')
blid[8] <- create_web_resource('ijk', '/ijk',		type_const[['BLOCKTYPE_RAW_ANYTHING']],	 raw_object(8))

if (any(!grepl('^[0-9]+$', blid))) stop('create_web_resource() failed')

if (any(list_web_sources() != c('aaa', 'bbb', 'ccc', 'ijk', 'xyz'))) stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html')
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'en-US')
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES')
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png')
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript')
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg')
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR')
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream')

blid[2] <- create_web_resource('bbb', '/b/home',	type_const[['BLOCKTYPE_RAW_MIME_HTML']], raw_object(2), lang = 'es-ES')
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'es-ES')

if (!delete_web_source('aaa')) stop('delete failed.')

if (any(list_web_sources() != c('bbb', 'ccc', 'ijk', 'xyz'))) stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html', exists = F)
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'es-ES')
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES')
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png', exists = F)
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript', exists = F)
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg')
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR')
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream')

if (!delete_web_source('bbb')) stop('delete failed.')

if (any(list_web_sources() != c('ccc', 'ijk', 'xyz'))) stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html', exists = F)
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'en-US', exists = F)
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES')
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png', exists = F)
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript', exists = F)
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg', exists = F)
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR')
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream')

if (!delete_web_source('ccc')) stop('delete failed.')

if (any(list_web_sources() != c('ijk', 'xyz'))) stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html', exists = F)
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'en-US', exists = F)
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES', exists = F)
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png', exists = F)
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript', exists = F)
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg', exists = F)
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR')
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream')

if (!delete_web_source('ijk')) stop('delete failed.')

if (list_web_sources() != 'xyz') stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html', exists = F)
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'en-US', exists = F)
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES', exists = F)
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png', exists = F)
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript', exists = F)
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg', exists = F)
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR')
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream', exists = F)

if (!delete_web_source('xyz')) stop('delete failed.')

if (length(list_web_sources()) != 0) stop('Unexpected sources')

check_resource('/a/home',	 blid[1], raw_object(1), 'text/html', exists = F)
check_resource('/b/home',	 blid[2], raw_object(2), 'text/html', lang = 'en-US', exists = F)
check_resource('/c/home',	 blid[3], raw_object(3), 'text/html', lang = 'es-ES', exists = F)
check_resource('/a/pic.png', blid[4], raw_object(4), 'image/png', exists = F)
check_resource('/a/auto.js', blid[5], raw_object(5), 'application/javascript', exists = F)
check_resource('/b/pic.jpg', blid[6], raw_object(6), 'image/jpeg', exists = F)
check_resource('/xyz',		 blid[7], raw_object(7), 'text/plain', lang = 'tr-TR', exists = F)
check_resource('/ijk',		 blid[8], raw_object(8), 'application/octet-stream', exists = F)


# Test error pages
# ----------------

ok	<- (create_error_page(400, raw_object(400))
	&&	create_error_page(403, raw_object(403))
	&&	create_error_page(404, raw_object(404)))

if (!ok) stop('create_error_page() failed.')

if (any(getBinaryURL(paste0(rjazz:::.host., '//oops.block.fun')) != raw_object(400))) stop('400 error failed')
if (any(getBinaryURL(paste0(rjazz:::.host., '//sys.block')) != raw_object(403))) stop('403 error failed')
if (any(getBinaryURL(paste0(rjazz:::.host., '/google.mars/search?anyone_there?')) != raw_object(404))) stop('404 error failed')


# Cleanup
# -------

blk <- c('boo01', 'boo02', 'int01', 'rea01', 'str01', 'str02', 'int02', 'int03', 'int04', 'rea02', 'rea03', 'rea04')

for (b in blk)
{
	get_block_as_string('test_create', b, 'hi')

	if (!delete_block('test_create', b)) stop('delete_block() failed.')

	ret <- try(get_block_as_string('test_create', b, 'hi'), silent = T)
	if (class(ret) != 'try-error') stop('Block should not exist.')
}

if (!delete_source('test_create')) stop('delete_source() failed')
if (length(list_sources()) != 2) stop('list_sources() failed')

if (delete_source('sys', silent = T)) stop('Not allowed')
if (delete_source('w', silent = T)) stop('Not allowed')
if (length(list_sources()) != 2) stop('list_sources() failed')

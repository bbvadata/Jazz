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


check_block_attributes <- function(block, type, len, size, flags, hash64)
{
	ba <- get_block_attributes('test_putget', block)

	if (ba$type	  != type  || class(ba$type)   != 'numeric') stop(paste('Wrong type',	ba$type))
	if (ba$length != len   || class(ba$length) != 'numeric') stop(paste('Wrong length', ba$length))
	if (ba$size	  != size  || class(ba$size)   != 'numeric') stop(paste('Wrong size',	ba$size))
	if (ba$flags  != flags || class(ba$flags)  != 'numeric') stop(paste('Wrong flags',	ba$flags))

	if (is.na(hash64)) return(TRUE)

	if (ba$hash64 != hash64 || class(ba$hash64) != 'character') stop(paste('Wrong hash64',	ba$hash64))
}

check_block_as_strings <- function(block, fmt, tx2)
{
	tx1 <- strsplit(get_block_as_string('test_putget', block, fmt), '\n')[[1]]

	if (length(tx1) != length(tx2)) stop ('Wrong length')
	if (any(tx1 != tx2))			stop ('Wrong content')
}

hash_seq <- function(from, to, by)
{
	create_block_seq('test_putget', '_xxx_', from, to, by)

	get_block_attributes('test_putget', '_xxx_')['hash64']
}

assert_identical <- function(o1, o2)
{
	if (class(o1)  != class(o2))  stop('Wrong class')
	if (length(o1) != length(o2)) stop('Wrong length')

	ix1 <- which(is.na(o1))
	ix2 <- which(is.na(o2))
	if (length(ix1)	> 0)
	{
		assert_identical(ix1, ix2)

		if (any(o1[-ix1] != o2[-ix2])) stop('Wrong content')
	} else {
		if (any(o1 != o2)) stop('Wrong content')
	}
}


if (!('test_putget' %in% list_sources())) create_source('test_putget')

ok	<- (create_block_rep('test_putget', 'boo01', TRUE, 5)
	&&	create_block_rep('test_putget', 'int01', 123456789L, 3)
	&&	create_block_rep('test_putget', 'rea01', 1.23456789, 1)
	&&	create_block_rep('test_putget', 'str01', '123456789!', 7)
	&&	create_block_seq('test_putget', 'int02', 123456L, 789000000L, 356789L)
	&&	create_block_seq('test_putget', 'int03', 789000000L, -555555L, -356789L)
	&&	create_block_seq('test_putget', 'rea02', 1, 2, 0.1234567890123)
	&&	create_block_seq('test_putget', 'rea03', 3, 2, -0.245678901234))

if (!ok) stop('create_block_* failed')

ok	<- (put_block_flags('test_putget', 'boo01',	 299792458)
	&&	put_block_flags('test_putget', 'int01',	  19141918)
	&&	put_block_flags('test_putget', 'str01', 1234567890)
	&&	put_block_flags('test_putget', 'rea01',	 914445556))

if (!ok) stop('put_block_flags failed')


# get_server_version()
# --------------------

if (get_server_version() != rjazz:::.version.) stop('Strict version check failed: run ./config.R')

cc <- c('version', 'build', 'artifact', 'myname', 'sysname', 'hostname', 'kernel', 'sysvers', 'machine')
sv <- get_server_version(full = T)

if (any(sapply(cc, function(x) is.na(sv[x]) || sv[x] == ''))) stop('Something missing in get_server_version(full).')


# get_block_attributes() & put_block_flags()
# ------------------------------------------

l1 <- 5
l2 <- 3
l3 <- 1
l4 <- 7
l5 <- length(seq(123456L, 789000000L, 356789L))
l6 <- length(seq(789000000L, -555555L, -356789L))
l7 <- length(seq(1, 2, 0.1234567890123))
l8 <- length(seq(3, 2, -0.245678901234))

check_block_attributes('boo01', type_const[['BLOCKTYPE_C_BOOL']],		l1,	   5,  299792458, NA)
check_block_attributes('int01', type_const[['BLOCKTYPE_C_R_INTEGER']],	l2,	  12,	19141918, NA)
check_block_attributes('rea01', type_const[['BLOCKTYPE_C_R_REAL']],		l3,	   8,  914445556, NA)
check_block_attributes('str01', type_const[['BLOCKTYPE_C_OFFS_CHARS']], l4,	  48, 1234567890, NA)
check_block_attributes('int02', type_const[['BLOCKTYPE_C_R_INTEGER']],	l5, l5*4,		   0, hash_seq(123456L, 789000000L, 356789L))
check_block_attributes('int03', type_const[['BLOCKTYPE_C_R_INTEGER']],	l6, l6*4,		   0, hash_seq(789000000L, -555555L, -356789L))
check_block_attributes('rea02', type_const[['BLOCKTYPE_C_R_REAL']],		l7, l7*8,		   0, hash_seq(1, 2, 0.1234567890123))
check_block_attributes('rea03', type_const[['BLOCKTYPE_C_R_REAL']],		l8, l8*8,		   0, hash_seq(3, 2, -0.245678901234))


# get_R_block()
# -------------

boo1 <- get_R_block('test_putget', 'boo01')

if (any(boo1 != TRUE)) stop('boo1 <- get_R_block() failed')

int1 <- get_R_block('test_putget', 'int01')

if (any(int1 != 123456789L)) stop('int1 <- get_R_block() failed')

int2 <- get_R_block('test_putget', 'int02')

if (any(int2 != seq(123456L, 789000000L, 356789L))) stop('int2 <- get_R_block() failed')

int3 <- get_R_block('test_putget', 'int03')

if (any(int3 != seq(789000000L, -555555L, -356789L))) stop('int3 <- get_R_block() failed')

rea1 <- get_R_block('test_putget', 'rea01')

if (any(rea1 != 1.23456789)) stop('rea1 <- get_R_block() failed')

rea2 <- get_R_block('test_putget', 'rea02')

if (any(rea2 != seq(1, 2, 0.1234567890123))) stop('rea2 <- get_R_block() failed')

rea3 <- get_R_block('test_putget', 'rea03')

if (any(rea3 != seq(3, 2, -0.245678901234))) stop('rea3 <- get_R_block() failed')

str1 <- get_R_block('test_putget', 'str01')

if (any(str1 != '123456789!')) stop('str1 <- get_R_block() failed')

ret <- try(str1 <- get_R_block('test_putget', 'str_xx_9876'), silent = T)

if (class(ret) != 'try-error') stop('Block not found failed.')

ret <- try(str1 <- get_R_block('test_putXXX', 'str_xx_9876'), silent = T)

if (class(ret) != 'try-error') stop('Source not found failed.')

if (!put_raw_block('test_putget', 'as_raw', 'anything')) stop ('put_raw_block() failed.')

ret <- try(str1 <- get_R_block('test_putget', 'as_raw'), silent = T)

if (class(ret) != 'try-error') stop('Wrong block type failed.')


# get_block_as_string()
# ---------------------

check_block_as_strings('boo01', '%d\\n',	sprintf('%d', rep(1, 5)))
check_block_as_strings('int01', '%d\\n',	sprintf('%d', rep(123456789L, 3)))
check_block_as_strings('rea01', '%1.6f\\n', sprintf('%1.6f', rep(1.23456789, 1)))
check_block_as_strings('str01', '%s\\n',	sprintf('%s', rep('123456789!', 7)))
check_block_as_strings('int02', '%d\\n',	sprintf('%d', seq(123456L, 789000000L, 356789L)))
check_block_as_strings('int03', '%d\\n',	sprintf('%d', seq(789000000L, -555555L, -356789L)))
check_block_as_strings('rea02', '%1.6f\\n', sprintf('%1.6f', seq(1, 2, 0.1234567890123)))
check_block_as_strings('rea03', '%1.6f\\n', sprintf('%1.6f', seq(3, 2, -0.245678901234)))

ret <- try(str1 <- get_block_as_string('test_putget', 'str_xx_9876', '%d\\n'), silent = T)

if (class(ret) != 'try-error') stop('Block not found failed (2).')

ret <- try(str1 <- get_block_as_string('test_putXXX', 'str_xx_9876', '%d\\n'), silent = T)

if (class(ret) != 'try-error') stop('Source not found failed (2).')

if (!put_raw_block('test_putget', 'as_raw', 'anything')) stop ('put_raw_block() failed (2).')

ret <- try(str1 <- get_block_as_string('test_putget', 'as_raw', '%d\\n'), silent = T)

if (class(ret) != 'try-error') stop('Wrong block type failed (2).')


# get_raw_block()
# ---------------

rr <- get_raw_block('test_putget', 'boo01')
if (class(rr) != 'raw' || length(rr) != 5) stop ('get_raw_block() || length')

rr <- get_raw_block('test_putget', 'int01')
if (class(rr) != 'raw' || length(rr) != 12) stop ('get_raw_block() || length (2)')

rr <- get_raw_block('test_putget', 'rea01')
if (class(rr) != 'raw' || length(rr) != 8) stop ('get_raw_block() || length (3)')

rr <- get_raw_block('test_putget', 'str01')
if (class(rr) != 'raw' || length(rr) != get_block_attributes('test_putget', 'str01')$size) stop ('get_raw_block() || length (4)')

rr <- get_raw_block('test_putget', 'int02')
if (class(rr) != 'raw' || length(rr) != 4*length(seq(123456L, 789000000L, 356789L))) stop ('get_raw_block() || length (5)')

rr <- get_raw_block('test_putget', 'int03')
if (class(rr) != 'raw' || length(rr) != 4*length(seq(789000000L, -555555L, -356789L))) stop ('get_raw_block() || length (6)')

rr <- get_raw_block('test_putget', 'rea02')
if (class(rr) != 'raw' || length(rr) != 8*length(seq(1, 2, 0.1234567890123))) stop ('get_raw_block() || length (7)')

rr <- get_raw_block('test_putget', 'rea03')
if (class(rr) != 'raw' || length(rr) != 8*length(seq(3, 2, -0.245678901234))) stop ('get_raw_block() || length (8)')

ret <- try(str1 <- get_raw_block('test_putget', 'str_xx_9876'), silent = T)

if (class(ret) != 'try-error') stop('Block not found failed (3).')

ret <- try(str1 <- get_raw_block('test_putXXX', 'str_xx_9876'), silent = T)

if (class(ret) != 'try-error') stop('Source not found failed (3).')

if (!put_raw_block('test_putget', 'as_raw', 'anything')) stop ('put_raw_block() failed (3.1).')
if (!put_raw_block('test_putget', 'as_Raw', serialize(1:5, NULL))) stop ('put_raw_block() failed (3.2).')

if (rawToChar(get_raw_block('test_putget', 'as_raw')) != 'anything') stop('put_raw_block() content (3.1)')
if (any(unserialize(get_raw_block('test_putget', 'as_Raw')) != 1:5)) stop('put_raw_block() content (3.2)')


# set_compatible_data_type()
# --------------------------

if (!put_raw_block('test_putget', 'raw01', as.raw(1))) stop('put_raw_block() failed (4)')

ok <- (set_compatible_data_type('test_putget', 'boo01', type_const[['BLOCKTYPE_C_BOOL']])
	&& set_compatible_data_type('test_putget', 'int01', type_const[['BLOCKTYPE_C_R_FACTOR']])
	&& set_compatible_data_type('test_putget', 'int01', type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& set_compatible_data_type('test_putget', 'int01', type_const[['BLOCKTYPE_C_R_GRADE']])
	&& set_compatible_data_type('test_putget', 'rea01', type_const[['BLOCKTYPE_C_R_TIMESEC']])
	&& set_compatible_data_type('test_putget', 'rea01', type_const[['BLOCKTYPE_C_R_REAL']])
	&& set_compatible_data_type('test_putget', 'str01', type_const[['BLOCKTYPE_C_OFFS_CHARS']])
	&& set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_C_R_GRADE']])
	&& set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_C_R_FACTOR']])
	&& set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_C_R_FACTOR']])
	&& set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_C_R_GRADE']])
	&& set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& set_compatible_data_type('test_putget', 'rea02', type_const[['BLOCKTYPE_C_R_REAL']])
	&& set_compatible_data_type('test_putget', 'rea02', type_const[['BLOCKTYPE_C_R_TIMESEC']])
	&& set_compatible_data_type('test_putget', 'rea03', type_const[['BLOCKTYPE_C_R_TIMESEC']])
	&& set_compatible_data_type('test_putget', 'rea03', type_const[['BLOCKTYPE_C_R_REAL']])
	&& set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_RAW_ANYTHING']])
	&& set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_RAW_STRINGS']])
	&& set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_RAW_R_RAW']])
	&& set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_RAW_MIME_MP4']])
	&& set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_RAW_MIME_ICO']]))

if (!ok) stop('set_compatible_data_type failed')

check_block_attributes('boo01', type_const[['BLOCKTYPE_C_BOOL']],		l1,	   5,  299792458, NA)
check_block_attributes('int01', type_const[['BLOCKTYPE_C_R_GRADE']],	l2,	  12,	19141918, NA)
check_block_attributes('rea01', type_const[['BLOCKTYPE_C_R_REAL']],		l3,	   8,  914445556, NA)
check_block_attributes('str01', type_const[['BLOCKTYPE_C_OFFS_CHARS']], l4,	  48, 1234567890, NA)
check_block_attributes('int02', type_const[['BLOCKTYPE_C_R_FACTOR']],	l5, l5*4,		   0, hash_seq(123456L, 789000000L, 356789L))
check_block_attributes('int03', type_const[['BLOCKTYPE_C_R_INTEGER']],	l6, l6*4,		   0, hash_seq(789000000L, -555555L, -356789L))
check_block_attributes('rea02', type_const[['BLOCKTYPE_C_R_TIMESEC']],	l7, l7*8,		   0, hash_seq(1, 2, 0.1234567890123))
check_block_attributes('rea03', type_const[['BLOCKTYPE_C_R_REAL']],		l8, l8*8,		   0, hash_seq(3, 2, -0.245678901234))


ret <- c(class(try(set_compatible_data_type('test_putget', 'boo01', type_const[['BLOCKTYPE_RAW_MIME_APK']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int01', type_const[['BLOCKTYPE_C_BOOL']]),		 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int09', type_const[['BLOCKTYPE_C_R_GRADE']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int01', type_const[['BLOCKTYPE_C_R_REAL']]),	 silent = T)),

		 class(try(set_compatible_data_type('test_putget', 'rea01', type_const[['BLOCKTYPE_C_R_INTEGER']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'rea01', type_const[['BLOCKTYPE_C_OFFS_CHARS']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'str01', type_const[['BLOCKTYPE_C_R_FACTOR']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_RAW_ANYTHING']]), silent = T)),

		 class(try(set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_C_R_REAL']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int02', type_const[['BLOCKTYPE_RAW_MIME_MP4']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_C_OFFS_CHARS']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_C_R_TIMESEC']]),	 silent = T)),

		 class(try(set_compatible_data_type('test_putget', 'int03', type_const[['BLOCKTYPE_RAW_MIME_ICO']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'rea02', type_const[['BLOCKTYPE_RAW_ANYTHING']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'rea02', type_const[['BLOCKTYPE_RAW_STRINGS']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'rea03', type_const[['BLOCKTYPE_RAW_R_RAW']]),	 silent = T)),

		 class(try(set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_C_BOOL']]),		 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_C_R_REAL']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_C_R_FACTOR']]),	 silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'raw01', type_const[['BLOCKTYPE_C_OFFS_CHARS']]), silent = T)),
		 class(try(set_compatible_data_type('test_putget', 'raw11', type_const[['BLOCKTYPE_RAW_MIME_ICO']]), silent = T)))

if (any(ret != 'try-error')) stop('set_compatible_data_type wrong type not failed.')


# put_raw_block()
# ---------------

o_bn <- 1:9 > 3
o_in <- 1:5
o_rn <- 1:4*pi
o_sn <- paste0('x', o_in[-2])

ok <- (put_raw_block('test_putget', 'blk_b1', serialize(o_bn, NULL))
	&& put_raw_block('test_putget', 'blk_i1', serialize(o_in, NULL))
	&& put_raw_block('test_putget', 'blk_r1', serialize(o_rn, NULL))
	&& put_raw_block('test_putget', 'blk_s1', serialize(o_sn, NULL))
	&& put_raw_block('test_putget', 'blk_s2', o_sn))

if (!ok) stop('put_raw_block failed')

assert_identical(o_bn, unserialize(get_raw_block('test_putget', 'blk_b1')))
assert_identical(o_in, unserialize(get_raw_block('test_putget', 'blk_i1')))
assert_identical(o_rn, unserialize(get_raw_block('test_putget', 'blk_r1')))
assert_identical(o_sn, unserialize(get_raw_block('test_putget', 'blk_s1')))
assert_identical(o_sn, strsplit(rawToChar(get_raw_block('test_putget', 'blk_s2')), '\n')[[1]])


# put_R_block()
# -------------

o_bn <- c(T, F, T, T, F, F, F)
o_in <- c(1L, 4:6, -2L, 66L)
o_rn <- c(pi, 3.3, 299.792458, -0.1)
o_sn <- c('This', 'is', 'good', '', 'a b c 1 2 3 &%$ Ñ áéíóú')

ok <- (put_R_block('test_putget', 'blkR_b1', o_bn)
	&& put_R_block('test_putget', 'blkR_i1', o_in)
	&& put_R_block('test_putget', 'blkR_r1', o_rn)
	&& put_R_block('test_putget', 'blkR_s1', o_sn))

if (!ok) stop('put_R_block failed')

assert_identical(o_bn, get_R_block('test_putget', 'blkR_b1'))
assert_identical(o_in, get_R_block('test_putget', 'blkR_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'blkR_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'blkR_s1'))

assert_identical(o_bn, strsplit(get_block_as_string('test_putget', 'blkR_b1', '%d\\n'), '\n')[[1]] == "1")
assert_identical(o_in, as.integer(strsplit(get_block_as_string('test_putget', 'blkR_i1', '%d\\n'), '\n')[[1]]))
assert_identical(sprintf('%1.6f', o_rn), strsplit(get_block_as_string('test_putget', 'blkR_r1', '%1.6f\\n'), '\n')[[1]])
assert_identical(o_sn, strsplit(get_block_as_string('test_putget', 'blkR_s1', '%s\\n'), '\n')[[1]])


# put_strings_as_block()
# ----------------------

o_bn <- c('1', '0', '1', '1', '0', '0', '0')
o_in <- c('1', '4', '5', '6', '-2', '66')
o_rn <- c('3.141593', '3.3', '299.792458', '-0.1')
o_sn <- c('This', 'is', 'good', '', 'a b c 1 2 3 &%$ Ñ áéíóú')

ok <- (put_strings_as_block('test_putget', 'blkT_b1', o_bn, type_const[['BLOCKTYPE_C_BOOL']])
	&& put_strings_as_block('test_putget', 'blkT_i1', o_in, type_const[['BLOCKTYPE_C_R_FACTOR']])
	&& put_strings_as_block('test_putget', 'blkT_i2', o_in, type_const[['BLOCKTYPE_C_R_GRADE']])
	&& put_strings_as_block('test_putget', 'blkT_i3', o_in, type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& put_strings_as_block('test_putget', 'blkT_r1', o_rn, type_const[['BLOCKTYPE_C_R_TIMESEC']])
	&& put_strings_as_block('test_putget', 'blkT_r2', o_rn, type_const[['BLOCKTYPE_C_R_REAL']])
	&& put_strings_as_block('test_putget', 'blkT_s1', o_sn, type_const[['BLOCKTYPE_C_OFFS_CHARS']]))

if (!ok) stop('put_strings_as_block failed')

assert_identical(as.logical(o_bn == 1), get_R_block('test_putget', 'blkT_b1'))
assert_identical(as.integer(o_in),		get_R_block('test_putget', 'blkT_i1'))
assert_identical(as.integer(o_in),		get_R_block('test_putget', 'blkT_i2'))
assert_identical(as.integer(o_in),		get_R_block('test_putget', 'blkT_i3'))
assert_identical(as.numeric(o_rn),		get_R_block('test_putget', 'blkT_r1'))
assert_identical(as.numeric(o_rn),		get_R_block('test_putget', 'blkT_r2'))
assert_identical(o_sn,					get_R_block('test_putget', 'blkT_s1'))

assert_identical(o_bn, strsplit(get_block_as_string('test_putget', 'blkT_b1', '%d\\n'), '\n')[[1]])
assert_identical(o_in, strsplit(get_block_as_string('test_putget', 'blkT_i1', '%d\\n'), '\n')[[1]])
assert_identical(o_in, strsplit(get_block_as_string('test_putget', 'blkT_i2', '%d\\n'), '\n')[[1]])
assert_identical(o_in, strsplit(get_block_as_string('test_putget', 'blkT_i3', '%d\\n'), '\n')[[1]])
assert_identical(as.numeric(o_rn), as.numeric(strsplit(get_block_as_string('test_putget', 'blkT_r1', '%1.6f\\n'), '\n')[[1]]))
assert_identical(as.numeric(o_rn), as.numeric(strsplit(get_block_as_string('test_putget', 'blkT_r2', '%1.6f\\n'), '\n')[[1]]))
assert_identical(o_sn, strsplit(get_block_as_string('test_putget', 'blkT_s1', '%s\\n'), '\n')[[1]])


# length=0 for all types, all puts and all gets
# ---------------------------------------------

o_bn <- logical()
o_in <- integer()
o_rn <- numeric()
o_sn <- character()

ok <- (put_raw_block('test_putget', 'empty_b1', serialize(o_bn, NULL))
	&& put_raw_block('test_putget', 'empty_i1', serialize(o_in, NULL))
	&& put_raw_block('test_putget', 'empty_r1', serialize(o_rn, NULL))
	&& put_raw_block('test_putget', 'empty_s1', serialize(o_sn, NULL))
	&& put_raw_block('test_putget', 'empty_s2', o_sn))

if (!ok) stop('empty put_raw_block failed')

ok <- (put_R_block('test_putget', 'emptR_b1', o_bn)
	&& put_R_block('test_putget', 'emptR_i1', o_in)
	&& put_R_block('test_putget', 'emptR_r1', o_rn)
	&& put_R_block('test_putget', 'emptR_s1', o_sn))

if (!ok) stop('empty put_R_block failed')

ok <- (put_strings_as_block('test_putget', 'emptT_b1', o_sn, type_const[['BLOCKTYPE_C_BOOL']])
	&& put_strings_as_block('test_putget', 'emptT_i1', o_sn, type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& put_strings_as_block('test_putget', 'emptT_r1', o_sn, type_const[['BLOCKTYPE_C_R_REAL']])
	&& put_strings_as_block('test_putget', 'emptT_s1', o_sn, type_const[['BLOCKTYPE_C_OFFS_CHARS']]))

if (!ok) stop('empty put_strings_as_block failed')

ok <- (create_block_rep('test_putget', 'emptQ_b1', TRUE, 0)
	&& create_block_rep('test_putget', 'emptQ_i1', 4L, 0)
	&& create_block_rep('test_putget', 'emptQ_r1', 3.14, 0)
	&& create_block_rep('test_putget', 'emptQ_s1', '', 0))

if (!ok) stop('empty create_block_rep failed')

assert_identical(o_bn, unserialize(get_raw_block('test_putget', 'empty_b1')))
assert_identical(o_in, unserialize(get_raw_block('test_putget', 'empty_i1')))
assert_identical(o_rn, unserialize(get_raw_block('test_putget', 'empty_r1')))
assert_identical(o_sn, unserialize(get_raw_block('test_putget', 'empty_s1')))
assert_identical(raw(0), get_raw_block('test_putget', 'empty_s2'))

assert_identical(o_bn, get_R_block('test_putget', 'emptR_b1'))
assert_identical(o_in, get_R_block('test_putget', 'emptR_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'emptR_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'emptR_s1'))

assert_identical(o_bn, get_R_block('test_putget', 'emptT_b1'))
assert_identical(o_in, get_R_block('test_putget', 'emptT_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'emptT_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'emptT_s1'))

assert_identical(o_bn, get_R_block('test_putget', 'emptQ_b1'))
assert_identical(o_in, get_R_block('test_putget', 'emptQ_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'emptQ_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'emptQ_s1'))

assert_identical('', get_block_as_string('test_putget', 'emptR_b1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptR_i1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptR_r1', '%f'))
assert_identical('', get_block_as_string('test_putget', 'emptR_s1', '%s'))

assert_identical('', get_block_as_string('test_putget', 'emptT_b1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptT_i1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptT_r1', '%f'))
assert_identical('', get_block_as_string('test_putget', 'emptT_s1', '%s'))

assert_identical('', get_block_as_string('test_putget', 'emptQ_b1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptQ_i1', '%d'))
assert_identical('', get_block_as_string('test_putget', 'emptQ_r1', '%f'))
assert_identical('', get_block_as_string('test_putget', 'emptQ_s1', '%s'))

check_block_attributes('emptR_b1', type_const[['BLOCKTYPE_C_BOOL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptR_i1', type_const[['BLOCKTYPE_C_R_INTEGER']],  0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptR_r1', type_const[['BLOCKTYPE_C_R_REAL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptR_s1', type_const[['BLOCKTYPE_C_OFFS_CHARS']], 0,16, 0, "d5bbd50c662f1b45")

check_block_attributes('emptT_b1', type_const[['BLOCKTYPE_C_BOOL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptT_i1', type_const[['BLOCKTYPE_C_R_INTEGER']],  0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptT_r1', type_const[['BLOCKTYPE_C_R_REAL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptT_s1', type_const[['BLOCKTYPE_C_OFFS_CHARS']], 0,16, 0, "d5bbd50c662f1b45")

check_block_attributes('emptQ_b1', type_const[['BLOCKTYPE_C_BOOL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptQ_i1', type_const[['BLOCKTYPE_C_R_INTEGER']],  0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptQ_r1', type_const[['BLOCKTYPE_C_R_REAL']],	   0, 0, 0, "bcebb9e2e0fe0786")
check_block_attributes('emptQ_s1', type_const[['BLOCKTYPE_C_OFFS_CHARS']], 0,16, 0, "d5bbd50c662f1b45")


# NAs for all types, all puts and all gets
# ----------------------------------------

o_bn <- logical();	 o_bn[4] <- NA; o_bn[2] <- T
o_in <- integer();	 o_in[4] <- NA; o_in[2] <- 1L
o_rn <- numeric();	 o_rn[4] <- NA; o_rn[2] <- 1.0
o_sn <- character(); o_sn[4] <- NA; o_sn[2] <- '1'

n_sn <- c('NA', '1', 'NA', 'NA')

ok <- (put_raw_block('test_putget', 'partNA_b1', serialize(o_bn, NULL))
	&& put_raw_block('test_putget', 'partNA_i1', serialize(o_in, NULL))
	&& put_raw_block('test_putget', 'partNA_r1', serialize(o_rn, NULL))
	&& put_raw_block('test_putget', 'partNA_s1', serialize(o_sn, NULL))
	&& put_raw_block('test_putget', 'partNA_s2', o_sn))

if (!ok) stop('partNA put_raw_block failed')

ok <- (put_R_block('test_putget', 'partNR_b1', o_bn)
	&& put_R_block('test_putget', 'partNR_i1', o_in)
	&& put_R_block('test_putget', 'partNR_r1', o_rn)
	&& put_R_block('test_putget', 'partNR_s1', o_sn))

if (!ok) stop('partNA put_R_block failed')

ok <- (put_strings_as_block('test_putget', 'partNT_b1', o_sn, type_const[['BLOCKTYPE_C_BOOL']])
	&& put_strings_as_block('test_putget', 'partNT_i1', o_sn, type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& put_strings_as_block('test_putget', 'partNT_r1', o_sn, type_const[['BLOCKTYPE_C_R_REAL']])
	&& put_strings_as_block('test_putget', 'partNT_s1', o_sn, type_const[['BLOCKTYPE_C_OFFS_CHARS']]))

if (!ok) stop('partNA put_strings_as_block failed')

assert_identical(o_bn, unserialize(get_raw_block('test_putget', 'partNA_b1')))
assert_identical(o_in, unserialize(get_raw_block('test_putget', 'partNA_i1')))
assert_identical(o_rn, unserialize(get_raw_block('test_putget', 'partNA_r1')))
assert_identical(o_sn, unserialize(get_raw_block('test_putget', 'partNA_s1')))
assert_identical(n_sn, strsplit(rawToChar(get_raw_block('test_putget', 'partNA_s2')), '\n')[[1]])

assert_identical(o_bn, get_R_block('test_putget', 'partNR_b1'))
assert_identical(o_in, get_R_block('test_putget', 'partNR_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'partNR_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'partNR_s1'))

assert_identical(o_bn, get_R_block('test_putget', 'partNT_b1'))
assert_identical(o_in, get_R_block('test_putget', 'partNT_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'partNT_r1'))
assert_identical(n_sn, get_R_block('test_putget', 'partNT_s1'))

assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNR_b1', '%d\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNR_i1', '%d\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNR_r1', '%1.0f\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNR_s1', '%s\\n'), '\n')[[1]])

assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNT_b1', '%d\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNT_i1', '%d\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNT_r1', '%1.0f\\n'), '\n')[[1]])
assert_identical(n_sn, strsplit(get_block_as_string('test_putget', 'partNT_s1', '%s\\n'), '\n')[[1]])


o_bn <- logical();	 o_bn[3] <- NA
o_in <- integer();	 o_in[5] <- NA
o_rn <- numeric();	 o_rn[4] <- NA
o_sn <- character(); o_sn[6] <- NA

ok <- (put_raw_block('test_putget', 'fullNA_b1', serialize(o_bn, NULL))
	&& put_raw_block('test_putget', 'fullNA_i1', serialize(o_in, NULL))
	&& put_raw_block('test_putget', 'fullNA_r1', serialize(o_rn, NULL))
	&& put_raw_block('test_putget', 'fullNA_s1', serialize(o_sn, NULL))
	&& put_raw_block('test_putget', 'fullNA_s2', o_sn))

if (!ok) stop('fullNA put_raw_block failed')

ok <- (put_R_block('test_putget', 'fullNR_b1', o_bn)
	&& put_R_block('test_putget', 'fullNR_i1', o_in)
	&& put_R_block('test_putget', 'fullNR_r1', o_rn)
	&& put_R_block('test_putget', 'fullNR_s1', o_sn))

if (!ok) stop('fullNA put_R_block failed')

ok <- (put_strings_as_block('test_putget', 'fullNT_b1', o_sn[1:3], type_const[['BLOCKTYPE_C_BOOL']])
	&& put_strings_as_block('test_putget', 'fullNT_i1', o_sn[1:5], type_const[['BLOCKTYPE_C_R_INTEGER']])
	&& put_strings_as_block('test_putget', 'fullNT_r1', o_sn[1:4], type_const[['BLOCKTYPE_C_R_REAL']])
	&& put_strings_as_block('test_putget', 'fullNT_s1', o_sn[1:6], type_const[['BLOCKTYPE_C_OFFS_CHARS']]))

if (!ok) stop('fullNA put_strings_as_block failed')

ok <- (create_block_rep('test_putget', 'fullNQ_b1', o_bn[1], 3)
	&& create_block_rep('test_putget', 'fullNQ_i1', o_in[1], 5)
	&& create_block_rep('test_putget', 'fullNQ_r1', o_rn[1], 4)
	&& create_block_rep('test_putget', 'fullNQ_s1', o_sn[1], 6))

if (!ok) stop('fullNA create_block_rep failed')

assert_identical(o_bn, unserialize(get_raw_block('test_putget', 'fullNA_b1')))
assert_identical(o_in, unserialize(get_raw_block('test_putget', 'fullNA_i1')))
assert_identical(o_rn, unserialize(get_raw_block('test_putget', 'fullNA_r1')))
assert_identical(o_sn, unserialize(get_raw_block('test_putget', 'fullNA_s1')))
assert_identical(rep('NA', 6), strsplit(rawToChar(get_raw_block('test_putget', 'fullNA_s2')), '\n')[[1]])

assert_identical(o_bn, get_R_block('test_putget', 'fullNR_b1'))
assert_identical(o_in, get_R_block('test_putget', 'fullNR_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'fullNR_r1'))
assert_identical(o_sn, get_R_block('test_putget', 'fullNR_s1'))

assert_identical(o_bn, get_R_block('test_putget', 'fullNT_b1'))
assert_identical(o_in, get_R_block('test_putget', 'fullNT_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'fullNT_r1'))
assert_identical(rep('NA', 6), get_R_block('test_putget', 'fullNT_s1'))

assert_identical(o_bn, get_R_block('test_putget', 'fullNQ_b1'))
assert_identical(o_in, get_R_block('test_putget', 'fullNQ_i1'))
assert_identical(o_rn, get_R_block('test_putget', 'fullNQ_r1'))
assert_identical(rep('NA', 6), get_R_block('test_putget', 'fullNQ_s1'))

assert_identical(rep('NA', 3), strsplit(get_block_as_string('test_putget', 'fullNR_b1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 5), strsplit(get_block_as_string('test_putget', 'fullNR_i1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 4), strsplit(get_block_as_string('test_putget', 'fullNR_r1', '%1.0f\\n'), '\n')[[1]])
assert_identical(rep('NA', 6), strsplit(get_block_as_string('test_putget', 'fullNR_s1', '%s\\n'), '\n')[[1]])

assert_identical(rep('NA', 3), strsplit(get_block_as_string('test_putget', 'fullNT_b1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 5), strsplit(get_block_as_string('test_putget', 'fullNT_i1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 4), strsplit(get_block_as_string('test_putget', 'fullNT_r1', '%1.0f\\n'), '\n')[[1]])
assert_identical(rep('NA', 6), strsplit(get_block_as_string('test_putget', 'fullNT_s1', '%s\\n'), '\n')[[1]])

assert_identical(rep('NA', 3), strsplit(get_block_as_string('test_putget', 'fullNQ_b1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 5), strsplit(get_block_as_string('test_putget', 'fullNQ_i1', '%d\\n'), '\n')[[1]])
assert_identical(rep('NA', 4), strsplit(get_block_as_string('test_putget', 'fullNQ_r1', '%1.0f\\n'), '\n')[[1]])
assert_identical(rep('NA', 6), strsplit(get_block_as_string('test_putget', 'fullNQ_s1', '%s\\n'), '\n')[[1]])


delete_source('test_putget')

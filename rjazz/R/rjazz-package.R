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
.host.	  <- '127.0.0.1:8888'
.version. <- '0.1.07'
.rep_fun. <- c('C_bool_rep', 'C_integer_rep', 'C_real_rep', 'C_chars_rep')
.seq_fun. <- c('C_integer_seq', 'C_real_seq')

names(.rep_fun.) <- c('logical', 'integer', 'numeric', 'character')
names(.seq_fun.) <- c('integer', 'numeric')

.autofmt. <- c('%d', '', '%i', '%i', '%i', '%lf', '%lf')


type_const <- list(BLOCKTYPE_C_BOOL			= 1,
				   BLOCKTYPE_C_OFFS_CHARS	= 2,
				   BLOCKTYPE_C_R_FACTOR		= 3,
				   BLOCKTYPE_C_R_GRADE		= 4,
				   BLOCKTYPE_C_R_INTEGER	= 5,
				   BLOCKTYPE_C_R_TIMESEC	= 6,
				   BLOCKTYPE_C_R_REAL		= 7,

				   BLOCKTYPE_RAW_ANYTHING	= 10,
				   BLOCKTYPE_RAW_STRINGS	= 11,
				   BLOCKTYPE_RAW_R_RAW		= 12,
				   BLOCKTYPE_RAW_MIME_APK	= 13,
				   BLOCKTYPE_RAW_MIME_CSS	= 14,
				   BLOCKTYPE_RAW_MIME_CSV	= 15,
				   BLOCKTYPE_RAW_MIME_GIF	= 16,
				   BLOCKTYPE_RAW_MIME_HTML	= 17,
				   BLOCKTYPE_RAW_MIME_IPA	= 18,
				   BLOCKTYPE_RAW_MIME_JPG	= 19,
				   BLOCKTYPE_RAW_MIME_JS	= 20,
				   BLOCKTYPE_RAW_MIME_JSON	= 21,
				   BLOCKTYPE_RAW_MIME_MP4	= 22,
				   BLOCKTYPE_RAW_MIME_PDF	= 23,
				   BLOCKTYPE_RAW_MIME_PNG	= 24,
				   BLOCKTYPE_RAW_MIME_TSV	= 25,
				   BLOCKTYPE_RAW_MIME_TXT	= 26,
				   BLOCKTYPE_RAW_MIME_XAP	= 27,
				   BLOCKTYPE_RAW_MIME_XML	= 28,
				   BLOCKTYPE_RAW_MIME_ICO	= 29,

				   BLOCKTYPE_FUN_R			= 31,
				   BLOCKTYPE_FUN_DFQL		= 32,
				   BLOCKTYPE_FUN_DFQO		= 33)

.unlockbind <- function(sym, nsp)
{
	eval(parse(text = 'unlockBinding(sym, nsp)'))
}

.intToRaw <- function(i)
{
	rr <- raw(4)
	rr[1] <- as.raw(bitwAnd(i, 255))
	rr[2] <- as.raw(bitwAnd(bitwShiftR(i, 8), 255))
	rr[3] <- as.raw(bitwAnd(bitwShiftR(i, 16), 255))
	rr[4] <- as.raw(bitwAnd(bitwShiftR(i, 24), 255))
	rr
}

.niceFmt <- function(x)
{
	if (class(x) == 'integer') return (sprintf('%d', x[1]))

	format(x[1], digits = 16, scientific = F)
}

/* BBVA - Jazz: A lightweight analytical web server for data-driven applications.
   ------------

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

using namespace std;

/*! \brief This module does all block conversions specified in the API.

This includes a lower layer of memory only functions and an intermediate layer of lmdb persisted functions. The lower layer includes R compatible
functions (using SEXP and the same functions using serialized images) and all the text conversion functions. The upper layer has API functionality
move here tho make the top API PUT function (exec_block_put_function()) simpler.
*/

#include "src/include/jazz_commons.h"
#include "src/jazz_blocks/jazz_blocks.h"

/*~ end of automatic header ~*/

#ifndef jzz_IG_JZZBLOCKCONV
#define jzz_IG_JZZBLOCKCONV


struct R_binary						///< The header of any R object
{
	unsigned short signature;		///< The constant two chars: "X\\n"
	int			   format_version;	///< The constant 0x00, 0x00, 0x00, 0x02
	int			   writer;			///< R 3.2.5	0x00, 0x03, 0x02, 0x05
	int			   min_reader;		///< R 2.3.0	0x00, 0x02, 0x03, 0x00
	int			   R_type;			///< The SEXP type (E.g., LGLSXP for boolean)
	int			   R_length;		///< Number of elements (R booleans are 32 bit long)
}__attribute__((packed));


struct RStr_header					///< The header of each individual R string
{
	int signature;					///< The constant four bytes: 00 04 00 09
	int nchar;						///< Number of characters in each string.
};


union pRStr_stream					///< A pointer to the R stream usable as both RStr_header_int and char.
{
	char * pchar;
	RStr_header * phead;
};


/** Internal binary const related with R serialization.
*/
#define sw_RBINARY_SIGNATURE		0x0a58
#define sw_RBINARY_FORMATVERSION	0x2000000
#define sw_RBINARY_WRITER			0x5020300
#define sw_RBINARY_MINREADER		0x30200
#define sw_CHARSXP_HEA_TO_R			0x09000400
#define sw_CHARSXP_HEA_FROM_R		0x09000000
#define sw_CHARSXP_MASK_FROM_R		0xff000000
#define sw_CHARSXP_NA				0x09000000
#define sw_CHARSXP_NA_LENGTH		0xFFFFffff
#define sw_ONE						0x1000000
#define sw_NA_LOGICAL				0x80


/** NA converted as output.
*/
#define LENGTH_NA_AS_TEXT	3		///< The length of NA_AS_TEXT
#define NA_AS_TEXT			"NA\n"	///< The output produced by translate_block_TO_TEXT() for all NA values.


class jzzBLOCKCONV: public jzzBLOCKS {

	public:

		 jzzBLOCKCONV();
		~jzzBLOCKCONV();

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();

		bool translate_block_FROM_R	   (pJazzBlock psrc, pJazzBlock &pdest);
		bool translate_block_TO_R	   (pJazzBlock psrc, pJazzBlock	&pdest);

		bool translate_block_FROM_TEXT (pJazzBlock psrc, pJazzBlock &pdest, int type, char * fmt);
		bool translate_block_TO_TEXT   (pJazzBlock psrc, pJazzBlock &pdest, const char * fmt);

		bool cast_lmdb_block	(int source, persistedKey key, int type);
		bool set_lmdb_blockflags(int source, persistedKey key, int flags);
		bool set_lmdb_fromtext	(int source, persistedKey key, int type, persistedKey source_key, char * fmt);
		bool set_lmdb_fromR		(int source, persistedKey key, persistedKey source_key);

	private:

		typedef jzzBLOCKS super;

};

#endif

/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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

#include "src/jazz_elements/jazz_stdcore.h"

namespace jazz_stdcore
{

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
	int n_char;						///< Number of characters in each string.
};


union pRStr_stream					///< A pointer to the R stream usable as both RStr_header_int and char.
{
	char 		*p_char;
	RStr_header *p_head;
};


/** Internal binary const related with R serialization.
*/
#define R_SIG_RBINARY_SIGNATURE		0x0a58
#define R_SIG_RBINARY_FORMATVERSION	0x2000000
#define R_SIG_RBINARY_WRITER		0x5020300
#define R_SIG_RBINARY_MINREADER		0x30200
#define R_SIG_CHARSXP_HEA_TO_R		0x09000400
#define R_SIG_CHARSXP_HEA_FROM_R	0x09000000
#define R_SIG_CHARSXP_MASK_FROM_R	0xff000000
#define R_SIG_CHARSXP_NA			0x09000000
#define R_SIG_CHARSXP_NA_LENGTH		0xFFFFffff
#define R_SIG_ONE					0x1000000
#define R_SIG_NA_LOGICAL			0x80


/** NA converted as output.
*/
#define LENGTH_NA_AS_TEXT	3		///< The length of NA_AS_TEXT
#define NA_AS_TEXT			"NA\n"	///< The output produced by translate_block_TO_TEXT() for all NA values.


/**
//TODO: Document JazzCoreTypecasting()
*/
JazzCoreTypecasting::JazzCoreTypecasting(jazz_utils::pJazzLogger a_logger)	: JazzObject(a_logger)
{
//TODO: Implement JazzCoreTypecasting
}


/**
//TODO: Document ~JazzCoreTypecasting()
*/
JazzCoreTypecasting::~JazzCoreTypecasting()
{
//TODO: Implement ~JazzCoreTypecasting
}


bool JazzCoreTypecasting::FromR (pJazzBlock p_source, pJazzBlock &p_dest)
{

}


bool JazzCoreTypecasting::ToR (pJazzBlock p_source, pJazzBlock &p_dest)
{

}


bool JazzCoreTypecasting::FromText (pJazzBlock p_source, pJazzBlock &p_dest, int type, char *fmt)
{

}


bool JazzCoreTypecasting::ToText (pJazzBlock p_source, pJazzBlock &p_dest, const char *fmt)
{

}

} // namespace jazz_stdcore


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_stdcore.ctest"
#endif

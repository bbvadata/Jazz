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

#include <arpa/inet.h>

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
#define NA_AS_TEXT			"NA\n"	///< The output produced by JazzCoreTypecasting::ToText() for all NA values.


/** Constants identifying R object types
*/
#define CHARSXP					   9	///< "scalar" string type (internal only)
#define LGLSXP					  10	///< logical vectors
#define INTSXP					  13	///< integer vectors
#define REALSXP					  14	///< real variables
#define STRSXP					  16	///< string vectors


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


/** Create a block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS from a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW binary
compatible with a serialized R object.

	\param p_source	An R object serialized as a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW
	\param p_dest 	Address of a pJazzBlock allocated by this function to store the block.

	\return			true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the p_source object except copying it.
	The returned p_dest is owned by the caller and must be JAZZFREE()ed when no longer necessary.

//TODO: Update doc of JazzCoreTypecasting::FromR when complete.

*/
bool JazzCoreTypecasting::FromR (pJazzBlock p_source, pJazzBlock &p_dest)
{
	R_binary * p_head = (R_binary *) &p_source->tensor.cell_byte[0];

	if (p_head->signature != R_SIG_RBINARY_SIGNATURE || p_head->format_version != R_SIG_RBINARY_FORMATVERSION) {
		log(LOG_MISS, "JazzCoreTypecasting::FromR() : Wrong signature || format_version.");

		return false;
	}

	int type = htonl(p_head->R_type), R_len = htonl(p_head->R_length);
/*
	switch (type) {
		case LGLSXP:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_BOOL, R_len);
				if (!ok) {
					log(LOG_MISS, "JazzCoreTypecasting::FromR() : JAZZALLOC(RAM_ALLOC_C_BOOL) failed.");

					return false;
				}
				int	 *			p_data_src  = (int *)		   &p_head[1];
				unsigned char * p_data_dest = (unsigned char *) &reinterpret_cast<pBoolBlock>(p_dest)->data;
				for (int i = 0; i < R_len; i++) {
					if (p_data_src[i]) {
						if (p_data_src[i] == R_SIG_ONE) p_data_dest[i] = 1;
						else						p_data_dest[i] = JAZZ_BOOLEAN_NA;
					}
					else							p_data_dest[i] = 0;
				}
			}
			break;

		case INTSXP:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_INTEGER, R_len);
				if (!ok) {
					log(LOG_MISS, "JazzCoreTypecasting::FromR() : JAZZALLOC(RAM_ALLOC_C_INTEGER) failed.");

					return false;
				}
				int * p_data_src	 = (int *) &p_head[1];
				int * p_data_dest = (int *) &reinterpret_cast<pIntBlock>(p_dest)->data;
				for (int i = 0; i < R_len; i++)
					p_data_dest[i] = htonl(p_data_src[i]);
			}
			break;

		case STRSXP:
			{
				// Allocation: R serializes all strings without final zero and with 8 trailing bytes -> string_buffer + Nx(4 bytes + 2 trailing)
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_OFFS_CHARS, p_source->size - sizeof(R_binary) + sizeof(string_buffer) + 8 - 2*R_len);
				if (!ok) {
					log(LOG_MISS, "JazzCoreTypecasting::FromR() : JAZZALLOC(RAM_ALLOC_C_OFFS_CHARS) failed.");

					return false;
				}
				if (!R_len) {
					memset(&reinterpret_cast<pCharBlock>(p_dest)->data, 0, p_dest->size);
					return true;
				}

				if (!format_C_OFFS_CHARS((pCharBlock) p_dest, R_len)) {
					JAZZFREE(p_dest, AUTOTYPEBLOCK(p_dest));

					log(LOG_MISS, "JazzCoreTypecasting::FromR() : format_C_OFFS_CHARS() failed.");

					return false;
				}
				pRStr_stream src;
				src.p_char = (char *) &p_head[1];
				for (int i = 0; i < R_len; i++) {
					RStr_header rh = *src.phead++;

					if (rh.signature == R_SIG_CHARSXP_NA && rh.n_char == R_SIG_CHARSXP_NA_LENGTH) {
						reinterpret_cast<pCharBlock>(p_dest)->data[i] = JAZZ_STRING_NA;
					} else {
						if ((rh.signature & R_SIG_CHARSXP_MASK_FROM_R) == R_SIG_CHARSXP_HEA_FROM_R) {
							int n_char = htonl(rh.n_char);
							if (n_char >= MAX_STRING_LENGTH || n_char < 0) {
								JAZZFREE(p_dest, AUTOTYPEBLOCK(p_dest));

								log(LOG_MISS, "JazzCoreTypecasting::FromR() : String too long.");

								return false;
							}
							if (n_char == 0) {
								reinterpret_cast<pCharBlock>(p_dest)->data[i] = JAZZ_STRING_EMPTY;
							} else {
								reinterpret_cast<pCharBlock>(p_dest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) p_dest, src.p_char, n_char);

								src.p_char += n_char;
							}
						} else {
							JAZZFREE(p_dest, AUTOTYPEBLOCK(p_dest));

							log_printf(LOG_MISS, "JazzCoreTypecasting::FromR() : Unexpected signature = %4X", rh.signature);

							return false;
						}
					}
				}
			}
			break;

		case REALSXP:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_REAL, R_len);
				if (!ok) {
					log(LOG_MISS, "JazzCoreTypecasting::FromR() : JAZZALLOC(RAM_ALLOC_C_REAL) failed.");

					return false;
				}
				int * p_data_src	 = (int *) &p_head[1];
				int * p_data_dest = (int *) &reinterpret_cast<pRealBlock>(p_dest)->data;
				for (int i = 0; i < R_len; i++) {
					p_data_dest[1] = ntohl(*p_data_src++);
					p_data_dest[0] = ntohl(*p_data_src++);

					p_data_dest += 2;
				}
			}
			break;

		default:
		{
			log(LOG_MISS, "JazzCoreTypecasting::FromR() : Wrong type.");

			return false;
		}
	}
*/

//TODO: Complete refactoring of JazzCoreTypecasting::FromR

	return false;
}


/** Create a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW binary compatible with a serialized R object from a block_C_BOOL, block_C_R_INTEGER,
block_C_R_REAL or block_C_OFFS_CHARS.

	\param p_source	The source block (A block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS).
	\param p_dest 	Address of a pJazzBlock allocated by this function to store the block.

	\return		 	true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the p_source object except copying it.
	The returned p_dest is owned by the caller and must be JAZZFREE()ed when no longer necessary.

//TODO: Update doc of JazzCoreTypecasting::ToR when complete.

*/
bool JazzCoreTypecasting::ToR (pJazzBlock p_source, pJazzBlock &p_dest)
{
	int size = sizeof(R_binary);

	switch (p_source->cell_type) {
		case CELL_TYPE_BOOLEAN:
		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_INTEGER:
			size += p_source->size*sizeof(int);
			break;

		case CELL_TYPE_JAZZ_STRING:
			for (int i = 0; i < p_source->size; i++)
				size += 2*sizeof(int) + strlen(p_source->get_string(i));
			break;

		case CELL_TYPE_JAZZ_TIME:
		case CELL_TYPE_DOUBLE:
			size += p_source->size*sizeof(double);
			break;

		default:
			log(LOG_MISS, "JazzCoreTypecasting::ToR(): Unsupported type.");

			return false;
	}
/*
	bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_RAW, size);
	if (!ok) {
		log(LOG_MISS, "JazzCoreTypecasting::ToR(): JAZZALLOC failed.");

		return false;

	}
	reinterpret_cast_block(p_dest, BLOCKTYPE_RAW_R_RAW);
	R_binary * pt = (R_binary *) &reinterpret_cast<pRawBlock>(p_dest)->data;

	pt->signature	   = R_SIG_RBINARY_SIGNATURE;
	pt->format_version = R_SIG_RBINARY_FORMATVERSION;
	pt->writer		   = R_SIG_RBINARY_WRITER;
	pt->min_reader	   = R_SIG_RBINARY_MINREADER;
	pt->R_length	   = ntohl(p_source->length);

	switch (p_source->cell_type)
	{
		case CELL_TYPE_BOOLEAN:
			{
				int	 * p_data_dest = (int *) &pt[1];
				unsigned char * p_data_src  = (unsigned char *) &reinterpret_cast<pBoolBlock>(p_source)->data;
				pt->R_type = ntohl(LGLSXP);
				for (int i = 0; i < p_source->length; i++) {
					if (p_data_src[i]) {
						if (p_data_src[i] == 1) p_data_dest[i] = R_SIG_ONE;
						else				    p_data_dest[i] = R_SIG_NA_LOGICAL;
					}
					else					    p_data_dest[i] = 0;
				}
			}
			break;

		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_INTEGER:
			{
				int * p_data_dest = (int *) &pt[1];
				int * p_data_src	 = (int *) &reinterpret_cast<pIntBlock>(p_source)->data;
				pt->R_type = ntohl(INTSXP);
				for (int i = 0; i < p_source->length; i++)
					p_data_dest[i] = ntohl(p_data_src[i]);
			}
			break;

		case CELL_TYPE_JAZZ_STRING:
			{
				pRStr_stream dest;
				dest.p_char = (char *) &pt[1];
				RStr_header sth;
				pt->R_type = ntohl(STRSXP);
				for (int i = 0; i < p_source->length; i++) {
					if (reinterpret_cast<pCharBlock>(p_source)->data[i] != JAZZ_STRING_NA) {
						char * p_data_src = PCHAR(reinterpret_cast<pCharBlock>(p_source), i);
						int len = strlen(p_data_src);
						sth.n_char = ntohl(len);
						sth.signature = R_SIG_CHARSXP_HEA_TO_R;
						*dest.phead++ = sth;
						for (int j = 0; j < len; j ++)
							*dest.p_char++ = p_data_src[j];
					} else {
						sth.n_char	  = R_SIG_CHARSXP_NA_LENGTH;
						sth.signature = R_SIG_CHARSXP_NA;
						*dest.phead++ = sth;
					}
				}
			}
			break;

		case CELL_TYPE_JAZZ_TIME:
		case CELL_TYPE_DOUBLE:
			{
				int * p_data_dest = (int *) &pt[1];
				int * p_data_src	 = (int *) &reinterpret_cast<pRealBlock>(p_source)->data;
				pt->R_type = ntohl(REALSXP);
				for (int i = 0; i < p_source->length; i++) {
					p_data_dest[1] = ntohl(*p_data_src++);
					p_data_dest[0] = ntohl(*p_data_src++);

					p_data_dest += 2;
				}
			}
	}
*/

//TODO: Complete refactoring of JazzCoreTypecasting::ToR

	return false;
}


/** Create a block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS from a block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV,
BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or BLOCKTYPE_RAW_MIME_XML.

	\param p_source	The source block (A block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV, BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or
					BLOCKTYPE_RAW_MIME_XML).
	\param p_dest 	Address of a pJazzBlock allocated by this function to store the block.
	\param type	 	The output type (CELL_TYPE_BOOLEAN, CELL_TYPE_JAZZ_STRING, BLOCKTYPE_C_R_FACTOR, BLOCKTYPE_C_R_GRADE, BLOCKTYPE_C_R_INTEGER,
					BLOCKTYPE_C_R_TIMESEC or BLOCKTYPE_C_R_REAL).
	\param fmt		The (sscanf() compatible) format for converting the data. Each row of text must be convertible to one element of the appropriate
					data type. E.g., " %lf," will convert " 1.23," to a double == 1.23

	\return			true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the p_source object except copying it.
	The returned p_dest is owned by the caller and must be JAZZFREE()ed when no longer necessary.

//TODO: Update doc of JazzCoreTypecasting::FromText when complete.

*/
bool JazzCoreTypecasting::FromText (pJazzBlock p_source, pJazzBlock &p_dest, int type, char *fmt)
{
	bool copy = fmt[0] == 0;

	if (copy && type != CELL_TYPE_JAZZ_STRING) {
		log(LOG_MISS, "JazzCoreTypecasting::FromText() : fmt == '' is only valid for strings.");

		return false;
	}
/*
	char buff[MAX_STRING_LENGTH + 4];
	char * p_char = (char *) &reinterpret_cast<pRawBlock>(p_source)->data;

	int bytes  = p_source->size;
	int length = 0;
	int n_char;

	while (bytes > 0) {
		length++;
		n_char = (uintptr_t) strchrnul(p_char, '\n') - (uintptr_t) p_char + 1;
		n_char = min(n_char, bytes);

		if (n_char >= MAX_STRING_LENGTH) {
			log(LOG_MISS, "JazzCoreTypecasting::FromText() : String too long.");

			return false;
		}

		p_char += n_char;
		bytes -= n_char;
	}

	p_char = (char *) &reinterpret_cast<pRawBlock>(p_source)->data;
	bytes = p_source->size;

	switch (type)
	{
		case CELL_TYPE_BOOLEAN:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_BOOL, length);
				if (!ok)
				{
					log(LOG_MISS, "JazzCoreTypecasting::FromText() : JAZZALLOC(RAM_ALLOC_C_BOOL) failed.");

					return false;
				}
				unsigned char * p_data_dest = (unsigned char *) &reinterpret_cast<pBoolBlock>(p_dest)->data;
				int i = 0;
				while (bytes > 0)
				{
					n_char = (uintptr_t) strchrnul(p_char, '\n') - (uintptr_t) p_char + 1;
					n_char = min(n_char, bytes);

					strncpy(buff, p_char, n_char);
					buff[n_char] = 0;
					if (sscanf(buff, fmt, &p_data_dest[i]) != 1)
						p_data_dest[i] = JAZZ_BOOLEAN_NA;

					p_char += n_char;
					bytes -= n_char;
					i++;
				}
			}
			break;

		case CELL_TYPE_JAZZ_STRING:
			{
				// Allocation: string_buffer + Nx(4 bytes + 2 trailing)
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_OFFS_CHARS, p_source->size + sizeof(string_buffer) + 8 + 6*length);
				if (!ok)
				{
					log(LOG_MISS, "JazzCoreTypecasting::FromText() : JAZZALLOC(RAM_ALLOC_C_OFFS_CHARS) failed.");

					return false;
				}
				if (!length)
				{
					memset(&reinterpret_cast<pCharBlock>(p_dest)->data, 0, p_dest->size);
					return true;
				}
				if (!format_C_OFFS_CHARS((pCharBlock) p_dest, length))
				{
					JAZZFREE(p_dest, AUTOTYPEBLOCK(p_dest));

					log(LOG_MISS, "JazzCoreTypecasting::FromText() : format_C_OFFS_CHARS() failed.");

					return false;
				}
				char buff2[MAX_STRING_LENGTH + 4];
				int i = 0;
				while (bytes > 0)
				{
					n_char = (uintptr_t) strchrnul(p_char, '\n') - (uintptr_t) p_char + 1;
					n_char = min(n_char, bytes);

					strncpy(buff, p_char, n_char);
					buff[n_char] = 0;
					if (copy)
					{
						int nc = n_char == bytes ? n_char : n_char - 1;
						reinterpret_cast<pCharBlock>(p_dest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) p_dest, buff, nc);
					}
					else
					{
						if (sscanf(buff, fmt, buff2) == 1)
						{
							reinterpret_cast<pCharBlock>(p_dest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) p_dest, buff2, strlen(buff2));
						}
						else
						{
							reinterpret_cast<pCharBlock>(p_dest)->data[i] = JAZZ_STRING_NA;
						}
					}

					p_char += n_char;
					bytes -= n_char;
					i++;
				}
			}
			break;

		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_INTEGER:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_INTEGER, length);
				if (!ok)
				{
					log(LOG_MISS, "JazzCoreTypecasting::FromText() : JAZZALLOC(RAM_ALLOC_C_INTEGER) failed.");

					return false;
				}
				p_dest->cell_type = type;
				int * p_data_dest = (int *) &reinterpret_cast<pIntBlock>(p_dest)->data;
				int i = 0;
				while (bytes > 0)
				{
					n_char = (uintptr_t) strchrnul(p_char, '\n') - (uintptr_t) p_char + 1;
					n_char = min(n_char, bytes);

					strncpy(buff, p_char, n_char);
					buff[n_char] = 0;
					if (sscanf(buff, fmt, &p_data_dest[i]) != 1)
						p_data_dest[i] = JAZZ_INTEGER_NA;

					p_char += n_char;
					bytes -= n_char;
					i++;
				}
			}
			break;

		case CELL_TYPE_JAZZ_TIME:
		case CELL_TYPE_DOUBLE:
			{
				bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_REAL, length);
				if (!ok)
				{
					log(LOG_MISS, "JazzCoreTypecasting::FromText() : JAZZALLOC(RAM_ALLOC_C_REAL) failed.");

					return false;
				}
				p_dest->cell_type = type;
				double * p_data_dest = (double *) &reinterpret_cast<pRealBlock>(p_dest)->data;
				int i = 0;
				while (bytes > 0)
				{
					n_char = (uintptr_t) strchrnul(p_char, '\n') - (uintptr_t) p_char + 1;
					n_char = min(n_char, bytes);

					strncpy(buff, p_char, n_char);
					buff[n_char] = 0;
					if (sscanf(buff, fmt, &p_data_dest[i]) != 1)
						p_data_dest[i] = JAZZ_DOUBLE_NA;

					p_char += n_char;
					bytes -= n_char;
					i++;
				}
			}
			break;

		default:
			log(LOG_MISS, "JazzCoreTypecasting::FromText() : Unexpected block type.");

			return false;
	}

	return true;
*/
}


/** Create a block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV, BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or BLOCKTYPE_RAW_MIME_XML from a
block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS.

	\param p_source	 The source block (A block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS).
	\param p_dest Address of a pJazzBlock allocated by this function to store the block.
	\param fmt	 The (sprintf() compatible) format for converting the data, must be compatible with the source type.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the p_source object except copying it.
	The returned p_dest is owned by the caller and must be JAZZFREE()ed when no longer necessary.
*/
bool JazzCoreTypecasting::ToText (pJazzBlock p_source, pJazzBlock &p_dest, const char *fmt)
{
/*
	char buff_item[MAX_STRING_LENGTH];
	int size = 0;

	switch (p_source->cell_type)
	{
		case CELL_TYPE_BOOLEAN:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pBoolBlock>(p_source)->data[i] == JAZZ_BOOLEAN_NA)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pBoolBlock>(p_source)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		case CELL_TYPE_JAZZ_STRING:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pCharBlock>(p_source)->data[i] == JAZZ_STRING_NA)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, PCHAR(reinterpret_cast<pCharBlock>(p_source), i));
					size += strlen(buff_item);
				}
			}
			break;

		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_INTEGER:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pIntBlock>(p_source)->data[i] == JAZZ_INTEGER_NA)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pIntBlock>(p_source)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		case CELL_TYPE_JAZZ_TIME:
		case CELL_TYPE_DOUBLE:
			for (int i = 0; i < p_source->length; i++)
			{
				if (R_IsNA(reinterpret_cast<pRealBlock>(p_source)->data[i]))
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pRealBlock>(p_source)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		default:
			log(LOG_MISS, "JazzCoreTypecasting::ToText(): Unexpected block cell_type.");

			return false;
	}

	bool ok = JAZZALLOC(p_dest, RAM_ALLOC_C_RAW, size + 8);	// Needed by sprintf() to add the extra 0.
	p_dest->size = size;

	if (!ok)
	{
		log(LOG_MISS, "JazzCoreTypecasting::ToText(): JAZZALLOC failed.");

		return false;

	}
	reinterpret_cast_block(p_dest, BLOCKTYPE_RAW_STRINGS);
	char * pt = (char *) &reinterpret_cast<pRawBlock>(p_dest)->data;

	switch (p_source->cell_type)
	{
		case CELL_TYPE_BOOLEAN:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pBoolBlock>(p_source)->data[i] == JAZZ_BOOLEAN_NA)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pBoolBlock>(p_source)->data[i]);
					pt += strlen(pt);
				}
			}
			break;

		case CELL_TYPE_JAZZ_STRING:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pCharBlock>(p_source)->data[i] == JAZZ_STRING_NA)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, PCHAR(reinterpret_cast<pCharBlock>(p_source), i));
					pt += strlen(pt);
				}
			}
			break;

		case CELL_TYPE_JAZZ_TIME:
		case CELL_TYPE_DOUBLE:
			for (int i = 0; i < p_source->length; i++)
			{
				if (R_IsNA(reinterpret_cast<pRealBlock>(p_source)->data[i]))
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pRealBlock>(p_source)->data[i]);
					pt += strlen(pt);
				}
			}
			break;

		default:
			for (int i = 0; i < p_source->length; i++)
			{
				if (reinterpret_cast<pIntBlock>(p_source)->data[i] == JAZZ_INTEGER_NA)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pIntBlock>(p_source)->data[i]);
					pt += strlen(pt);
				}
			}
	}
	pt[0] = 0;

	return true;
*/
}

} // namespace jazz_stdcore


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_stdcore.ctest"
#endif

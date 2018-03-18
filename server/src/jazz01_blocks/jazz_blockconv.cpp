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

#include "src/jazz01_blocks/jazz_blockconv.h"

/*~ end of automatic header ~*/

#include <arpa/inet.h>
#include <math.h>

#ifdef USER
	#define JAZZ_SERVICE_RHOME USER_R_HOME
#else
	#ifdef CATCH_TEST
		#define JAZZ_SERVICE_RHOME USER_R_HOME
	#else
		#define JAZZ_SERVICE_RHOME JAZZ_RELEASE_RHOME
	#endif
#endif

/*	-----------------------------------------------
	  C o n s t r u c t o r / d e s t r u c t o r
--------------------------------------------------- */

jzzBLOCKCONV::jzzBLOCKCONV()
{
};


jzzBLOCKCONV::~jzzBLOCKCONV()
{
};

/*	-----------------------------------------------
	 Methods inherited from	 j a z z S e r v i c e
--------------------------------------------------- */

/** Start of jzzBLOCKCONV

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	see jazzService::start()
*/
bool jzzBLOCKCONV::start()
{
	bool ok = super::start();

	if (!ok) return false;

	jCommons.log(LOG_INFO, "jzzBLOCKCONV started.");

	return true;
}


/** Stop of jzzBLOCKCONV

	\return true and log(LOG_INFO, "further details"). No error checking.

	see jazzService::stop()
*/
bool jzzBLOCKCONV::stop()
{
	jCommons.log(LOG_INFO, "jzzBLOCKCONV stopped.");

	return super::stop();
}


/** Reload of jzzBLOCKCONV

	\return true and log(LOG_INFO, "further details"). No reloading implemented.

	see jazzService::reload()
*/
bool jzzBLOCKCONV::reload()
{
	bool ok = super::reload();

	if (!ok) return false;

	jCommons.log(LOG_INFO, "jzzBLOCKCONV reloaded, but no configuration changes applied.");

	return true;
}

/*	-------------------------------------------------
		R O M	 :	translate_block_*
----------------------------------------------------- */

/** Create a block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS from a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW binary
compatible with a serialized R object.

	\param psrc	 An R object serialized as a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW
	\param pdest Address of a pJazzBlock allocated by this function to store the block.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the psrc object except copying it.
	The returned pdest is owned by the caller and must be JAZZFREE()ed when no longer necessary.
*/
bool jzzBLOCKCONV::translate_block_FROM_R (pJazzBlock psrc, pJazzBlock &pdest)
{
	R_binary * phea = (R_binary *) &reinterpret_cast<pRawBlock>(psrc)->data;

	if (phea->signature != sw_RBINARY_SIGNATURE || phea->format_version != sw_RBINARY_FORMATVERSION)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : Wrong signature || format_version.");

		return false;
	}

	int type = htonl(phea->R_type), R_len = htonl(phea->R_length);

	switch (type)
	{
		case LGLSXP:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_BOOL, R_len);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : JAZZALLOC(RAM_ALLOC_C_BOOL) failed.");

					return false;
				}
				int	 *			pdata_src  = (int *)		   &phea[1];
				unsigned char * pdata_dest = (unsigned char *) &reinterpret_cast<pBoolBlock>(pdest)->data;
				for (int i = 0; i < R_len; i++)
				{
					if (pdata_src[i])
					{
						if (pdata_src[i] == sw_ONE) pdata_dest[i] = 1;
						else						pdata_dest[i] = JAZZC_NA_BOOL;
					}
					else							pdata_dest[i] = 0;
				}
			}
			break;

		case INTSXP:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_INTEGER, R_len);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : JAZZALLOC(RAM_ALLOC_C_INTEGER) failed.");

					return false;
				}
				int * pdata_src	 = (int *) &phea[1];
				int * pdata_dest = (int *) &reinterpret_cast<pIntBlock>(pdest)->data;
				for (int i = 0; i < R_len; i++)
					pdata_dest[i] = htonl(pdata_src[i]);
			}
			break;

		case STRSXP:
			{
				// Allocation: R serializes all strings without final zero and with 8 trailing bytes -> string_buffer + Nx(4 bytes + 2 trailing)
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_OFFS_CHARS, psrc->size - sizeof(R_binary) + sizeof(string_buffer) + 8 - 2*R_len);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : JAZZALLOC(RAM_ALLOC_C_OFFS_CHARS) failed.");

					return false;
				}
				if (!R_len)
				{
					memset(&reinterpret_cast<pCharBlock>(pdest)->data, 0, pdest->size);
					return true;
				}

				if (!format_C_OFFS_CHARS((pCharBlock) pdest, R_len))
				{
					JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : format_C_OFFS_CHARS() failed.");

					return false;
				}
				pRStr_stream src;
				src.pchar = (char *) &phea[1];
				for (int i = 0; i < R_len; i++)
				{
					RStr_header rh = *src.phead++;

					if (rh.signature == sw_CHARSXP_NA && rh.nchar == sw_CHARSXP_NA_LENGTH)
					{
						reinterpret_cast<pCharBlock>(pdest)->data[i] = JAZZC_NA_STRING;
					}
					else
					{
						if ((rh.signature & sw_CHARSXP_MASK_FROM_R) == sw_CHARSXP_HEA_FROM_R)
						{
							int nchar = htonl(rh.nchar);
							if (nchar >= MAX_STRING_LENGTH || nchar < 0)
							{
								JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

								jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : String too long.");

								return false;
							}
							if (nchar == 0)
							{
								reinterpret_cast<pCharBlock>(pdest)->data[i] = JAZZC_EMPTY_STRING;
							}
							else
							{
								reinterpret_cast<pCharBlock>(pdest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) pdest, src.pchar, nchar);

								src.pchar += nchar;
							}
						}
						else
						{
							JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

							jCommons.log_printf(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : Unexpected signature = %4X", rh.signature);

							return false;
						}
					}
				}
			}
			break;

		case REALSXP:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_REAL, R_len);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : JAZZALLOC(RAM_ALLOC_C_REAL) failed.");

					return false;
				}
				int * pdata_src	 = (int *) &phea[1];
				int * pdata_dest = (int *) &reinterpret_cast<pRealBlock>(pdest)->data;
				for (int i = 0; i < R_len; i++)
				{
					pdata_dest[1] = ntohl(*pdata_src++);
					pdata_dest[0] = ntohl(*pdata_src++);

					pdata_dest += 2;
				}
			}
			break;

		default:
		{
			jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_R() : Wrong type.");

			return false;
		}
	}

	return true;
}


/** Create a block_C_R_RAW of type BLOCKTYPE_RAW_R_RAW binary compatible with a serialized R object from a block_C_BOOL, block_C_R_INTEGER,
block_C_R_REAL or block_C_OFFS_CHARS.

	\param psrc	 The source block (A block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS).
	\param pdest Address of a pJazzBlock allocated by this function to store the block.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the psrc object except copying it.
	The returned pdest is owned by the caller and must be JAZZFREE()ed when no longer necessary.
*/
bool jzzBLOCKCONV::translate_block_TO_R (pJazzBlock psrc, pJazzBlock &pdest)
{
	int size = sizeof(R_binary);

	switch (psrc->type)
	{
		case BLOCKTYPE_C_BOOL:
		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			size += psrc->length*sizeof(int);
			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			for (int i = 0; i < psrc->length; i++)
				size += 2*sizeof(int) + strlen(PCHAR(reinterpret_cast<pCharBlock>(psrc), i));
			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			size += psrc->length*sizeof(double);
			break;

		default:
			jCommons.log(LOG_MISS, "translate_block_TO_R(): Unsupported type.");

			return false;
	}

	bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_RAW, size);
	if (!ok)
	{
		jCommons.log(LOG_MISS, "translate_block_TO_R(): JAZZALLOC failed.");

		return false;

	}
	reinterpret_cast_block(pdest, BLOCKTYPE_RAW_R_RAW);
	R_binary * pt = (R_binary *) &reinterpret_cast<pRawBlock>(pdest)->data;

	pt->signature	   = sw_RBINARY_SIGNATURE;
	pt->format_version = sw_RBINARY_FORMATVERSION;
	pt->writer		   = sw_RBINARY_WRITER;
	pt->min_reader	   = sw_RBINARY_MINREADER;
	pt->R_length	   = ntohl(psrc->length);

	switch (psrc->type)
	{
		case BLOCKTYPE_C_BOOL:
			{
				int	 * pdata_dest = (int *) &pt[1];
				unsigned char * pdata_src  = (unsigned char *) &reinterpret_cast<pBoolBlock>(psrc)->data;
				pt->R_type = ntohl(LGLSXP);
				for (int i = 0; i < psrc->length; i++)
				{
					if (pdata_src[i])
					{
						if (pdata_src[i] == 1) pdata_dest[i] = sw_ONE;
						else				   pdata_dest[i] = sw_NA_LOGICAL;
					}
					else					   pdata_dest[i] = 0;
				}
			}
			break;

		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			{
				int * pdata_dest = (int *) &pt[1];
				int * pdata_src	 = (int *) &reinterpret_cast<pIntBlock>(psrc)->data;
				pt->R_type = ntohl(INTSXP);
				for (int i = 0; i < psrc->length; i++)
					pdata_dest[i] = ntohl(pdata_src[i]);
			}
			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			{
				pRStr_stream dest;
				dest.pchar = (char *) &pt[1];
				RStr_header sth;
				pt->R_type = ntohl(STRSXP);
				for (int i = 0; i < psrc->length; i++)
				{
					if (reinterpret_cast<pCharBlock>(psrc)->data[i] != JAZZC_NA_STRING)
					{
						char * pdata_src = PCHAR(reinterpret_cast<pCharBlock>(psrc), i);
						int len = strlen(pdata_src);
						sth.nchar = ntohl(len);
						sth.signature = sw_CHARSXP_HEA_TO_R;
						*dest.phead++ = sth;
						for (int j = 0; j < len; j ++)
							*dest.pchar++ = pdata_src[j];
					}
					else
					{
						sth.nchar	  = sw_CHARSXP_NA_LENGTH;
						sth.signature = sw_CHARSXP_NA;
						*dest.phead++ = sth;
					}
				}
			}
			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			{
				int * pdata_dest = (int *) &pt[1];
				int * pdata_src	 = (int *) &reinterpret_cast<pRealBlock>(psrc)->data;
				pt->R_type = ntohl(REALSXP);
				for (int i = 0; i < psrc->length; i++)
				{
					pdata_dest[1] = ntohl(*pdata_src++);
					pdata_dest[0] = ntohl(*pdata_src++);

					pdata_dest += 2;
				}
			}
	}

	return true;
}


/** Create a block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS from a block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV,
BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or BLOCKTYPE_RAW_MIME_XML.

	\param psrc	 The source block (A block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV, BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or
				 BLOCKTYPE_RAW_MIME_XML).
	\param pdest Address of a pJazzBlock allocated by this function to store the block.
	\param type	 The output type (BLOCKTYPE_C_BOOL, BLOCKTYPE_C_OFFS_CHARS, BLOCKTYPE_C_R_FACTOR, BLOCKTYPE_C_R_GRADE, BLOCKTYPE_C_R_INTEGER,
				 BLOCKTYPE_C_R_TIMESEC or BLOCKTYPE_C_R_REAL).
	\param fmt	 The (sscanf() compatible) format for converting the data. Each row of text must be convertible to one element of the appropriate
				 data type. E.g., " %lf," will convert " 1.23," to a double == 1.23

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the psrc object except copying it.
	The returned pdest is owned by the caller and must be JAZZFREE()ed when no longer necessary.
*/
bool jzzBLOCKCONV::translate_block_FROM_TEXT (pJazzBlock psrc, pJazzBlock &pdest, int type, char * fmt)
{
	bool copy = fmt[0] == 0;

	if (copy && type != BLOCKTYPE_C_OFFS_CHARS)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : fmt == '' is only valid for strings.");

		return false;
	}

	char buff[MAX_STRING_LENGTH + 4];
	char * pchar = (char *) &reinterpret_cast<pRawBlock>(psrc)->data;

	int bytes  = psrc->size;
	int length = 0;
	int nchar;

	while (bytes > 0)
	{
		length++;
		nchar = (uintptr_t) strchrnul(pchar, '\n') - (uintptr_t) pchar + 1;
		nchar = min(nchar, bytes);

		if (nchar >= MAX_STRING_LENGTH)
		{
			jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : String too long.");

			return false;
		}

		pchar += nchar;
		bytes -= nchar;
	}

	pchar = (char *) &reinterpret_cast<pRawBlock>(psrc)->data;
	bytes = psrc->size;

	switch (type)
	{
		case BLOCKTYPE_C_BOOL:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_BOOL, length);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : JAZZALLOC(RAM_ALLOC_C_BOOL) failed.");

					return false;
				}
				unsigned char * pdata_dest = (unsigned char *) &reinterpret_cast<pBoolBlock>(pdest)->data;
				int i = 0;
				while (bytes > 0)
				{
					nchar = (uintptr_t) strchrnul(pchar, '\n') - (uintptr_t) pchar + 1;
					nchar = min(nchar, bytes);

					strncpy(buff, pchar, nchar);
					buff[nchar] = 0;
					if (sscanf(buff, fmt, &pdata_dest[i]) != 1)
						pdata_dest[i] = JAZZC_NA_BOOL;

					pchar += nchar;
					bytes -= nchar;
					i++;
				}
			}
			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			{
				// Allocation: string_buffer + Nx(4 bytes + 2 trailing)
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_OFFS_CHARS, psrc->size + sizeof(string_buffer) + 8 + 6*length);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : JAZZALLOC(RAM_ALLOC_C_OFFS_CHARS) failed.");

					return false;
				}
				if (!length)
				{
					memset(&reinterpret_cast<pCharBlock>(pdest)->data, 0, pdest->size);
					return true;
				}
				if (!format_C_OFFS_CHARS((pCharBlock) pdest, length))
				{
					JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : format_C_OFFS_CHARS() failed.");

					return false;
				}
				char buff2[MAX_STRING_LENGTH + 4];
				int i = 0;
				while (bytes > 0)
				{
					nchar = (uintptr_t) strchrnul(pchar, '\n') - (uintptr_t) pchar + 1;
					nchar = min(nchar, bytes);

					strncpy(buff, pchar, nchar);
					buff[nchar] = 0;
					if (copy)
					{
						int nc = nchar == bytes ? nchar : nchar - 1;
						reinterpret_cast<pCharBlock>(pdest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) pdest, buff, nc);
					}
					else
					{
						if (sscanf(buff, fmt, buff2) == 1)
						{
							reinterpret_cast<pCharBlock>(pdest)->data[i] = get_string_idx_C_OFFS_CHARS((pCharBlock) pdest, buff2, strlen(buff2));
						}
						else
						{
							reinterpret_cast<pCharBlock>(pdest)->data[i] = JAZZC_NA_STRING;
						}
					}

					pchar += nchar;
					bytes -= nchar;
					i++;
				}
			}
			break;

		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_INTEGER, length);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : JAZZALLOC(RAM_ALLOC_C_INTEGER) failed.");

					return false;
				}
				pdest->type = type;
				int * pdata_dest = (int *) &reinterpret_cast<pIntBlock>(pdest)->data;
				int i = 0;
				while (bytes > 0)
				{
					nchar = (uintptr_t) strchrnul(pchar, '\n') - (uintptr_t) pchar + 1;
					nchar = min(nchar, bytes);

					strncpy(buff, pchar, nchar);
					buff[nchar] = 0;
					if (sscanf(buff, fmt, &pdata_dest[i]) != 1)
						pdata_dest[i] = JAZZC_NA_INTEGER;

					pchar += nchar;
					bytes -= nchar;
					i++;
				}
			}
			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			{
				bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_REAL, length);
				if (!ok)
				{
					jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : JAZZALLOC(RAM_ALLOC_C_REAL) failed.");

					return false;
				}
				pdest->type = type;
				double * pdata_dest = (double *) &reinterpret_cast<pRealBlock>(pdest)->data;
				int i = 0;
				while (bytes > 0)
				{
					nchar = (uintptr_t) strchrnul(pchar, '\n') - (uintptr_t) pchar + 1;
					nchar = min(nchar, bytes);

					strncpy(buff, pchar, nchar);
					buff[nchar] = 0;
					if (sscanf(buff, fmt, &pdata_dest[i]) != 1)
						pdata_dest[i] = JAZZC_NA_DOUBLE;

					pchar += nchar;
					bytes -= nchar;
					i++;
				}
			}
			break;

		default:
			jCommons.log(LOG_MISS, "jzzBLOCKCONV::translate_block_FROM_TEXT() : Unexpected block type.");

			return false;
	}

	return true;
}


/** Create a block_C_R_RAW of type BLOCKTYPE_RAW_MIME_CSV, BLOCKTYPE_RAW_MIME_JSON, BLOCKTYPE_RAW_MIME_TSV or BLOCKTYPE_RAW_MIME_XML from a
block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS.

	\param psrc	 The source block (A block_C_BOOL, block_C_R_INTEGER, block_C_R_REAL or block_C_OFFS_CHARS).
	\param pdest Address of a pJazzBlock allocated by this function to store the block.
	\param fmt	 The (sprintf() compatible) format for converting the data, must be compatible with the source type.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the psrc object except copying it.
	The returned pdest is owned by the caller and must be JAZZFREE()ed when no longer necessary.
*/
bool jzzBLOCKCONV::translate_block_TO_TEXT (pJazzBlock psrc, pJazzBlock &pdest, const char * fmt)
{
	char buff_item[MAX_STRING_LENGTH];
	int size = 0;

	switch (psrc->type)
	{
		case BLOCKTYPE_C_BOOL:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pBoolBlock>(psrc)->data[i] == JAZZC_NA_BOOL)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pBoolBlock>(psrc)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pCharBlock>(psrc)->data[i] == JAZZC_NA_STRING)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, PCHAR(reinterpret_cast<pCharBlock>(psrc), i));
					size += strlen(buff_item);
				}
			}
			break;

		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pIntBlock>(psrc)->data[i] == JAZZC_NA_INTEGER)
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pIntBlock>(psrc)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			for (int i = 0; i < psrc->length; i++)
			{
				if (R_IsNA(reinterpret_cast<pRealBlock>(psrc)->data[i]))
				{
					size += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(buff_item, fmt, reinterpret_cast<pRealBlock>(psrc)->data[i]);
					size += strlen(buff_item);
				}
			}
			break;

		default:
			jCommons.log(LOG_MISS, "translate_block_TO_TEXT(): Unexpected block type.");

			return false;
	}

	bool ok = JAZZALLOC(pdest, RAM_ALLOC_C_RAW, size + 8);	// Needed by sprintf() to add the extra 0.
	pdest->size = size;

	if (!ok)
	{
		jCommons.log(LOG_MISS, "translate_block_TO_TEXT(): JAZZALLOC failed.");

		return false;

	}
	reinterpret_cast_block(pdest, BLOCKTYPE_RAW_STRINGS);
	char * pt = (char *) &reinterpret_cast<pRawBlock>(pdest)->data;

	switch (psrc->type)
	{
		case BLOCKTYPE_C_BOOL:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pBoolBlock>(psrc)->data[i] == JAZZC_NA_BOOL)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pBoolBlock>(psrc)->data[i]);
					pt += strlen(pt);
				}
			}
			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pCharBlock>(psrc)->data[i] == JAZZC_NA_STRING)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, PCHAR(reinterpret_cast<pCharBlock>(psrc), i));
					pt += strlen(pt);
				}
			}
			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			for (int i = 0; i < psrc->length; i++)
			{
				if (R_IsNA(reinterpret_cast<pRealBlock>(psrc)->data[i]))
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pRealBlock>(psrc)->data[i]);
					pt += strlen(pt);
				}
			}
			break;

		default:
			for (int i = 0; i < psrc->length; i++)
			{
				if (reinterpret_cast<pIntBlock>(psrc)->data[i] == JAZZC_NA_INTEGER)
				{
					strcpy(pt, NA_AS_TEXT);
					pt += LENGTH_NA_AS_TEXT;
				}
				else
				{
					sprintf(pt, fmt, reinterpret_cast<pIntBlock>(psrc)->data[i]);
					pt += strlen(pt);
				}
			}
	}
	pt[0] = 0;

	return true;
}


/** Implement the API function PUT //source.key.header/type to set the block type to a compatible type.

	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  A key identifying the block in that source.
	\param type	  The new type to which the block will be converted.

	\return		  true if successful, false and log(LOG_MISS, "further details") if not.

This function is a part of the APIs exec_block_put_function() separated to avoid that function becomes unmaintainable.
What it basically does is calling reinterpret_cast_block() and hash_block() and rewriting the block handling all the error/pointer logic.
*/
bool jzzBLOCKCONV::cast_lmdb_block (int source, persistedKey key, int type)
{
	pJazzBlock psrc, pdest;

	if (!block_get(source, key, psrc))
	{
		jCommons.log(LOG_MISS, "jzzAPI::cast_lmdb_block(): block_get() failed.");

		return false;
	}

	if (psrc->type == type)
	{
		block_unprotect(psrc);

		return true;
	}

	if (!new_block_copy(psrc, pdest))
	{
		block_unprotect(psrc);
		jCommons.log(LOG_MISS, "jzzAPI::cast_lmdb_block(): new_block_copy() failed.");

		return false;
	}
	pdest->flags = psrc->flags;

	block_unprotect(psrc);

	hash_block(pdest);

	if (!reinterpret_cast_block(pdest, type))
	{
		JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));
		jCommons.log(LOG_MISS, "jzzAPI::cast_lmdb_block(): reinterpret_cast_block() failed.");

		return false;
	}

	if (!block_put(source, key, pdest))
	{
		JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));
		jCommons.log(LOG_MISS, "jzzAPI::cast_lmdb_block(): block_put() failed.");

		return false;
	}

	JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

	return true;
}


/** Implement the API function PUT //source.key.header/flags to set the block flags and rewrite the block.

	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  A key identifying the block in that source.
	\param flags  The flags to be written.

	\return		  true if successful, false and log(LOG_MISS, "further details") if not.

This function is a part of the APIs exec_block_put_function() separated to avoid that function becomes unmaintainable.
*/
bool jzzBLOCKCONV::set_lmdb_blockflags (int source, persistedKey key, int flags)
{
	pJazzBlock psrc, pdest;

	if (!block_get(source, key, psrc))
	{
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_blockflags(): block_get() failed.");

		return false;
	}

	if (psrc->flags == flags)
	{
		block_unprotect(psrc);

		return true;
	}

	if (!new_block_copy(psrc, pdest))
	{
		block_unprotect(psrc);
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_blockflags(): new_block_copy() failed.");

		return false;
	}

	pdest->hash64 = psrc->hash64;
	pdest->flags  = flags;

	block_unprotect(psrc);

	if (!block_put(source, key, pdest))
	{
		JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_blockflags(): block_put() failed.");

		return false;
	}

	JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

	return true;
}


/** Implement the API function PUT //source.key.from_text/type,source_key,format to create a new data block by translating a text block.

	\param source	  The index of an open source. (As returned by get_source_idx().)
	\param key		  A key identifying the block in that source.
	\param type		  The new type to which the block will be converted.
	\param source_key The source block (of type BLOCKTYPE_RAW_STRINGS) with the data as text to be parsed.
	\param fmt		  The (sscanf() compatible) format for converting the data. Each row of text must be convertible to one element of the
					  appropriate data type. E.g., " %lf," will convert " 1.23," to a double == 1.23

	\return			  true if successful, false and log(LOG_MISS, "further details") if not.

This function is a part of the APIs exec_block_put_function() separated to avoid that function becomes unmaintainable.
*/
bool jzzBLOCKCONV::set_lmdb_fromtext (int source, persistedKey key, int type, persistedKey source_key, char * fmt)
{
	pJazzBlock psrc, pdest;

	if (!block_get(source, source_key, psrc))
	{
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromtext(): block_get() failed.");

		return false;
	}

	if (psrc->type != BLOCKTYPE_RAW_STRINGS)
	{
		block_unprotect(psrc);

		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromtext(): wrong type.");

		return false;
	}

	if (!translate_block_FROM_TEXT(psrc, pdest, type, fmt))
	{
		block_unprotect(psrc);
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromtext(): translate_block_FROM_TEXT() failed.");

		return false;
	}

	block_unprotect(psrc);

	hash_block(pdest);

	if (!block_put(source, key, pdest))
	{
		JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromtext(): block_put() failed.");

		return false;
	}

	JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

	return true;
}


/** Implement the API function PUT //source.key.from_R/source_key to create a new data block by translating an R binary block.

	\param source	  The index of an open source. (As returned by get_source_idx().)
	\param key		  A key identifying the block in that source.
	\param source_key The source block (of type BLOCKTYPE_RAW_R_RAW) with the data as an R binary to be parsed.

	\return			  true if successful, false and log(LOG_MISS, "further details") if not.

This function is a part of the APIs exec_block_put_function() separated to avoid that function becomes unmaintainable.
*/
bool jzzBLOCKCONV::set_lmdb_fromR (int source, persistedKey key, persistedKey source_key)
{
	pJazzBlock psrc, pdest;

	if (!block_get(source, source_key, psrc))
	{
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromR(): block_get() failed.");

		return false;
	}

	if (psrc->type != BLOCKTYPE_RAW_R_RAW)
	{
		block_unprotect(psrc);

		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromR(): wrong type.");

		return false;
	}

	if (!translate_block_FROM_R(psrc, pdest))
	{
		block_unprotect(psrc);
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromR(): translate_block_FROM_R() failed.");

		return false;
	}

	block_unprotect(psrc);

	hash_block(pdest);

	if (!block_put(source, key, pdest))
	{
		JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));
		jCommons.log(LOG_MISS, "jzzAPI::set_lmdb_fromR(): block_put() failed.");

		return false;
	}

	JAZZFREE(pdest, AUTOTYPEBLOCK(pdest));

	return true;
}

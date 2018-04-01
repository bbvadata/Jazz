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


#include <math.h>


#include "src/jazz_elements/jazz_datablocks.h"


namespace jazz_datablocks
{


inline double R_ValueOfNA()
{
	union {double d; int i[2];} na;

	na.i[1] = 0x7ff00000;
	na.i[0] = 1954;

	return na.d;
}

float  F_NA = nanf("");
double R_NA = R_ValueOfNA();


// /** Create a block_C_OFFS_CHARS by repeating a value a number of times.

// 	\param pb	 The address of a pCharBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
// 				 RAM_ALLOC_C_OFFS_CHARS).
// 	\param str	 The string to be copied many times. NULL can be used to indicate NA (also a string of length 0).
// 	\param times The number of times to repeat that value == length of the resulting vector.

// 	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

// 	This is not the usual way to create a block_C_OFFS_CHARS since it only allocates the necessary RAM to store the block. It does exactly as
// announced, but managing strings is not trivial since the amount of RAM that will be necessary can only be guessed. See functions JAZZALLOC(.,
// RAM_ALLOC_C_OFFS_CHARS, .) format_C_OFFS_CHARS(), get_string_idx_C_OFFS_CHARS() and realloc_C_OFFS_CHARS() for proper string management and use
// this only when it does exactly what you need (i.e, you have no intentions to further expand the block).
// */
// bool jzzBLOCKS::new_block_C_CHARS_rep(pCharBlock &pb, const char * str, int times)
// {
// 	int len = str == NULL ? 0 : strlen(str);

// 	// 1 for the end of string + end of buffer, 7 for rounding up to 8 bytes.
// 	unsigned int size = (sizeof(int)*times + sizeof(string_buffer) + len + 1 + 7) & 0xFFFFfff8;

// 	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_OFFS_CHARS, size);

// 	if (!ok)
// 	{
// 		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_CHARS_rep() alloc failed.");

// 		return false;
// 	}

// 	if (!times)
// 	{
// 		memset(&pb->data, 0, pb->size);

// 		return true;
// 	}

// 	format_C_OFFS_CHARS(pb, times);							// Cannot fail when alloc is Ok.
// 	int j = get_string_idx_C_OFFS_CHARS(pb, str, len);		// Cannot fail when alloc is Ok.

// 	for (int i = 0; i < times; i++) pb->data[i] = j;

// 	return true;
// }

// /*	------------------------------------------------------------------------------------------------------
// 	  R e s i z e	s t r i n g	  v e c t o r s	  a n d	  a d d	  s t r i n g s	  t o	v e c t o r s
// ------------------------------------------------------------------------------------------------------- */

// /** Set the length of a block_C_OFFS_CHARS and initialize its string_buffer[] to contain strings.

// 	\param pstr	  A just JAZZALLOC()ed block_C_OFFS_CHARS whose length has not been set yet.
// 	\param length The length of the allocated string vector.

// 	\return		  true if successful, false and log(LOG_MISS, "further details") if not.

// 	The procedure to create a block of strings is:

// 		pCharBlock pt;

// 		JAZZALLOC(pt, RAM_ALLOC_C_OFFS_CHARS, 16384);// Allocate 16K bytes for a 1000 element vector with many repeated values. Just a guess.
// 		format_C_OFFS_CHARS(pt, 1000);				 // Initialize and set the length to 1000, the buffer is ready and the data[] vector full of NA

// 		for(whatever i in 0..999)
// 			int j = get_string_idx_C_OFFS_CHARS(pt, "My whatever string", strlen(..));
// 			if (j >= 0)
// 				pt->data[i] = j;
// 			else
// 				realloc_C_OFFS_CHARS(pt, 4096);		// Add extra 4K each time get_string_idx_C_OFFS_CHARS() fails to allocate a string.
// */
// bool jzzBLOCKS::format_C_OFFS_CHARS(pCharBlock pstr, int length)
// {
// 	if (pstr->size < sizeof(int)*length + sizeof(string_buffer) + 1)	// One zero behind, but no possible allocation. The block is a block of NA.
// 	{
// 		jCommons.log(LOG_MISS, "jzzBLOCKS::format_C_OFFS_CHARS() length > allocation even with no string buffer.");

// 		return false;
// 	}
// 	pstr->length = length;

// 	memset(&pstr->data, JAZZC_NA_STRING, sizeof(int)*length);

// 	pStringBuff string_buffer = STRING_BUFFER(pstr);

// 	string_buffer->NA		 = 0;
// 	string_buffer->EMPTY	 = 0;
// 	string_buffer->isBig	 = false;
// 	string_buffer->last_idx	 = sizeof(string_buffer);
// 	string_buffer->buffer[0] = 0;

// 	return true;
// }


// /** Find an existing string in a block, or allocate a new one and return its offset in the string_buffer[].

// 	\param pstr	  A block_C_OFFS_CHARS containing strings.
// 	\param string The string to find or allocate in the string_buffer[].
// 	\param len	  The length of "string". This in mandatory to simplify operation with systems where it is known (R and std::string)

// 	\return		  The offset to the string if successfully allocated or -1 if allocation failed.

// 	See format_C_OFFS_CHARS() for an explanation on how block_C_OFFS_CHARS are filled.

// 	The block_C_OFFS_CHARS is designed to be fast to read or operate when the blocks are already created. Filling blocks is not efficient because
// all the (possibly many) strings have to be compared with the new string each time. If that becomes a bottleneck, a more efficient structure like
// a map should be used.
// */
// int jzzBLOCKS::get_string_idx_C_OFFS_CHARS(pCharBlock pstr, const char * string, int len)
// {
// 	if (string == NULL)
// 		return JAZZC_NA_STRING;

// 	if (!len)
// 		return JAZZC_EMPTY_STRING;

// 	pStringBuff string_buffer = STRING_BUFFER(pstr);

// 	char * pt;

// 	if (string_buffer->isBig)
// 	{
// 		pt = (char *)string_buffer + (uintptr_t) string_buffer->last_idx;
// 	}
// 	else
// 	{
// 		pt = (char *)string_buffer + (uintptr_t) sizeof(string_buffer);

// 		int t = 0;
// 		while (pt[0])
// 		{
// 			uintptr_t idx = pt - (char *)string_buffer;

// 			if (!strncmp(string, pt, len + 1)) return idx;

// 			int slen = strlen(pt);

// 			pt += slen + 1;

// 			t++;
// 		}
// 		if (t >= STR_SEARCH_BIG_ABOVE) string_buffer->isBig = true;
// 	}

// 	if (pstr->size >= (uintptr_t)pt - (uintptr_t)&pstr->data + len + 2)
// 	{
// 		uintptr_t idx = pt - (char *)string_buffer;

// 		strncpy(pt, string, len);
// 		pt[len] = 0;

// 		pt += len + 1;

// 		pt[0] = 0;

// 		string_buffer->last_idx = idx + len + 1;

// 		return idx;
// 	}

// 	return -1;
// }


// /** Reallocate a block_C_OFFS_CHARS to a bigger object when more string space is required.
// 	\param pstr			The address of an existing	pCharBlock of type block_C_OFFS_CHARS. It will be copied to a bigger one and freed.
// 	\param extra_length The difference between the old allocation size and the new one.

// 	\return				true if successful, false and log(LOG_MISS, "further details") if not.

// 	See format_C_OFFS_CHARS() for an explanation on how block_C_OFFS_CHARS are filled.

// 	The old object is freed and replaced by a new one where its content is copied first.
// */
// bool jzzBLOCKS::realloc_C_OFFS_CHARS(pCharBlock &pstr, int extra_length)
// {
// 	pCharBlock pnew;

// 	bool ok = JAZZALLOC(pnew, RAM_ALLOC_C_OFFS_CHARS, pstr->size + extra_length);

// 	if (!ok)
// 	{
// 		jCommons.log(LOG_MISS, "jzzBLOCKS::realloc_C_OFFS_CHARS() alloc failed.");

// 		return false;
// 	}

// 	memcpy(&pnew->data, &pstr->data, pstr->size);

// 	pnew->length = pstr->length;

// 	JAZZFREE(pstr, RAM_ALLOC_C_OFFS_CHARS);

// 	pstr = pnew;

// 	return true;
// }




} // namespace jazz_datablocks


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_datablocks.ctest"
#endif

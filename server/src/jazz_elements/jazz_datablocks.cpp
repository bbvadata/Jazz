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

/// Returns an R-compatible numeric NA.
inline double R_ValueOfNA()
{
	union {double d; int i[2];} na;

	na.i[1] = 0x7ff00000;
	na.i[0] = 1954;

	return na.d;
}

float  F_NA = nanf("");
double R_NA = R_ValueOfNA();


/** Find an existing string in a block, or allocate a new one and return its offset in the JazzStringBuffer.buffer.

	\param psb	   The address of the pJazzStringBuffer (passed to avoid calling pStringBuffer repeatedly).
	\param pString The string to find or allocate in the JazzStringBuffer.

	\return		   The offset to the (zero terminated) string inside psb->buffer[] or -1 if allocation failed.

	NOTE: This function is private, called by set_attributes() and set_string(). Use these functions instead and read their NOTES.
*/
int JazzBlock::get_string_offset(pJazzStringBuffer psb, const char *pString)
{
	if (psb->alloc_failed)
		return -1;

	if (pString == nullptr)
		return JAZZ_STRING_NA;

	int len = strlen(pString);

	if (!len)
		return JAZZ_STRING_EMPTY;

	char *pt;

	if (psb->stop_check_4_match) {
		pt = &psb->buffer[psb->last_idx];
	} else {
		pt = &psb->buffer[2];

		int t = 0;
		while (pt[0]) {
			uintptr_t idx = pt - &psb->buffer[0];

			if (!strncmp(pString, pt, len + 1))
				return idx;

			int slen = strlen(pt);

			pt += slen + 1;

			t++;
		}
		if (t >= JAZZ_MAX_CHECKS_4_MATCH) psb->stop_check_4_match = true;
	}

	if (psb->buffer_size >= (uintptr_t) pt - (uintptr_t) &psb->buffer[0] + len + 2) {
		uintptr_t idx = pt - &psb->buffer[0];

		strncpy(pt, pString, len);
		pt[len] = 0;

		pt += len + 1;

		pt[0] = 0;

		psb->last_idx = idx + len + 1;

		return idx;
	}

	return -1;
}


} // namespace jazz_datablocks


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_datablocks.ctest"
#endif

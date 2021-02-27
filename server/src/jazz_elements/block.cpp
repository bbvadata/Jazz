/* Jazz (c) 2018-2021 kaalam.ai (The Authors of Jazz), using (under the same license):

	1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

	2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

      Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

      This product includes software developed at

      BBVA (https://www.bbva.com/)

	3. LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

	  Licensed under http://www.OpenLDAP.org/license.html


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


// #include <stl_whatever>


#include "src/jazz_elements/block.h"


namespace jazz_elements
{

/** Scan a tensor object to see if it contains any NA valued of the type specified in cell_type.

	\return		True if NA values of the give type were found.

	Note: For boolean types everything other than true (1) or false (0) is considered NA
	Note: For floating point types, only binary identity with F_NA or R_NA counts as NA

*/
bool Block::find_NAs_in_tensor(){
	switch (cell_type) {
	case CELL_TYPE_BYTE_BOOLEAN: {
		for (int i = 0; i < size; i++) {
			if ((tensor.cell_byte[i] & 0xfe) != 0)
				return true;
		}
		return false; }

	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE: {
		for (int i = 0; i < size; i++) {
			if (tensor.cell_int[i] == INTEGER_NA)
				return true;
		}
		return false; }

	case CELL_TYPE_BOOLEAN: {
		for (int i = 0; i < size; i++) {
			if ((tensor.cell_uint[i] & 0xfffffffe) != 0)
				return true;
		}
		return false; }

	case CELL_TYPE_SINGLE: {
		u_int una = reinterpret_cast<u_int*>(&SINGLE_NA)[0];

		for (int i = 0; i < size; i++) {
			if (tensor.cell_uint[i] == una)
				return true;
		}
		return false; }

	case CELL_TYPE_STRING: {
		for (int i = 0; i < size; i++) {
			if (tensor.cell_int[i] == STRING_NA)
				return true;
		}
		return false; }

	case CELL_TYPE_LONG_INTEGER: {
		for (int i = 0; i < size; i++) {
			if (tensor.cell_longint[i] == LONG_INTEGER_NA)
				return true;
		}
		return false; }

	case CELL_TYPE_TIME: {
		for (int i = 0; i < size; i++) {
			if (tensor.cell_longint[i] == TIME_POINT_NA)
				return true;
		}
		return false; }

	case CELL_TYPE_DOUBLE: {
		uint64_t una = reinterpret_cast<uint64_t*>(&DOUBLE_NA)[0];

		for (int i = 0; i < size; i++) {
			if (tensor.cell_ulongint[i] == una)
				return true;
		}
		return false; }

	default:
		return false;
	}
}


/** Find an existing string in a block, or allocate a new one and return its offset in the StringBuffer.buffer.

	\param psb	   The address of the pStringBuffer (passed to avoid calling p_string_buffer repeatedly).
	\param pString The string to find or allocate in the StringBuffer.

	\return		   The offset to the (zero terminated) string inside psb->buffer[] or -1 if allocation failed.

	NOTE: This function is private, called by set_attributes() and set_string(). Use these functions instead and read their NOTES.
*/
int Block::get_string_offset(pStringBuffer psb, const char *pString)
{
	if (psb->alloc_failed)
		return STRING_NA;

	if (pString == nullptr)
		return STRING_NA;

	int len = strlen(pString);

	if (!len)
		return STRING_EMPTY;

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
		if (t >= MAX_CHECKS_4_MATCH) psb->stop_check_4_match = true;
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

	psb->alloc_failed = true;

	return STRING_NA;
}


/** Check (in depth) the validity of a filter and return its type or FILTER_TYPE_NOTAFILTER if invalid

	This checks both the values in the header and the validity of the data in .tensor[]

	\return FILTER_TYPE_BOOLEAN or FILTER_TYPE_INTEGER if it is a valid filter of that type, FILTER_TYPE_NOTAFILTER if not.
*/
int Block::filter_audit()
{
	switch (filter_type()) {

	case FILTER_TYPE_INTEGER: {
		if (range.filter.length == 0 || range.filter.length == size)
			return FILTER_TYPE_INTEGER;

		if (range.filter.length < 0 || range.filter.length > size)
			return FILTER_TYPE_NOTAFILTER;

		int lo = -1;

		for (int i = 0; i < range.filter.length; i++) {
			if (tensor.cell_int[i] <= lo || tensor.cell_int[i] >= size)
				return FILTER_TYPE_NOTAFILTER;
			lo = tensor.cell_int[i];
		}
		return FILTER_TYPE_INTEGER; }

	case FILTER_TYPE_BOOLEAN: {
		for (int i = 0; i < size; i++) {
			if ((tensor.cell_byte[i] & 0xfe) != 0)
				return FILTER_TYPE_NOTAFILTER;
		}
		return FILTER_TYPE_BOOLEAN; }
	}

	return FILTER_TYPE_NOTAFILTER;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_block.ctest"
#endif

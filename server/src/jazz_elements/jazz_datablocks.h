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


#include <chrono>
#include <limits.h>
#include <map>
#include <string.h>
#include <iostream>


/**< \brief Basic Jazz codeless structures, constants and the class JazzBlock.

	This module defines the logic to get/set data from/to JazzBlock objects at the simplest level.
*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_DATABLOCKS
#define INCLUDED_JAZZ_ELEMENTS_DATABLOCKS

namespace jazz_datablocks
{

#define JAZZ_MAX_TENSOR_RANK		6		///< Maximum rank = 6, E.g. a 2D array of raw videos (row, column, frame, x, y, color)
#define JAZZ_MAX_CHECKS_4_MATCH	   25		///< Maximum number of tries to match in get_string_offset() before setting stop_check_4_match

#define CELL_TYPE__

// 8 bit cell types
#define CELL_TYPE_BYTE			0x001		///< A tensor of unsigned 8-bit binaries. NA is not defined for this type
#define CELL_TYPE_BYTE_BOOLEAN	0x101		///< A tensor 8-bit booleans: 0, 1, JAZZ_BYTE_BOOLEAN_NA = NA

// 32 bit cell types
#define CELL_TYPE_INTEGER		0x004		///< A tensor of 32-bit signed integers. NA is JAZZ_INTEGER_NA
#define CELL_TYPE_FACTOR		0x104		///< A tensor of 32-bit unsorted categoricals. NA is JAZZ_INTEGER_NA
#define CELL_TYPE_GRADE			0x204		///< A tensor of 32-bit sorted categoricals. NA is JAZZ_INTEGER_NA
#define CELL_TYPE_BOOLEAN		0x304		///< A tensor of 32-bit booleans: 0, 1, JAZZ_BOOLEAN_NA = NA
#define CELL_TYPE_SINGLE		0x404		///< A tensor of IEEE 754 32-bit float (aka single). NA is JAZZ_SINGLE_NA
#define CELL_TYPE_JAZZ_STRING	0x504		///< A tensor or 32-bit offsets to unmutable strings or JAZZ_STRING_NA or JAZZ_STRING_EMPTY

// 64 bit cell types
#define CELL_TYPE_LONG_INTEGER	0x008		///< A tensor of 64-bit signed integers. NA is JAZZ_LONG_INTEGER_NA
#define CELL_TYPE_JAZZ_TIME		0x108		///< A tensor of 64-bit TimePoint. NA is JAZZ_TIME_POINT_NA
#define CELL_TYPE_DOUBLE		0x208		///< A vector of floating point numbers. Binary compatible with an R REALSXP (vector of numeric)

// NA values or empty string valuies for all cell_type values
#define JAZZ_BYTE_BOOLEAN_NA	0x0ff		///< NA for 8-bit boolean is binary 0xff. Type does not exist in R.
#define JAZZ_BOOLEAN_NA			0x0ff		///< NA for a 32-bit boolean is binary 0xff. This is R compatible.
#define JAZZ_INTEGER_NA			INT_MIN		///< NA for a 32-bit integer. This is R compatible.
#define JAZZ_SINGLE_NA			F_NA		///< NA for a float is the value returned by nanf(). Type does not exist in R.
#define JAZZ_STRING_NA			0			///< NA for a string coded as CELL_TYPE_JAZZ_STRING
#define JAZZ_STRING_EMPTY		1			///< An empty string coded as CELL_TYPE_JAZZ_STRING
#define JAZZ_LONG_INTEGER_NA	LLONG_MIN	///< NA for a 64-bit integer. Type does not exist in R.
#define JAZZ_TIME_POINT_NA		0			///< NA for a CELL_TYPE_JAZZ_TIME is a 64-bit zero. Type does not exist in R.
#define JAZZ_DOUBLE_NA			R_NA		///< NA for a double. This is R compatible.

typedef std::chrono::steady_clock::time_point TimePoint;	///< A time point stored as 8 bytes

/// Dimensions for the Tensor. The product of all * (cell_type & 0xff) < 2Gb
typedef int JazzTensorDim[JAZZ_MAX_TENSOR_RANK];

/// Header for a JazzBlock
struct JazzBlockHeader
{
	int	cell_type;				///< The type for the cells in the tensor. See CELL_TYPE_*
	int	rank;					///< The number of dimensions
	JazzTensorDim dim_offs;		///< The dimensions of the tensor in terms of offsets (Max. size is 2 Gb.)
	int size;					///< The total number of cells in the tensor
	int num_attributes;			///< Number of elements in the JazzAttributesMap
	int total_bytes;			///< Total size of the block everything included
	bool has_NA;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
	TimePoint created;			///< Timestamp when the block was created
	uint64_t hash64;			///< Hash of everything but the header

	int tensor[];				///< A tensor for type cell_type and dimensions set by JazzBlock.set_dimensions()
};

/// Structure at the end of a JazzBlock, initially created with init_string_buffer()
struct JazzStringBuffer
{
	bool stop_check_4_match;	///< When the JazzStringBuffer is small, try to match existing indices of the same string to save RAM
	bool alloc_failed;			///< A previous call to get_string_offset() failed to alloc space for a string
	int	 last_idx;				///< The index to the first free space after the last stored string
	int	 buffer_size;			///< The size in bytes of buffer[]
	char buffer[];				///< The buffer where strings are stored starting with two zeroes for JAZZ_STRING_NA & JAZZ_STRING_EMPTY
};

typedef std::map<int, const char *> AllAttributes;

typedef JazzBlockHeader	 *pJazzBlockHeader;
typedef JazzStringBuffer *pJazzStringBuffer;


/** A block. Anything in Jazz is a block. A block is a JazzBlockHeader, followed by a tensor, then two arrays of int
of length == num_attributes, then a JazzStringBuffer. Nothing in a JazzBlock is a pointer, JazzBlocks can be copied
or stored 'as is', every RAM location in a block is defined by its JazzBlockHeader and computed by the methods in JazzBlock.

At this level, you only have the fields JazzBlockHeader that you may read and probably only write through some methods.
This is the lowest level, it does not even provide support for allocation, at this level you have support for maniputating
the JazzStringBuffer to read and write strings and the JazzAttributesMap to read and write attributes.
*/
class JazzBlock: public JazzBlockHeader {

	public:

	// Methods on indices.

		/** Sets the tensor dimensions from a JazzTensorDim array.

			\param pDim A pointer to the JazzTensorDim containing the dimensions.

			NOTES: 1. This writes: rank, dim_offs[] and size.
				   2. A dimension 0 is the same as 1, only dimensions >1 count for the rank.
				   3. All >1 dimensions must be in the beginning. 3,2,1,4 == 3,2,0,0 has rank == 2
				   4. All dimensions == 0 (or 1) has rank == 1 and size == 0 or 1 depending on the first dimension being 0 or 1.
		*/
		inline void set_dimensions(int *pDim) {
			rank = JAZZ_MAX_TENSOR_RANK;
			int j = 1;
			for (int i = JAZZ_MAX_TENSOR_RANK -1; i > 0; i--)
				if (pDim[i] > 1) { dim_offs[i] = j; j *= pDim[i]; } else { j = 1; dim_offs[i] = 0; rank = i; }
			if (pDim[0] > 1) { dim_offs[0] = j; j *= pDim[0]; } else { j = pDim[0]; dim_offs[0] = 1; rank = 1; }
			size = j;
		}

		/** Returns the tensor dimensions as a JazzTensorDim array.

			\param pDim A pointer to the JazzTensorDim containing the dimensions.

			NOTES: See notes on set_dimensions() to understand why in case of 0 and 1, it may return different values than those
			passed when the block was created with a set_dimensions() call.
		*/
		inline void get_dimensions(int *pDim) {
			int j = size;
			for (int i = 0; i < JAZZ_MAX_TENSOR_RANK; i++)
				if (i < rank) { pDim[i] = j/dim_offs[i]; j = dim_offs[i]; } else pDim[i] = 0;
		}

		/** Returns if an index (as a JazzTensorDim array) is valid for the tensor.

			\param pIndex A pointer to the JazzTensorDim containing the index.

			\return	True if the index is valid.
		*/
		inline bool validate_index(int *pIndex) {
			int j = size;
			for (int i = 0; i < rank; i++) {
				if (pIndex[i] < 0 || pIndex[i]*dim_offs[i] >= j) return false;
				j = dim_offs[i];
			}
			return true;
		}

		/** Returns if an offset (as an integer) is valid for the tensor.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return	True if the offset is valid.
		*/
		inline int validate_offset(int offset) { return offset >=0 & offset < size; }

		/** Convert an index (as a JazzTensorDim array) to the corresponding offset without checking its validity.

			\param pIndex A pointer to the JazzTensorDim containing the index.

			\return	The offset corresponding to the same cell if the index was in a valid range.
		*/
		inline int get_offset(int *pIndex) {
			int j = 0;
			for (int i = 0; i < rank; i++) j += pIndex[i]*dim_offs[i];
			return j;
		}

		/** Convert an offset to a tensor cell into its corresponding index (as a JazzTensorDim array) without checking its validity.

			\param offset the input offset
			\param pIndex A pointer to the JazzTensorDim to return the result.
		*/
		inline void get_index(int offset, int *pIndex) {
			for (int i = 0; i < rank; i++) { pIndex[i] = offset/dim_offs[i]; offset -= pIndex[i]*dim_offs[i]; }
		}

	// Methods on strings.

		/** Get a string from the tensor by index without checking index range.

			\param pIndex A pointer to the JazzTensorDim containing the index.

			\return A pointer to where the (zero ended) string is stored in the JazzBlock.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int *pIndex) {
			return reinterpret_cast<char *>(&pStringBuffer()->buffer[tensor[get_offset(pIndex)]]);
		}

		/** Get a string from the tensor by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return A pointer to where the (zero ended) string is stored in the JazzBlock.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int offset)	 {
			return reinterpret_cast<char *>(&pStringBuffer()->buffer[tensor[offset]]);
		}

		/** Set a string in the tensor, if there is enough allocation space to contain it, by index without checking index range.

			\param pIndex A pointer to the JazzTensorDim containing the index.
			\param pString A pointer to a (zero ended) string that will be allocated inside the JazzBlock.

			NOTE: Allocation inside a JazzBlock is typically hard since they are created with "just enough space", a JazzBlock is
			typically unmutable. jazz_alloc.h contains methods that make a JazzBlock bigger if that is necessary. This one doesn't.
			The 100% safe way is creating a new block from the unmutable one using jazz_alloc.h methods. Otherwise, use at your own
			risk or not at all. When this fails, it sets the variable alloc_failed in the JazzStringBuffer. When alloc_failed is
			true, it doesn't even try to allocate.
		*/
		inline void set_string(int *pIndex, const char *pString) {
			pJazzStringBuffer psb = pStringBuffer();
			tensor[get_offset(pIndex)] = get_string_offset(psb, pString);
		}

		/** Set a string in the tensor, if there is enough allocation space to contain it, by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.
			\param pString A pointer to a (zero ended) string that will be allocated inside the JazzBlock.

			NOTE: Allocation inside a JazzBlock is typically hard since they are created with "just enough space", a JazzBlock is
			typically unmutable. jazz_alloc.h contains methods that make a JazzBlock bigger if that is necessary. This one doesn't.
			The 100% safe way is creating a new block from the unmutable one using jazz_alloc.h methods. Otherwise, use at your own
			risk or not at all. When this fails, it sets the variable alloc_failed in the JazzStringBuffer. When alloc_failed is
			true, it doesn't even try to allocate.
		*/
		inline void set_string(int offset, const char *pString) {
			pJazzStringBuffer psb = pStringBuffer();
			tensor[offset] = get_string_offset(psb, pString);
		}

	// Methods on attributes.

		/** Find an attribute by its attribute_id (key).

			\param attribute_id A unique 32-bit id defining what the attribute is: e.g., a Jazz class, a url, a mime type, ...

			\return A pointer to where the (zero ended) string (the attribute value) is stored in the JazzBlock. Or nullptr if
					the key is not found.

			NOTE: Search is linear since no assumptions can be made on how keys are ordered. Typically JazzBlocks have few (less
			than 5) attributes and that is okay. There is no limit on the number of attributes, so if you want to use the block's
			attributes as a dictionary containing, e.g., thousands of config keys, use get_attributes() instead which returns a
			map with all the attributes.
		*/
		inline char *find_attribute(int attribute_id) {
			int *ptk = pAttribute_keys();
			for (int i = 0; i < num_attributes; i++)
				if (ptk[i] == attribute_id)
					return reinterpret_cast<char *>(&pStringBuffer()->buffer[ptk[i + num_attributes]]);
			return nullptr;
		}

		/** Set all attributes of a JazzBlock, only when creating it, using a map.

			\param all_att A map containing all the attributes for the block.

			NOTE: This function is public because it has to be called by jazz_alloc.h methods. set_attributes() can
			only be called once, so it will do nothing if called after a JazzBlock is built. JazzBlocks are near-unmutable
			objects, if you need to change a JazzBlock's attributes create a new object using jazz_alloc.h methods.
		*/
		inline void set_attributes(AllAttributes &all_att) {
			if (num_attributes) return;

			num_attributes = all_att.size();
			init_string_buffer();

			int i = 0;
			int *ptk = pAttribute_keys();
			pJazzStringBuffer psb = pStringBuffer();
			for(AllAttributes::iterator it = all_att.begin(); it != all_att.end(); ++it)
			{
				if (i < num_attributes) {
					ptk[i] = it->first;
					ptk[i + num_attributes] = get_string_offset(psb, it->second);
				}
				i++;
			}
		}

		/** Get all attributes of a JazzBlock storing them inside a map.

			\param all_att A (typically empty) map where all the attributes will be stored.

			NOTE: You can use a non-empty map. This will keep existing key/values not found in the JazzBlock and create/override
			those in the JazzBlock by using a normal 'map[key] = value' instruction.
		*/
		inline void get_attributes(AllAttributes &all_att) {
			int *ptk = pAttribute_keys();
			pJazzStringBuffer psb = pStringBuffer();
			for (int i = 0; i < num_attributes; i++)
				all_att[ptk[i]] = reinterpret_cast<char *>(&psb->buffer[ptk[i + num_attributes]]);
		}

		/** Initialize the JazzStringBuffer of a JazzBlock, only when creating it.

			NOTE: This function is public because it has to be called by jazz_alloc.h methods. Never call it to construct
			you own JazzBlocks except for test cases or in jazz_alloc.h methods. Use those to build JazzBlocks instead.
		*/
		inline void init_string_buffer() {
			pJazzStringBuffer psb = pStringBuffer();

			int buff_size = total_bytes - ((uintptr_t) psb - (uintptr_t) &cell_type) - sizeof(JazzStringBuffer);
			if (buff_size < 4) {
				psb->alloc_failed = true;
				return;
			}
			memset(psb, 0, sizeof(JazzStringBuffer) + 4);	// This also clears psb->buffer[0..3]
			// psb->buffer[0] = 0;	// JAZZ_STRING_NA
			// psb->buffer[1] = 0;	// JAZZ_STRING_EMPTY
			// psb->buffer[2] = 0;	// The end of string for searching (will change once a string is inserted)
			psb->buffer_size = buff_size;
			psb->last_idx	 = 2;	// Where the first string will be inserted
		}

#ifndef CATCH_TEST
	private:
#endif

		/** Align a pointer (as uintptr_t) to the next 16 byte boundary.
		*/
		inline int *align_128bit(uintptr_t ipt) {
			return reinterpret_cast<int *>((ipt + 0xf) & 0xffffFFFFffffFFF0);
		}

		/** Return the address of the vector containing both the attribute keys and the attribute ids in the JazzStringBuffer.

			NOTE: The actual values (which are strings) are stored in the same JazzStringBuffer containing the strings of the tensor
			(if any). This array has double the num_attributes size and stores the keys in the lower part and the offsets to the
			values on the upper part.
		*/
		inline int *pAttribute_keys() {
			return align_128bit((uintptr_t) &tensor[0] + (cell_type & 0xf)*size);
		}

		/** Return the address of the JazzStringBuffer containing the strings in the tensor and the attribute values.
		*/
		inline pJazzStringBuffer pStringBuffer() {
			return reinterpret_cast<pJazzStringBuffer>((uintptr_t) pAttribute_keys() + 2*num_attributes*sizeof(int));
		}

		int get_string_offset(pJazzStringBuffer psb, const char *pString);
};

typedef JazzBlock  *pJazzBlock;


/** A filter. First: a filter is just a JazzBlock with a strict structure and extra methods
*/
class JazzFilter: public JazzBlock {
	inline int filter_type();
	inline int filter_audit();
	inline int can_filter(pJazzBlock p_block);
};

typedef JazzFilter *pJazzFilter;


extern float  F_NA;		///< NaN in single
extern double R_NA;		///< NaN in double

} // namespace jazz_datablocks

#endif

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


/**< \brief Basic Jazz codeless data types and constants.

	This module defines the constants and codeless structures to create JazzDataBlock structures.
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

#define JAZZ_MAX_TENSOR_RANK 	6			///< Maximum rank = 6, E.g. a 2D array of raw videos (row, column, frame, x, y, color)

#define CELL_TYPE__

// 8 bit cell types
#define CELL_TYPE_BYTE			0x001		///< A tensor of unsigned 8-bit binaries. NA is not defined for this type.
#define CELL_TYPE_BYTE_BOOLEAN	0x101		///< A tensor 8-bit booleans: 0, 1, JAZZ_BYTE_BOOLEAN_NA = NA.

// 32 bit cell types
#define CELL_TYPE_INTEGER		0x004		///< A tensor of 32-bit signed integers. NA is JAZZ_INTEGER_NA.
#define CELL_TYPE_FACTOR		0x104		///< A tensor of 32-bit unsorted categoricals. NA is JAZZ_INTEGER_NA.
#define CELL_TYPE_GRADE			0x204		///< A tensor of 32-bit sorted categoricals. NA is JAZZ_INTEGER_NA.
#define CELL_TYPE_BOOLEAN		0x304		///< A tensor of 32-bit booleans: 0, 1, JAZZ_BOOLEAN_NA = NA.
#define CELL_TYPE_SINGLE		0x404		///< A tensor of IEEE 754 32-bit float (aka single). NA is JAZZ_SINGLE_NA.
#define CELL_TYPE_JAZZ_STRING	0x504		///< A tensor or 32-bit offsets to unmutable strings or JAZZ_STRING_NA or JAZZ_STRING_EMPTY.

// 64 bit cell types
#define CELL_TYPE_LONG_INTEGER	0x008		///< A tensor of 64-bit signed integers. NA is JAZZ_LONG_INTEGER_NA.
#define CELL_TYPE_JAZZ_TIME		0x108		///< A tensor of 64-bit TimePoint. NA is JAZZ_TIME_POINT_NA.
#define CELL_TYPE_DOUBLE		0x208		///< A vector of floating point numbers. Binary compatible with an R REALSXP (vector of numeric).

// NA values or empty string valuies for all cell_type values.
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

struct JazzBlockHeader
{
	int	cell_type;				///< The type for the cells in the tensor. See CELL_TYPE_*
	int	rank;					///< The number of dimensions
	JazzTensorDim dim_offs;		///< The dimensions of the tensor in terms of offsets (Max. size is 2 Gb.)
	int size;					///< The total number of cells in the tensor
	int num_attributes;			///< Number of elements in the JazzAttributesMap
	int total_bytes;			///< Total size of the block everything included
	bool has_NA;				///< Any value in the tensor is a NA, block requires NA-aware arithmetic.
	TimePoint created;			///< Timestamp when the block was created.
	long long hash64;			///< Hash of everything but the header.

	int tensor[];				///< A tensor for type cell_type and dimensions set by JazzBlock.set_dimensions()
};

struct JazzStringBuffer
{
	char NA, EMPTY;				///< A binary zero making a string with offset 0 (== JAZZ_STRING_NA) or 1 (== JAZZ_STRING_EMPTY) == ""
	bool search_for_matches;	///< When the JazzStringBuffer is small try to match existing indices of the same string to save RAM.
	int	 last_idx;				///< The index to the first free space after the last stored string
	char buffer[];				///< The buffer where all the non-empty strings are stored
};

typedef std::map<int, char *> AllAttributes;

typedef JazzBlockHeader  *pJazzBlockHeader;
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
		inline char *get_string(int *pIndex) { return reinterpret_cast<char *>(&pStringBuffer()->buffer[get_offset(pIndex)]); }

		/** Get a string from the tensor by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return A pointer to where the (zero ended) string is stored in the JazzBlock.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int offset)  { return reinterpret_cast<char *>(&pStringBuffer()->buffer[offset]); }

		inline void set_string(int *pIndex, char *pString) {
			pJazzStringBuffer psb = pStringBuffer();
			psb->buffer[get_offset(pIndex)] = get_string_offset(psb, pString);
		}

		inline void set_string(int offset, char *pString) {
			pJazzStringBuffer psb = pStringBuffer();
			psb->buffer[offset] = get_string_offset(psb, pString);
		}

	// Methods on attributes.

		inline char *find_attribute(int attribute_id) {
			int * ptk = pAttribute_keys();
			for (int i = 0; i < num_attributes; i++)
				if (ptk[i] == attribute_id)
					return reinterpret_cast<char *>(&pStringBuffer()->buffer[ptk[i + num_attributes]]);
			return nullptr;
		}

		inline void set_attributes(AllAttributes &all_att) {
			num_attributes = all_att.size();
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

		inline void get_attributes(AllAttributes &all_att) {
			int *ptk = pAttribute_keys();
			pJazzStringBuffer psb = pStringBuffer();
			for (int i = 0; i < num_attributes; i++)
				all_att[ptk[i]] = reinterpret_cast<char *>(&psb->buffer[ptk[i + num_attributes]]);
		}

	private:

		inline int *align_128bit(uintptr_t ipt) {
			return reinterpret_cast<int *>((ipt + 0xf) & 0xffffFFFFffffFFF0);
		}
		inline int *pAttribute_keys() {
			return align_128bit((uintptr_t) &tensor[0] + (cell_type & 0xf)*size);
		}
		inline pJazzStringBuffer pStringBuffer() {
			return reinterpret_cast<pJazzStringBuffer>((uintptr_t) pAttribute_keys() + 2*num_attributes*sizeof(int));
		}

		int get_string_offset(pJazzStringBuffer psb, char *pString);
};

typedef JazzBlock *pJazzBlock;

extern float  F_NA;
extern double R_NA;

}

#endif

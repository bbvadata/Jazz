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
	JazzTensorDim dim_offs;		///< The dimensions of the tensor in terms of offsets (Max. size is 2 Gb).
	int jazz_class;				///< The class to which the block belongs. See jazz_classes.h for details.
	int offset_stringbuff;		///< Offset from tensor[] to where the JazzStringBuffer starts. (When cell_type == CELL_TYPE_JAZZ_STRING)
	int offset_map;				///< Offset from tensor[] to where the JazzAttributesMap starts.
	int total_bytes;			///< Total size of the block everything included.
	TimePoint created;			///< Timestamp when the block was created.
	long long hash64;			///< Hash of everything but the header.
	int tensor[];
};

struct JazzStringBuffer
{
	char NA, EMPTY;				///< A binary zero making a string with offset 0 (== JAZZ_STRING_NA) or 1 (== JAZZ_STRING_EMPTY) == ""
	bool search_for_matches;	///< When the JazzStringBuffer is small try to match existing indices of the same string to save RAM.
	int	 last_idx;				///< The index to the first free space after the last stored string
	char buffer[];				///< The buffer where all the non-empty strings are stored
};

struct JazzAttributesMap
{
	int map_num_elements;		///< Number of elements in the JazzAttributesMap.
	int attribute_id[];
};

typedef std::map<int, std::string> AllAttributes;

typedef JazzTensorDim     *pJazzTensorDim;
typedef JazzBlockHeader   *pJazzBlockHeader;
typedef JazzStringBuffer  *pJazzStringBuffer;
typedef JazzAttributesMap *pJazzAttributesMap;
typedef AllAttributes     *pAllAttributes;


/** A block. Anything in Jazz is a block. A block is a JazzBlockHeader, followed by a tensor, then, in case
cell_type == CELL_TYPE_JAZZ_STRING followed by a JazzStringBuffer, and followed by a JazzAttributesMap.

At this level, you only have the fields JazzBlockHeader that you may read and probably only write through some methods.
This is the lowest level, it does not even provide support for allocation, at this level you have support for maniputating
the JazzStringBuffer to read and write strings and the JazzAttributesMap to read and write attributes.
*/
class JazzBlock: public JazzBlockHeader {

	public:

	// Methods about the block.

		/** The size of the cells in the tensor (is the lowest byte of .cell_type).
			\return		  The size in bytes (1, 4 or 8).
		*/
		inline int cell_size() { return cell_type & 0xff; }

		inline pJazzStringBuffer  pStringBuffer () { return reinterpret_cast<pJazzStringBuffer>(&tensor[0]  + (uintptr_t) offset_stringbuff); }
		inline pJazzAttributesMap pAttributesMap() { return reinterpret_cast<pJazzAttributesMap>(&tensor[0] + (uintptr_t) offset_map); }

	// Methods on indices.

		inline void set_dimensions(pJazzTensorDim pDim);
		inline void get_dimensions(pJazzTensorDim pDim);
		inline int  get_offset(pJazzTensorDim pIndex);
		inline void get_index(int offset, pJazzTensorDim pIndex);

	// Methods on strings.

		inline char *get_string(pJazzTensorDim pIndex);
		inline char *get_string(int offset);
		inline int   set_string(pJazzTensorDim pIndex, char *pString);
		inline int   set_string(int offset, char *pString);

	// Methods on attributes.

		inline char *find_attribute(int attribute_id);
		inline char *get_attribute (int idx);

		int set_attributes(pAllAttributes pAttr);
		pAllAttributes get_attributes();

	private:

		inline int  get_string_id(pJazzStringBuffer pBuff, char *pString);
		inline int *get_attrib_string_vector();
		inline pJazzStringBuffer *get_attrib_string_buffer();

};

typedef JazzBlock *pJazzBlock;

extern float  F_NA;
extern double R_NA;

}

#endif

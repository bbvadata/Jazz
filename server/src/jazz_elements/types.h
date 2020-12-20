/* Jazz (c) 2018-2020 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include <chrono>
#include <stdint.h>


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_TYPES
#define INCLUDED_JAZZ_ELEMENTS_TYPES


/**< \brief Basic Jazz code-less structures and constants.

	This module defines constant and structures used by Block. A Block is a class derived from a structure BlockHeader.
*/

namespace jazz_elements
{

#define MAX_TENSOR_RANK			6			///< Maximum rank = 6, E.g. a 2D array of raw videos (row, column, frame, x, y, color)
#define MAX_CHECKS_4_MATCH	   25			///< Maximum number of tries to match in get_string_offset() before setting stop_check_4_match

/// Different values for Block.cell_type
#define CELL_TYPE__

// 8 bit cell types
#define CELL_TYPE_BYTE			0x001		///< A tensor of unsigned 8-bit binaries. NA is not defined for this type
#define CELL_TYPE_BYTE_BOOLEAN	0x101		///< A tensor 8-bit booleans: 0, 1, BYTE_BOOLEAN_NA = NA

// 32 bit cell types
#define CELL_TYPE_INTEGER		0x004		///< A tensor of 32-bit signed integers. NA is INTEGER_NA
#define CELL_TYPE_FACTOR		0x104		///< A tensor of 32-bit unsorted categorical. NA is INTEGER_NA
#define CELL_TYPE_GRADE			0x204		///< A tensor of 32-bit sorted categorical. NA is INTEGER_NA
#define CELL_TYPE_BOOLEAN		0x304		///< A tensor of 32-bit booleans: 0, 1, BOOLEAN_NA = NA
#define CELL_TYPE_SINGLE		0x404		///< A tensor of IEEE 754 32-bit float (aka single). NA is SINGLE_NA
#define CELL_TYPE_STRING		0x504		///< A tensor or 32-bit offsets to immutable strings or STRING_NA or STRING_EMPTY

// 64 bit cell types
#define CELL_TYPE_LONG_INTEGER	0x008		///< A tensor of 64-bit signed integers. NA is LONG_INTEGER_NA
#define CELL_TYPE_TIME			0x108		///< A tensor of 64-bit TimePoint. NA is TIME_POINT_NA
#define CELL_TYPE_DOUBLE		0x208		///< A vector of floating point numbers. Binary compatible with an R REALSXP (vector of numeric)

// NA values or empty string values for all cell_type values
#define BYTE_BOOLEAN_NA			0x0ff		///< NA for 8-bit boolean is binary 0xff. Type does not exist in R.
#define BOOLEAN_NA				0x0ff		///< NA for a 32-bit boolean is binary 0xff. This is R compatible.
#define INTEGER_NA				INT_MIN		///< NA for a 32-bit integer. This is R compatible.
#define SINGLE_NA				F_NA		///< NA for a float is the value returned by nanf(). Type does not exist in R.
#define STRING_NA				0			///< NA for a string coded as CELL_TYPE_STRING
#define STRING_EMPTY			1			///< An empty string coded as CELL_TYPE_STRING
#define LONG_INTEGER_NA			LLONG_MIN	///< NA for a 64-bit integer. Type does not exist in R.
#define TIME_POINT_NA			0			///< NA for a CELL_TYPE_TIME is a 64-bit zero. Type does not exist in R.
#define DOUBLE_NA				R_NA		///< NA for a double. This is R compatible.

/// Possible return values of Filter.filter_type()

#define FILTER_TYPE_

#define FILTER_TYPE_NOTAFILTER	0			///< This Block cannot be used as a filter. A try to use it in new_jazz_block() will fail.
#define FILTER_TYPE_BOOLEAN	 	1			///< This Block is a vector of CELL_TYPE_BYTE_BOOLEAN for each row.
#define FILTER_TYPE_INTEGER		2			///< This Block is a vector of CELL_TYPE_INTEGER containing the selected rows.


typedef std::chrono::steady_clock::time_point TimePoint;	///< A time point stored as 8 bytes

struct FilterSize { int one; int length; };	///< Two names for the first two elements in a TensorDim


/** The dimension of a tensor.

	The structure is declared as a union to make filter operation more elegant. A filter is a record that always has rank 1 and a size,
	but has an extra parameter, its length. Since dim[1] is not used (as rank is 1), it is a good place to store the length but remembering
	that dim[1] is the length is ugly. Therefore, the more elegant filter.length is an alias for dim[1].
*/
union TensorDim
{
	int		   dim[MAX_TENSOR_RANK];	///< Dimensions for the Tensor. The product of all * (cell_type & 0xff) < 2 Gb
	FilterSize filter;					///< When object is a Filter the second element is named filter.length rather than dim[1]
};


/// A tensor of cell size 1, 4 or 8
union Tensor
{
	uint8_t	  cell_byte[0];		///< Cell size for CELL_TYPE_BYTE
	bool	  cell_bool[0];		///< Cell size for CELL_TYPE_BYTE_BOOLEAN
	int		  cell_int[0];		///< Cell size for CELL_TYPE_INTEGER, CELL_TYPE_FACTOR, CELL_TYPE_GRADE, CELL_TYPE_BOOLEAN and CELL_TYPE_STRING
	uint32_t  cell_uint[0];		///< Cell size for matching CELL_TYPE_SINGLE or CELL_TYPE_BOOLEAN as 32 bit unsigned
	float	  cell_single[0];	///< Cell size for CELL_TYPE_SINGLE
	long long cell_longint[0];	///< Cell size for CELL_TYPE_LONG_INTEGER and CELL_TYPE_TIME
	uint64_t  cell_ulongint[0];	///< Cell size for matching CELL_TYPE_DOUBLE or CELL_TYPE_TIME as 64 bit unsigned
	double	  cell_double[0];	///< Cell size for CELL_TYPE_DOUBLE
};


/// Header for a Block
struct BlockHeader
{
	int	cell_type;				///< The type for the cells in the tensor. See CELL_TYPE_*
	int	rank;					///< The number of dimensions
	TensorDim range;			///< The dimensions of the tensor in terms of ranges (Max. size is 2 Gb.)
	int size;					///< The total number of cells in the tensor
	int num_attributes;			///< Number of elements in the JazzAttributesMap
	int total_bytes;			///< Total size of the block everything included
	bool has_NA;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
	TimePoint created;			///< Timestamp when the block was created
	uint64_t hash64;			///< Hash of everything but the header

	Tensor tensor;				///< A tensor for type cell_type and dimensions set by Block.set_dimensions()
};


/// Structure at the end of a Block, initially created with init_string_buffer()
struct StringBuffer
{
	bool stop_check_4_match;	///< When the StringBuffer is small, try to match existing indices of the same string to save RAM
	bool alloc_failed;			///< A previous call to get_string_offset() failed to alloc space for a string
	int	 last_idx;				///< The index to the first free space after the last stored string
	int	 buffer_size;			///< The size in bytes of buffer[]
	char buffer[];				///< The buffer where strings are stored starting with two zeroes for STRING_NA & STRING_EMPTY
};


extern float  F_NA;		///< NaN in single
extern double R_NA;		///< NaN in double

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TYPES

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


/*! \brief Basic Jazz code-less structures and constants.

	This namespace includes utilities, types, constants and structures used by Block, all different block-based data and code structures
	(Tuple, Kind and Field). All the Services used to allocate/store and communicate blocks (Volatile, Persisted, Cluster and Flux).

	All together is instanced in the server as the BEAT (of Jazz).
*/
namespace jazz_elements
{

/** API buffers limit the size of http API calls, but also anything like lists of item names, dimension names, types, blocktypes, etc.
Since Blocks do allocate RAM, when they communicate these kind of text operations, they expect the caller to assign a buffer of
ANSWER_LENGTH chars. Of course, data serialization does not have any limits it is done by containers creating new blocks.
*/
#define ANSWER_LENGTH			4096
#define SAFE_URL_LENGTH			2048		///< Maximum safe assumption of URL length for both parsing and forwarding.
#define NAME_SIZE				  32		///< Size of a Name (ending 0 included)

/// Block API (syntax related)

#define REGEX_VALIDATE_NAME				"^[a-zA-Z][a-zA-Z0-9_]{0,30}$"	///< Regex validating a Name

/** Number of elements preallocated in thread-specific buffers. Jazz is thread safe in a caller transparent way. The Block level API
does not normally modify blocks. The few exceptions have a block-specific lock in the BlockKeeper. Services also have a service-specific
lock. The few services that require full thread awareness (Bebop and API) will allocate a number of Core or APIexecutor objects equal to
JAZZ_MAX_NUM_THREADS inside the service. This number can be modified down (but not up) via the configuration keys: MHD_THREAD_POOL_SIZE
and BEBOP_NUM_CORES. As expected, MHD_THREAD_POOL_SIZE also defines the thread pool size allocated in libmicrohttpd.
*/
#define JAZZ_MAX_NUM_THREADS	64

#define MAX_TENSOR_RANK			6			///< Maximum rank = 6, E.g. a 2D array of raw videos (row, column, frame, x, y, color)
#define MAX_CHECKS_4_MATCH		25			///< Maximum number of tries to match in get_string_offset() before setting stop_check_4_match

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

// 64 bit cell types
#define CELL_TYPE_TUPLE_ITEM	0x030		///< A vector of ItemHeader (in a Tuple)
#define CELL_TYPE_KIND_ITEM		0x130		///< A vector of ItemHeader (in a Kind)

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

/// Possible return values of Block.filter_type() and Block.filter_audit()

#define FILTER_TYPE_

#define FILTER_TYPE_NOTAFILTER	0			///< This Block cannot be used as a filter. A try to use it in new_jazz_block() will fail.
#define FILTER_TYPE_BOOLEAN		1			///< This Block is a vector of CELL_TYPE_BYTE_BOOLEAN for each row.
#define FILTER_TYPE_INTEGER		2			///< This Block is a vector of CELL_TYPE_INTEGER containing the selected rows.

/// Possible return values of Kind.kind_audit() and Tuple.tuple_audit()

#define KIND_TYPE_

#define KIND_TYPE_NOTAKIND		0			///< The kind fails audit (wrong or duplicate item names, wrong dimensions, ...).
#define KIND_TYPE_KIND		 	1			///< This Block is a valid Kind.
#define KIND_TYPE_TUPLE			2			///< This Block is a valid Tuple.

/// Attribute types for Block descendants

#define BLOCK_ATTRIB_

#define BLOCK_ATTRIB_BLOCKTYPE	0			///< The fundamental block type (E.g., all tuples are tuple, filter exists, but table doesn't.)
#define BLOCK_ATTRIB_TYPE		1			///< The name of the c++ class (Flux, Group, Table, ... but also Block or Kind)
#define BLOCK_ATTRIB_KIND		2			///< The location of the kind (for tuples and descendants)
#define BLOCK_ATTRIB_SOURCEKIND	3			///< The location of the source kind (for fields, fluxes and thing with two Kinds)
#define BLOCK_ATTRIB_DESTKIND	4			///< The location of the destination kind (for fields, fluxes and thing with two Kinds)
#define BLOCK_ATTRIB_MIMETYPE	5			///< The mime type (can also be some proprietary file spec. E.g., "Adobe PhotoShop Image")
#define BLOCK_ATTRIB_URL		6			///< A url for the server to expose the file by.
#define BLOCK_ATTRIB_LANGUAGE	7			///< An http language identifier that will be returned in a / interface GET call.


typedef std::chrono::steady_clock::time_point TimePoint;	///< A time point stored as 8 bytes


/** The identifier of a Container type, a container inside another container, a Block descendant in a container, a field in a Tuple or
Kind, or the name of a contract. It must be a string matching REGEX_VALIDATE_NAME.
*/
typedef char Name[NAME_SIZE];

typedef char		 *pChar;				///< A pointer to char.
typedef 	   Name	 *pName;


/** Names for elements in a TensorDim to make filter operation more elegant. A filter is a record that always has rank 1 and a size,
	but has an extra parameter, its length.
*/
struct FilterSize {
	int one;		///< This is dim[0]. Since the filter is a block (of rank 1). This is always == 1 (see Block.get_offset()).
	int length;		///< Since dim[1] is not used (as rank is 1), it is a good place to store the length of the block this can filter.
};


/** The dimension of a tensor.

	The structure is declared as a union to make filter operation more elegant.
*/
union TensorDim
{
	int		   dim[MAX_TENSOR_RANK];	///< Dimensions for the Tensor. The product of all * (cell_type & 0xff) < 2 Gb
	FilterSize filter;					///< When object is a filter the second element is named filter.length rather than dim[1]
};


/// Header for an item (of a Kind or Tuple)
struct ItemHeader
{
	int	cell_type;				///< The type for the cells in the item. See CELL_TYPE_*
	int	rank;					///< The number of dimensions
	int level;					///< The 0-based level (== depth of this item in the tree of items)
	int name;					///< The name of this item as an offset in StringBuffer.
	int size;					///< The total number of cells in the item's tensor. (If is is a Tuple.)
	int data_start;				///< The data start of this tensor as an offset of &BlockHeader.tensor. (If is is a Tuple.)
	int	dim[MAX_TENSOR_RANK];	///< Dimensions for the Tensor. For Kind: negative numbers are dimension names as -offset in StringBuffer.
};


/// A tensor of cell size 1, 4, 8 or sizeof(BlockHeader)
union Tensor
{
	uint8_t	   cell_byte[0];		///< Cell size for CELL_TYPE_BYTE
	bool	   cell_bool[0];		///< .. CELL_TYPE_BYTE_BOOLEAN
	int		   cell_int[0];			///< .. CELL_TYPE_INTEGER, CELL_TYPE_FACTOR, CELL_TYPE_GRADE, CELL_TYPE_BOOLEAN and CELL_TYPE_STRING
	uint32_t   cell_uint[0];		///< .. CELL_TYPE_SINGLE or CELL_TYPE_BOOLEAN as 32 bit unsigned
	float	   cell_single[0];		///< .. CELL_TYPE_SINGLE
	long long  cell_longint[0];		///< .. CELL_TYPE_LONG_INTEGER and CELL_TYPE_TIME
	uint64_t   cell_ulongint[0];	///< .. CELL_TYPE_DOUBLE or CELL_TYPE_TIME as 64 bit unsigned
	double	   cell_double[0];		///< .. CELL_TYPE_DOUBLE
	ItemHeader cell_item[0];		///< .. An array of BlockHeader used by Kinds and Tuples
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


/** A string possibly returned by a contract. (Contracts return either a block or an answer.)
*/
struct Answer {
	char text[ANSWER_LENGTH];	///< A message, metadata, lists of items, columns, etc.
};

typedef struct Answer	*pAnswer;


extern float  F_NA;		///< NaN in single
extern double R_NA;		///< NaN in double

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TYPES

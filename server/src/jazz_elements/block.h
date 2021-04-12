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


#include <limits.h>
#include <map>
#include <string.h>
#include <iostream>


#include "src/jazz_elements/network.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_BLOCK
#define INCLUDED_JAZZ_ELEMENTS_BLOCK


namespace jazz_elements
{
/* \brief The class Block

	This module defines the logic to get/set data from/to Block objects at the simplest level.
*/


// Forward declarations
typedef std::map<int, const char *> AttributeMap;

typedef struct BlockHeader	*pBlockHeader;
typedef struct StringBuffer	*pStringBuffer;

typedef class Block			*pBlock;


/** A block. Anything in Jazz is a block. A block is a BlockHeader, followed by a tensor, then two arrays of int
of length == num_attributes, then a StringBuffer. Nothing in a Block is a pointer, Blocks can be copied
or stored 'as is', every RAM location in a block is defined by its BlockHeader and computed by the methods in Block.

At this level, you only have the fields BlockHeader that you may read and probably only write through some methods.
This is the lowest level, it does not even provide support for allocation, at this level you have support for manipulating
the StringBuffer to read and write strings and the JazzAttributesMap to read and write attributes.

A **filter** (is not a separate class anymore) is just a Block with a strict structure and extra methods.

The structure of a filter is strictly:

- .cell_type CELL_TYPE_BYTE_BOOLEAN or CELL_TYPE_INTEGER
- .rank == 1
- .range.filter.one == 1, .range.filter.length == number of elements in the tensor when cell_type == CELL_TYPE_INTEGER
- .size == length of the selector
- .num_attributes == 1	// To prevent set_attributes() being applied to it
- .total_bytes == as expected
- .has_NA == false;
- .created == as expected
- .hash64 == as expected

Details:

1. A filter has a length (.size) and can only filter in blocks where the number of rows (the first dimension) equals that length.
2. A FILTER_TYPE_BOOLEAN is a vector of CELL_TYPE_BYTE_BOOLEAN specifying which rows are selected (cell == true).
3. A FILTER_TYPE_INTEGER is a vector of ordered CELL_TYPE_INTEGER in 0..(size-1) whose length is stored in .range.filter.length.
As expected, .range.filter.length == 0 means nothing is selected, .range.filter.length == .size means everything is selected regardless
of .tensor[]
*/
class Block: public BlockHeader {

	public:

	// Methods on indices.

		/** Sets the tensor dimensions from a TensorDim array.

			\param p_dim A pointer to the TensorDim containing the dimensions.

			NOTES: 1. This writes: rank, range[] and size.
				   2. rank counts the number of dimension >0, except, when all dimensions == 0 produces: rank == 1, size == 0
				   3. size returns the size in number of cells, not bytes.
		*/
		inline void set_dimensions(int *p_dim) {
			rank = MAX_TENSOR_RANK;
			int j = 1;
			for (int i = MAX_TENSOR_RANK - 1; i > 0; i--)
				if (p_dim[i] > 0) { range.dim[i] = j; j *= p_dim[i]; } else { j = 1; range.dim[i] = 0; rank = i; }
			range.dim[0] = j;
			j			*= p_dim[0];
			size		 = j;
		}

		/** Returns the tensor dimensions as a TensorDim array.

			\param p_dim A pointer to the TensorDim containing the dimensions.

			NOTES: See notes on set_dimensions() to understand why in case of 0 and 1, it may return different values than those
			passed when the block was created with a set_dimensions() call.
		*/
		inline void get_dimensions(int *p_dim) {
			int j = size;
			for (int i = 0; i < MAX_TENSOR_RANK; i++)
				if (i < rank) { p_dim[i] = j/range.dim[i]; j = range.dim[i]; } else p_dim[i] = 0;
		}

		/** Returns if an index (as a TensorDim array) is valid for the tensor.

			\param p_idx A pointer to the TensorDim containing the index.

			\return	True if the index is valid.
		*/
		inline bool validate_index(int *p_idx) {
			int j = size;
			for (int i = 0; i < rank; i++) {
				if (p_idx[i] < 0 || p_idx[i]*range.dim[i] >= j) return false;
				j = range.dim[i];
			}
			return true;
		}

		/** Returns if an offset (as an integer) is valid for the tensor.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return	True if the offset is valid.
		*/
		inline int validate_offset(int offset) { return offset >=0 & offset < size; }

		/** Convert an index (as a TensorDim array) to the corresponding offset without checking its validity.

			\param p_idx A pointer to the TensorDim containing the index.

			\return	The offset corresponding to the same cell if the index was in a valid range.
		*/
		inline int get_offset(int *p_idx) {
			int j = 0;
			for (int i = 0; i < rank; i++) j += p_idx[i]*range.dim[i];
			return j;
		}

		/** Convert an offset to a tensor cell into its corresponding index (as a TensorDim array) without checking its validity.

			\param offset The input offset
			\param p_idx  A pointer to the TensorDim to return the result.
		*/
		inline void get_index(int offset, int *p_idx) {
			for (int i = 0; i < rank; i++) { p_idx[i] = offset/range.dim[i]; offset -= p_idx[i]*range.dim[i]; }
		}

	// Methods on strings.

		/** Get a string from the tensor by index without checking index range.

			\param p_idx A pointer to the TensorDim containing the index.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int *p_idx) {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_int[get_offset(p_idx)]]);
		}

		/** Get a string from the tensor by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int offset)	 {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_int[offset]]);
		}

		/** Set a string in the tensor, if there is enough allocation space to contain it, by index without checking index range.

			\param p_idx A pointer to the TensorDim containing the index.
			\param p_str A pointer to a (zero ended) string that will be allocated inside the Block.

			NOTE: Allocation inside a Block is typically hard since they are created with "just enough space", a Block is
			typically immutable. jazz_alloc.h contains methods that make a Block bigger if that is necessary. This one doesn't.
			The 100% safe way is creating a new block from the immutable one using jazz_alloc.h methods. Otherwise, use at your own
			risk or not at all. When this fails, it sets the variable alloc_failed in the StringBuffer. When alloc_failed is
			true, it doesn't even try to allocate.
		*/
		inline void set_string(int *p_idx, const char *p_str) {
			pStringBuffer psb = p_string_buffer();
			tensor.cell_int[get_offset(p_idx)] = get_string_offset(psb, p_str);
		}

		/** Set a string in the tensor, if there is enough allocation space to contain it, by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.
			\param p_str  A pointer to a (zero ended) string that will be allocated inside the Block.

			NOTE: Allocation inside a Block is typically hard since they are created with "just enough space", a Block is
			typically immutable. jazz_alloc.h contains methods that make a Block bigger if that is necessary. This one doesn't.
			The 100% safe way is creating a new block from the immutable one using jazz_alloc.h methods. Otherwise, use at your own
			risk or not at all. When this fails, it sets the variable alloc_failed in the StringBuffer. When alloc_failed is
			true, it doesn't even try to allocate.
		*/
		inline void set_string(int offset, const char *p_str) {
			pStringBuffer psb = p_string_buffer();
			tensor.cell_int[offset] = get_string_offset(psb, p_str);
		}

	// Methods on attributes.

		/** Find an attribute by its attribute_id (key).

			\param attribute_id A unique 32-bit id defining what the attribute is: e.g., a Jazz class, a url, a mime type, ...

			\return A pointer to where the (zero ended) string (the attribute value) is stored in the Block. Or nullptr if
					the key is not found.

			NOTE: Search is linear since no assumptions can be made on how keys are ordered. Typically Blocks have few (less
			than 5) attributes and that is okay. There is no limit on the number of attributes, so if you want to use the block's
			attributes as a dictionary containing, e.g., thousands of config keys, use get_attributes() instead which returns a
			map with all the attributes.
		*/
		inline char *find_attribute(int attribute_id) {
			int *ptk = p_attribute_keys();
			for (int i = 0; i < num_attributes; i++)
				if (ptk[i] == attribute_id)
					return reinterpret_cast<char *>(&p_string_buffer()->buffer[ptk[i + num_attributes]]);
			return nullptr;
		}

		/** Set all attributes of a Block, only when creating it, using a map.

			\param all_att A map containing all the attributes for the block.

			NOTE: This function is public because it has to be called by jazz_alloc.h methods. set_attributes() can
			only be called once, so it will do nothing if called after a Block is built. Blocks are near-immutable
			objects, if you need to change a Block's attributes create a new object using jazz_alloc.h methods.
		*/
		inline void set_attributes(AttributeMap *all_att) {
			if (num_attributes) return;

			num_attributes = all_att->size();
			init_string_buffer();

			int i = 0;
			int *ptk = p_attribute_keys();
			pStringBuffer psb = p_string_buffer();
			for (AttributeMap::iterator it = all_att->begin(); it != all_att->end(); ++it) {
				if (i < num_attributes) {
					ptk[i] = it->first;
					ptk[i + num_attributes] = get_string_offset(psb, it->second);
				}
				i++;
			}
		}

		/** Get all attributes of a Block storing them inside a map.

			\param all_att A (typically empty) map where all the attributes will be stored.

			NOTE: You can use a non-empty map. This will keep existing key/values not found in the Block and create/override
			those in the Block by using a normal 'map[key] = value' instruction.
		*/
		inline void get_attributes(AttributeMap *all_att) {
			int *ptk = p_attribute_keys();
			pStringBuffer psb = p_string_buffer();
			for (int i = 0; i < num_attributes; i++)
				(*all_att)[ptk[i]] = reinterpret_cast<char *>(&psb->buffer[ptk[i + num_attributes]]);
		}

		/** Initialize the StringBuffer of a Block, only when creating it.

			NOTE: This function is public because it has to be called by jazz_alloc.h methods. Never call it to construct
			you own Blocks except for test cases or in jazz_alloc.h methods. Use those to build Blocks instead.
		*/
		inline void init_string_buffer() {
			pStringBuffer psb = p_string_buffer();

			int buff_size = total_bytes - ((uintptr_t) psb - (uintptr_t) &cell_type) - sizeof(StringBuffer);
			if (buff_size < 4) {
				psb->alloc_failed = true;
				return;
			}
			memset(psb, 0, sizeof(StringBuffer) + 4);	// This also clears psb->buffer[0..3]
			// psb->buffer[0] = 0;	// STRING_NA
			// psb->buffer[1] = 0;	// STRING_EMPTY
			// psb->buffer[2] = 0;	// The end of string for searching (will change once a string is inserted)
			psb->buffer_size = buff_size;
			psb->last_idx	 = 2;	// Where the first string will be inserted
		}

		bool find_NAs_in_tensor();

		/** Align a pointer (as uintptr_t) to the next 16 byte boundary.
		*/
		inline int *align_128bit(uintptr_t ipt) {
			return reinterpret_cast<int *>((ipt + 0xf) & 0xffffFFFFffffFFF0);
		}

		/** Return the address of the vector containing both the attribute keys and the attribute ids in the StringBuffer.

			NOTE: The actual values (which are strings) are stored in the same StringBuffer containing the strings of the tensor
			(if any). This array has double the num_attributes size and stores the keys in the lower part and the offsets to the
			values on the upper part.
		*/
		inline int *p_attribute_keys() {
			return align_128bit((uintptr_t) &tensor + (cell_type & 0xff)*size);
		}

		/** Return the address of the StringBuffer containing the strings in the tensor and the attribute values.
		*/
		inline pStringBuffer p_string_buffer() {
			return reinterpret_cast<pStringBuffer>((uintptr_t) p_attribute_keys() + 2*num_attributes*sizeof(int));
		}

		int get_string_offset(pStringBuffer psb, const char *p_str);

	// Methods for filtering (selecting).

		/** Check (fast) the validity of a filter and return its type or FILTER_TYPE_NOTAFILTER if invalid

			This checks the values in the header, but not the validity of the data in .tensor[]

			\return FILTER_TYPE_BOOLEAN or FILTER_TYPE_INTEGER if it is a valid filter of that type, FILTER_TYPE_NOTAFILTER if not.
		*/
		inline int filter_type() {
			if (rank != 1 || range.filter.one != 1 || num_attributes != 1 || has_NA)
				return FILTER_TYPE_NOTAFILTER;

			if (cell_type == CELL_TYPE_INTEGER)
				return FILTER_TYPE_INTEGER;

			if (cell_type == CELL_TYPE_BYTE_BOOLEAN)
				return FILTER_TYPE_BOOLEAN;

			return FILTER_TYPE_NOTAFILTER;
		}

		int filter_audit();

		/** Check (fast) if a filter is valid and can be applied to filter inside a specific Block

			This is verifies (size == number of rows) and calls filter_type() to check its requirements too.

			\return true if it is a valid filter of that type.
		*/
		inline bool can_filter(pBlock p_block) {
			if (p_block->rank < 1 || p_block->range.dim[0] <= 0 || size != p_block->size/p_block->range.dim[0])
				return false;

			return filter_type() != FILTER_TYPE_NOTAFILTER;
		}

		/** Set has_NA, the creation time and the hash64 of a JazzBlock based on the content of the tensor

			Despite its name, this function does not actually "close" anything. JazzBlock manipulation is based on "good will",
			after calling close_jazz_block() the owner should not change the content. If you do, you should close_jazz_block() again after.

			close_jazz_block() can be called any number of times on the same block.

			\param p_block The block to be "closed".
		*/
		inline void finish_creation(int set_has_NA = SET_HAS_NA_AUTO,
									bool set_hash = true,
									bool set_time = true) {
			switch (set_has_NA) {
			case SET_HAS_NA_FALSE:
				has_NA = false;
				break;
			case SET_HAS_NA_TRUE:
				has_NA = cell_type != CELL_TYPE_BYTE;	// CELL_TYPE_BYTE must always be has_NA == FALSE
				break;
			default:
				has_NA = find_NAs_in_tensor();
			}

#ifdef DEBUG	// Initialize the RAM between the end of the string buffer and last allocated byte for Valgrind.
			pStringBuffer psb = p_string_buffer();

			char *pt1 = (char *) &psb->buffer[psb->last_idx],
				 *pt2 = (char *) &cell_type + total_bytes;

			while (pt1 < pt2) {
				pt1[0] = 0;
				pt1++;
			}
#endif
			if (set_hash)
				hash64 = MurmurHash64A(&tensor, total_bytes - sizeof(BlockHeader));

			if (set_time)
				created = std::chrono::steady_clock::now();
		}

};

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_BLOCK

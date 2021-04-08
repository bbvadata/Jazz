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

#include "src/jazz_elements/kind.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE
#define INCLUDED_JAZZ_ELEMENTS_TUPLE


namespace jazz_elements
{

typedef class Tuple *pTuple;

/** \brief Tuple: A Jazz data block of a Kind.

An instance of a **Kind**. Physically, like a Kind, it is a single block with some differences:

- It holds data and metadata. (It has a reference to the kind, but that is only interesting in the context of formal fields. Anything
  required to understand the data architecture is in the block itself.)
- It has constant values for the dimensions. E.g., all instances of "image_width" across all items where applicable are, say 640.
- It has different attributes than a Tuple.

Also, tuples always define, if the Container sets them as expected, these attributes:

- BLOCK_ATTRIB_BLOCKTYPE as the const "tuple"
- BLOCK_ATTRIB_TYPE as whatever the inherited class name is ("Tuple" for the main class)
- BLOCK_ATTRIB_KIND as the locator to the definition of the Kind it satisfies. (Can be a tab separated list.)

*/
class Tuple : public Block {

	public:

	// Methods to create a Tuple:

		/** Initializes a Tuple object (step 1): Allocates the space.

			\param num_bytes The size in bytes allocated. Should be enough for all names, data, ItemHeaders and attributes.

			\return			 False on error (insufficient alloc size for a very conservative minimum).
		*/
		inline bool new_tuple (int num_bytes) {

			return false;
		}

		/** Initializes a Tuple object (step 2): Adds an item to the (unfinished) tuple.

			\param p_block A tensor to be copied into the Tuple.

			\return		   False on error (insufficient alloc space or wrong block type).
		*/
		inline bool add_item (pBlock p_block) {

			return false;
		}

		/** Initializes a Tuple object (step 3): Set names, levels and attributes.

			\param p_names	A pointer to the names of all the items.
			\param attr		The attributes for the Tuple. Set "as is", without adding BLOCK_ATTRIB_BLOCKTYPE or BLOCK_ATTRIB_TYPE.
			\param p_levels A pointer to an array with the level of each item. (Allows to create a tree structure. nullptr => All == 0)

			\return		    False on error (insufficient alloc space for the strings).
		*/
		inline bool close_tuple (pNames		   p_names,
								 AttributeMap &attr,
								 int		  *p_levels = nullptr) {

			return false;
		}

	// Methods on Tuple items:

		/** Overrides the Block's p_attribute_keys() - Return the address of the vector containing both the attribute keys for a Tuple.

			NOTE: The tuple, besides metadata, contains data and therefore, the first tensor of ItemHeader corresponding to each
			item is followed by the tensors containing the data of each item. This changes the logic
		*/
		inline int *p_attribute_keys() {
			return align_128bit((uintptr_t) &tensor + (cell_type & 0xff)*size);
		}

		/** Get the name for an item of a Tuple by index without checking index range.

			\param idx The index of the item.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *item_name(int idx)	 {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_item[idx].name]);
		}

	// Methods taken for Block to tuple items:

		/** Returns the tensor dimensions as a TensorDim array.

			\param p_dim A pointer to the TensorDim containing the dimensions.

			NOTES: See notes on set_dimensions() to understand why in case of 0 and 1, it may return different values than those
			passed when the block was created with a set_dimensions() call.
		*/
		inline void get_dimensions(int item, int *p_dim) {
			int j = size;
			for (int i = 0; i < MAX_TENSOR_RANK; i++)
				if (i < rank) { p_dim[i] = j/range.dim[i]; j = range.dim[i]; } else p_dim[i] = 0;
		}

		/** Returns if an index (as a TensorDim array) is valid for the tensor.

			\param p_idx A pointer to the TensorDim containing the index.

			\return	True if the index is valid.
		*/
		inline bool validate_index(int item, int *p_idx) {
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
		inline int validate_offset(int item, int offset) { return offset >=0 & offset < size; }

		/** Convert an index (as a TensorDim array) to the corresponding offset without checking its validity.

			\param p_idx A pointer to the TensorDim containing the index.

			\return	The offset corresponding to the same cell if the index was in a valid range.
		*/
		inline int get_offset(int item, int *p_idx) {
			int j = 0;
			for (int i = 0; i < rank; i++) j += p_idx[i]*range.dim[i];
			return j;
		}

		/** Convert an offset to a tensor cell into its corresponding index (as a TensorDim array) without checking its validity.

			\param offset The input offset
			\param p_idx  A pointer to the TensorDim to return the result.
		*/
		inline void get_index(int item, int offset, int *p_idx) {
			for (int i = 0; i < rank; i++) { p_idx[i] = offset/range.dim[i]; offset -= p_idx[i]*range.dim[i]; }
		}

	// Methods taken for Block to tuple items of strings:

		/** Get a string from the tensor by index without checking index range.

			\param p_idx A pointer to the TensorDim containing the index.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int item, int *p_idx) {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_int[get_offset(item, p_idx)]]);
		}

		/** Get a string from the tensor by offset without checking offset range.

			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int item, int offset)	 {
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
		inline void set_string(int item, int *p_idx, const char *p_str) {
			pStringBuffer psb = p_string_buffer();
			tensor.cell_int[get_offset(item, p_idx)] = get_string_offset(psb, p_str);
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
		inline void set_string(int item, int offset, const char *p_str) {
			pStringBuffer psb = p_string_buffer();
			tensor.cell_int[offset] = get_string_offset(psb, p_str);
		}

	// Tuple/Kind specific methods:

		/** Verifies if a Tuple is of a Kind.

			\return True if the Tuple can be linked to a Kind (regardless of BLOCK_ATTRIB_KIND)
		*/
		inline bool is_a(pKind kind) {

			return 0;
		}

		inline int audit();
};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE

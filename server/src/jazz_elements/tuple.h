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

/// An array of pointers to Blocks to create Tuples in one call
typedef pBlock Blocks[0];


/** \brief Tuple: A Jazz Block with multiple Tensors.

Can be simplified as "An instance of a **Kind**" allthough that is not exactly what it is. It is an array of Tensors and it can match
one or more Kinds if its .is_a(<kind>) method returns true.

Physically, like a Kind, it is a single block with some differences:

- It holds data and metadata.
- It has constant values for the dimensions.
- It has a one step creation process: new_tuple().
- It also stores all the Blocks "as is" in the same space (after its header, vector of CELL_TYPE_KIND_ITEM, attribute keys and StringBuffer)
- The data stored @tensor is the metadata (like in a Kind) and the method .block(item) returns a pointer to each Block.
- A Tuple is a Block of type CELL_TYPE_TUPLE_ITEM (instead of CELL_TYPE_KIND_ITEM).
- The StringBuffer contains the item names and Tuple attributes. Blocks may have their own StringBuffers

Also, Tuples should define, if the Container sets them as expected, the attribute:

- BLOCK_ATTRIB_BLOCKTYPE as the const "Tuple"

Creating Tuples
---------------

More advanced Tuple functionalities (including other ways of creating tuples) can be done by Containers. This class, has a minimum
functionality to build Tuples: new_tuple() to do the basic building. It does not includes creating Tuples from other Tuples by appending the names.

Using Tuples
------------

Like any Block, it is a moveble structure that can be edited by this class. Channel-wise, Volatile, Persistenc or via the http API, it is
just a Block. Note:

- It has a new method, .block(item), that returns the address of an item.
- It has an is_a() method that verifies if it satisfies a Kind.
- It has an audit() method to check validity.
- Besides that, it is just a "big Block" whose header has a `total_bytes` that includes all the metadata and data.

*/
class Tuple : public Block {

	public:

	// Methods to create a Tuple:

		/** Initializes a Tuple object (step 1): Initializes the space.

			\param num_items The number of items the Tuple will have == size of the p_blocks-> and p_names->
			\param p_blocks  An array of pointer to the blocks to be included in the Tuple.
			\param p_names   An array of Name by which the items will go.
			\param num_bytes The size in bytes allocated. Should be enough for all names, data, ItemHeaders and attributes.
			\param attr		 The attributes for the Tuple. Set "as is", without adding BLOCK_ATTRIB_BLOCKTYPE or anything.

			\return			 0, SERVICE_ERROR_NO_MEM, SERVICE_ERROR_WRONG_TYPE, SERVICE_ERROR_WRONG_NAME, SERVICE_ERROR_WRONG_ARGUMENTS
		*/
		inline StatusCode new_tuple (int	 num_items,
									 Blocks &blocks,
									 pNames  p_names,
									 int	 num_bytes,
									 AttributeMap &attr) {

			int rq_sz = sizeof(BlockHeader) + sizeof(StringBuffer) + num_items*sizeof(ItemHeader) + (num_items + attr.size())*8;

			if (num_bytes < rq_sz)
				return SERVICE_ERROR_NO_MEM;

			memset(&cell_type, 0, rq_sz);

			cell_type	 = CELL_TYPE_TUPLE_ITEM;
			rank		 = 1;
			range.dim[0] = 1;
			size		 = num_items;
			total_bytes	 = num_bytes;

			set_attributes(&attr);

			pStringBuffer psb = p_string_buffer();

			for (int i = 0; i < num_items; i++) {
				pBlock p_block = blocks[i];
				if (p_block == nullptr) return SERVICE_ERROR_WRONG_ARGUMENTS;

				ItemHeader *p_it_hea = &tensor.cell_item[i];

				if (p_block->cell_type && 0xff > 8)
					return SERVICE_ERROR_WRONG_TYPE;

				pChar p_name = (pChar) &p_names->name[i];

				if (!valid_name(p_name))
					return SERVICE_ERROR_WRONG_NAME;

				p_it_hea->cell_type = p_block->cell_type;
				p_it_hea->name		= get_string_offset(psb, p_name);
				p_it_hea->rank		= p_block->rank;
				memcpy(p_it_hea->dim, p_block->range.dim, sizeof(TensorDim));

				if (p_it_hea->name <= STRING_EMPTY)
					return SERVICE_ERROR_NO_MEM;
			}

			int *p_dest = align_128bit((uintptr_t) &psb->buffer[psb->last_idx]);

			if ((uintptr_t) p_dest - (uintptr_t) &cell_type > num_bytes)
				return SERVICE_ERROR_NO_MEM;

			psb->buffer_size = (uintptr_t) p_dest - ((uintptr_t) &psb->buffer[0]);

			for (int i = 0; i < num_items; i++) {
				pBlock p_block = blocks[i];
				ItemHeader *p_it_hea = &tensor.cell_item[i];

				p_it_hea->data_start = (uintptr_t) p_dest - (uintptr_t) &tensor;

				if ((uintptr_t) p_dest - (uintptr_t) &cell_type + p_block->total_bytes > num_bytes)
					return SERVICE_ERROR_NO_MEM;

				memcpy(p_dest, p_block, p_block->total_bytes);

				p_dest = align_128bit((uintptr_t) p_dest + p_block->total_bytes);
			}

			return SERVICE_NO_ERROR;
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

			\param item  The index of the tuple item.
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

			\param item  The index of the tuple item.
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

			\param item   The index of the tuple item.
			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return	True if the offset is valid.
		*/
		inline int validate_offset(int item, int offset) { return offset >=0 & offset < size; }

		/** Convert an index (as a TensorDim array) to the corresponding offset without checking its validity.

			\param item  The index of the tuple item.
			\param p_idx A pointer to the TensorDim containing the index.

			\return	The offset corresponding to the same cell if the index was in a valid range.
		*/
		inline int get_offset(int item, int *p_idx) {
			int j = 0;
			for (int i = 0; i < rank; i++) j += p_idx[i]*range.dim[i];
			return j;
		}

		/** Convert an offset to a tensor cell into its corresponding index (as a TensorDim array) without checking its validity.

			\param item	  The index of the tuple item.
			\param offset The input offset
			\param p_idx  A pointer to the TensorDim to return the result.
		*/
		inline void get_index(int item, int offset, int *p_idx) {
			for (int i = 0; i < rank; i++) { p_idx[i] = offset/range.dim[i]; offset -= p_idx[i]*range.dim[i]; }
		}

	// Methods taken for Block to tuple items of strings:

		/** Get a string from the tensor by index without checking index range.

			\param item  The index of the tuple item.
			\param p_idx A pointer to the TensorDim containing the index.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int item, int *p_idx) {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_int[get_offset(item, p_idx)]]);
		}

		/** Get a string from the tensor by offset without checking offset range.

			\param item   The index of the tuple item.
			\param offset An offset corresponding to the cell as if the tensor was a linear vector.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *get_string(int item, int offset)	 {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_int[offset]]);
		}

		/** Set a string in the tensor, if there is enough allocation space to contain it, by index without checking index range.

			\param item  The index of the tuple item.
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

			\param item   The index of the tuple item.
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
typedef Tuple *pTuple;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE

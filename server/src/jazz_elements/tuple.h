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


#include <map>


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
- The data stored &tensor is the metadata (like in a Kind) and the method .block(item) returns a pointer to each Block.
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
			\param blocks	 An array of pointers to the blocks to be included in the Tuple.
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

		/** Get the name for an item of a Tuple by index without checking index range.

			\param idx The index of the item.

			\return A pointer to where the (zero ended) string is stored in the Block or nullptr for an invalid index.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *item_name(int idx)	 {
			if (idx < 0 | idx >= size)
				return nullptr;
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_item[idx].name]);
		}

		/** Get a Block from a tuple by item index.

			\param idx The index of the item.

			\return The block stored in the tuple or nullptr for an invalid index.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline pBlock block(int idx)	 {
			if (idx < 0 | idx >= size)
				return nullptr;
			return (pBlock) ((uintptr_t) &tensor + tensor.cell_item[idx].data_start);
		}

	// Tuple/Kind specific methods:

		/** Verifies if a Tuple is of a Kind.

			\return True if the Tuple can be linked to a Kind (regardless of BLOCK_ATTRIB_KIND)
		*/
		inline bool is_a(pKind kind) {
			if (kind->cell_type != CELL_TYPE_KIND_ITEM | kind->size != size)
				return false;

			std::map<std::string, int> dimension;

			for (int i = 0; i < size; i++) {
				if (  kind->tensor.cell_item[i].cell_type != tensor.cell_item[i].cell_type
					| kind->tensor.cell_item[i].rank	  != tensor.cell_item[i].rank)
					return false;

				if (strcmp(kind->item_name(i), item_name(i)))
					return false;

				for (int j = 0; j < tensor.cell_item[i].rank; j++) {
					int d_k = kind->tensor.cell_item[i].dim[j];

					if (d_k < 0) {
						std::string dim_name(kind->get_string(-d_k));
						if (dimension.count(dim_name)) {
							if (dimension[dim_name] != tensor.cell_item[i].dim[j])
								return false;
						} else {
							dimension[dim_name] = tensor.cell_item[i].dim[j];
						}
					} else {
						if (d_k != tensor.cell_item[i].dim[j])
							return false;
					}
				}
			}

			return true;
		}

		inline int audit();
};
typedef Tuple *pTuple;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE

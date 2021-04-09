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


#include <set>


#include "src/jazz_elements/container.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_KIND
#define INCLUDED_JAZZ_ELEMENTS_KIND


namespace jazz_elements
{

typedef class  Kind	 *pKind;
typedef struct Names *pNames;


/** An array of Item names (used to select items in a Tuple).
*/
struct Names {
	Name name[0];		///< The item names. First zero breaks.
};


/** \brief Kind: A type definition for complex Jazz objects.

Kind objects contain the metadata only. A Tuple is a data object of a Kind. Kinds define more complex types than (raw) Blocks, even if
they are blocks. E.g., A Block can store a video of a fixed shape (image only or soundtrack only). A Kind can store both and have
**dimensions** defining things like: image_width, image_height, number_of_frames and, possibly, a subtitle track as another vector of
strings.

It is a block with special attributes to store a tree of (primitive type, block, kind). A Kind is a single Block! A kind has **dimensions**
which are integer variables that are used to define variable shapes.

Technically, a kind is a Block of type CELL_TYPE_KIND_ITEM. Each item contains data (is a Tensor). Even if kinds extend other kinds, only
the leaves of the tree (the tensors) are included in the resulting kind with the appropriate level. See TensorDim.

Since kinds keep only metadata, the space, unlike in Tuples, is uninterrupted as in a normal block: (header, vector of CELL_TYPE_KIND_ITEM,
attribute keys, StringBuffer).

The StringBuffer contains the item names and dimension names. See TensorDim.

Creating Kinds
--------------

Many Kind functionalities (including creating kinds and creating kinds from merging kinds) are done by Containers. This object, nevetheless
has a minimum of functionality to build by parts: new_kind(), add_item() to do the basic building which will be called by the
Container. It also has function to check the content and validate a kind.

Also, kinds should define, these attributes, but that it left to the Container:

- BLOCK_ATTRIB_BLOCKTYPE as the const "kind"
- BLOCK_ATTRIB_TYPE as the const "Kind"

*/
class Kind : public Block {

	public:

		/** Get the name for an item of a Kind by index without checking index range.

			\param idx The index of the item.

			\return A pointer to where the (zero ended) string is stored in the Block.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *item_name(int idx)	 {
			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_item[idx].name]);
		}

		/** Returns the number of dimensions in a Kind.

			\return Number of dimensions
		*/
		inline int num_dimensions() {
			if (cell_type != CELL_TYPE_KIND_ITEM | size <= 0)
				return 0;

			std::set <int> dims;

			for (int i = 0; i < size; i++) {
				ItemHeader *p_it_hea = &tensor.cell_item[i];

				for (int j = 0; j < p_it_hea->rank; j++) {
					int k = p_it_hea->dim[j];
					if (k < 0)
						dims.insert(k);
				}
			}

			return dims.size();
		}

		/** Returns the names of the dimensions as a tab separated list of names.

			\param p_buff  The address of an ApiBuffer to store the answer.
		*/
		inline void dimension_names(pAnswer p_buff) {
			p_buff->text[0] = 0;

			if (cell_type != CELL_TYPE_KIND_ITEM | size <= 0)
				return;

			std::set <int> dims;

			for (int i = 0; i < size; i++) {
				ItemHeader *p_it_hea = &tensor.cell_item[i];

				for (int j = 0; j < p_it_hea->rank; j++) {
					int k = p_it_hea->dim[j];
					if (k < 0 & dims.find(k) == dims.end()) {
						if (dims.size() > 0)
							strcat(p_buff->text, ",");

						char * pt = (&p_string_buffer()->buffer[-k]);

						strcat(p_buff->text, pt);

						dims.insert(k);
					}
				}
			}
		}

		/** Initializes a Kind object (step 1): Allocates the space.

			\param num_items The number of items the Kind will have. This call must be followed by one add_item() for each of them.
			\param num_bytes The size in bytes allocated. Should be enough for all names, dimensions and attributes + ItemHeaders.
			\param attr		 The attributes for the Kind. Set "as is", without adding BLOCK_ATTRIB_BLOCKTYPE or BLOCK_ATTRIB_TYPE.

			\return			 False on error (insufficient alloc size for a very conservative minimum).
		*/
		inline bool new_kind (int			num_items,
							  int			num_bytes,
			   				  AttributeMap &attr) {

			int rq_sz = sizeof(BlockHeader) + sizeof(StringBuffer) + num_items*sizeof(ItemHeader) + (num_items + attr.size())*8;
			if (num_bytes < rq_sz)
				return false;

			memset(&cell_type, 0, num_bytes);

			cell_type	 = CELL_TYPE_KIND_ITEM;
			rank		 = 1;
			range.dim[0] = 1;
			size		 = num_items;
			total_bytes	 = num_bytes;

			set_attributes(&attr);

			return true;
		}

		/** Initializes a Kind object (step 2): Adds each of the items.

			\param idx		 The index of the items to add. Must be in range [0..num_items-1] of the previous new_kind() call.
			\param level	 The level of the item in the Kind (Allows to create a tree structure. All == 0 is okay.)
			\param p_name	 The name of the item.
			\param p_dim	 The shape of the item. Rank will be set automatically on the first zero.
			\param cell_type The cell type of the item.
			\param dims		 Names for the dimensions. See below.

			\return			 False on error (insufficient alloc space, wrong shape, wrong dimension names).

		How dimensions are defined.
		---------------------------

		Dimensions are placehoders in the tensor shapes that contain an integer variable such as "width" instade of a constant. They are
		defined by placing a unique negative number for each dimension in the tensor and giving it a name inside the dims map. E.g.,
		if an image has shape [-1, -2, 3] and dims[-1] == "width" and dims[-2] == "height", The kind will store these variable dimensions
		together with their names.
		*/
		inline bool add_item (int			idx,
							  int			level,
			   				  char const   *p_name,
							  int		   *p_dim,
							  int			cell_type,
							  AttributeMap &dims) {
			if (idx < 0 | idx >= size)
				return false;

			ItemHeader *p_it_hea = &tensor.cell_item[idx];

			int rank = 6, j, k;

			pStringBuffer psb = p_string_buffer();

			for (int i = MAX_TENSOR_RANK - 1; i >= 0; i--) {
				j = p_dim[i];
				p_it_hea->dim[i] = j;
				if (j == 0) {
					rank = i;
					if (!i)
						return false;
				} else if (j < 0) {
					if (dims.find(j) == dims.end())
						return false;

					k = get_string_offset(psb, dims[j]);
					if (k <= STRING_EMPTY)
						return false;

					p_it_hea->dim[i] = -k;
				}
			}

			p_it_hea->name = get_string_offset(psb, p_name);

			if (p_it_hea->name <= STRING_EMPTY)
				return false;

			p_it_hea->cell_type = cell_type;
			p_it_hea->rank		= rank;
			p_it_hea->level		= level;

			return true;
		}

		int audit();
};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_KIND

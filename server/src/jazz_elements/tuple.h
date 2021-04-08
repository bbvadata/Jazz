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

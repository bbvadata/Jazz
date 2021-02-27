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

#include "src/jazz_elements/block.h"

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

typedef class Kind *pKind;

/** \brief Kind: A type definition for complex Jazz objects.

Kind objects contain the metadata only. A Tuple is a data object of a Kind. Kinds define more complex types than (raw) Blocks, even if
they are blocks. E.g., A Block can store a video of a fixed shape (image only or soundtrack only). A Kind can store both and have
**dimensions** defining things like: image_width, image_height, number_of_frames and, possibly, a subtitle track as another vector of
strings.

It is a block with special attributes to store a tree of (primitive type, block, kind). A Kind is a single Block! A kind has **dimensions**
which are integer variables that are used to define variable shapes.

Technically, a kind is a Block of type CELL_TYPE_TUPLE_ITEM. Each item has data (is a Tensor). The way a Kind can include other Kinds is
by merging them together and that increases the ItemHeader.level by one. E.g, a Kind x is made of tensors (a, b) and a Kind y is made of
(c, d, e), we can create a kind (f, x, y) as we merge it together we will just have: (f, x_a, x_b, y_c, y_d, y_e) all of them will have
level 1, except f which will be level 0. The naming convention (mergining with an underscore) is what a Container would do by default when
merging Kinds, but, in manually created Kinds each item can have arbitrary (but different) names. They may have dimensions named length,
width, num_items or whatever and any of them can use these names to define variable dimensions of their tensors anywhere. They do so by
having the names stored as strings and referring to the name (by its index) in the ItemHeader.dim_name[·] where that dimension applies.

Also, kinds always define, at least, these attributes:

- BLOCK_ATTRIB_BLOCKTYPE as the const "kind"
- BLOCK_ATTRIB_TYPE as the const "Kind"

Since kinds keep only metadata, the space is uninterrupted as in a normal block: (header, vector of CELL_TYPE_TUPLE_ITEM, attribute
keys, StringBuffer), which is the only difference between a Kind an a tuple (besides BLOCK_ATTRIB_BLOCKTYPE and BLOCK_ATTRIB_TYPE values).

*/
class Kind : public Block {

	public:

	inline int num_items() {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int num_dimensions() {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int as_text(pApiBuffer p_buff) {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int dimension_names(pApiBuffer p_buff) {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int item_as_text(int i, pApiBuffer p_buff) {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int kind_audit() {
//TODO: Implement, test and document this.

		return 0;
	};

};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_KIND

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

Also, tuples always define, at least, these attributes:

- BLOCK_ATTRIB_BLOCKTYPE as the const "tuple"
- BLOCK_ATTRIB_TYPE as whatever the inherited class name is ("Tuple" for the main class)
- BLOCK_ATTRIB_KIND as the locator to the definition of the Kind it satisfies. (Can be a tab separated list.)

*/
class Tuple : public Block {

	public:

	/** Verifies if a Tuple is of a Kind.

		\return True if the Tuple can be linked to a Kind (regardless of BLOCK_ATTRIB_KIND)
	*/
	inline bool is_a(pKind kind) {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int tuple_audit();

};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE

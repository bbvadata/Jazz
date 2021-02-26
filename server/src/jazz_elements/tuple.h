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

/**< \brief A tuple is a tree of blocks where each edge has a name. A kind is a tuple with indexed dimensions.

A tuple is a tree of blocks where each edge has a name (and no more attributes). The names in the tuple select a specific block
inside the tuple, somehow like the folder names in a directory tree. A kind is an abstract tuple with some dimensions being parameters.
These parameters are indices. This module implements: Tuple, Index and Kind.

*/

/**< \brief Kind: A Jazz data block of a Kind.

An instance of a **Kind**. Physically, like a Kind, it is a single block.
*/
class Tuple : public Block {

	public:

	inline int kind(pApiBuffer p_buff) {
//TODO: Implement, test and document this.

		return 0;
	};

	inline int tuple_audit() {
//TODO: Implement, test and document this.

		return 0;
	};

};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TUPLE

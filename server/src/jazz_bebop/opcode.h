/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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

#include "src/jazz_bebop/bop.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_OPCODE
#define INCLUDED_JAZZ_BEBOP_OPCODE

//TODO: From ROADMAP

// Complete anything under jazz_bebop.h which is:

//   * definitions of classes
//   * designing execution mechanism that supports macros (snippets) and objects
//   * collections (field) just one thing, no packs
//   * mechanism to define object (as something that provides an interface to a container)
//   * testing a basic MVP
//   * approving a PR and moving the version towards 0.7.1

// Reaching "wide picture" requirements:

//   1. Build an end-to-end with tests that contains: opcode, field, core. MVP without snippets or mutators.
//   2. Build an end-to-end with tests that adds a mutator.
//   3. Build an end-to-end with tests that adds a snippet.
//   4. Build an end-to-end with tests that adds an uplifted field.
//   5. Remove packs, fields (plural only), cores (plural only).
//   6. Update config (to no packs)
//   7. Update diagrams
//   8. Create a minimal bop with simplest map/reduced arithmetic
//   9. Close PR and push version

// Next step in "big picture": rename jazz_model to jazz_brane and build the bricks for:

//   * concepts
//   * grammar
//   * context
//   * HITS



/** \brief OpCode: stateless functions and mutators. Category theory provides the necessary composition to manage state without
introducing stateless and stateful as separate categories. Only mutators (which introduce addresses) are a separate category.

These functions are known as op-codes and inherit OpCode. They are implemented in C++ in a pack.
*/

namespace jazz_bebop
{


} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_OPCODE


/* Jazz (c) 2018-2025 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_bebop/fields.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_SNIPPET
#define INCLUDED_JAZZ_BEBOP_SNIPPET


/** \brief A Concept ancestor that contains both the source and the object code

A Snippet is an object that can do both forward (compile) and reverse engineering (decompile) between compilable Bop and an onnx file.
It supports a number of serializations to and from Jazz Blocks. It forms the minimal unit of what can be run.

*/

//TODO: Synchronize http://localhost:8888/jazz_reference/api_ref_core.html (Erase this only when Core is done.)

namespace jazz_bebop
{

/** \brief Snippet: A code snippet and the ancestor of Concept.

A Snippet is a Tuple with items and some attributes. The items are sources and compiled onnx code. The attributes identify it as a Snippet
and manage state (). The Snippet object provides an interface to access the text parts (`uses` and `source`) as a std::vector of std::string.
Also, it provides many constructors to create a Snippet from source, object, ...

The items are:

source, input, output, reads, writes, calls, body, object
------  -----------------------------------------  ------
   |  				        |                         +--> The Onnx object code
   |                        +----------------------------> The preprocessed source code (Possibly reverse engineered)
   +-----------------------------------------------------> The original source code (If available)

The attributes are set in BLOCK_SNIPPET_STATE and are:

- SNIPSTATE_SOURCE_AVAILABLE	: The source code is available (has original source)
- SNIPSTATE_SOURCE_PREPROCESSED	: The source code has been preprocessed (has input, output, reads, writes, calls, body)
- SNIPSTATE_SOURCE_COMPILED		: The object code has been compiled (has object)
- SNIPSTATE_OBJECT_AVAILABLE	: The object code is available (the object is the original source)
- SNIPSTATE_OBJECT_PREPROCESSED	: The object code has been preprocessed (has input, output, reads, writes, calls, body)
- SNIPSTATE_RUNS				: The snippet has run successfully, (was either SNIPSTATE_SOURCE_COMPILED or SNIPSTATE_OBJECT_PREPROCESSED)
- SNIPSTATE_IS_RUNNING			: The snippet is currently mounted in an ONNX runtime session
- SNIPSTATE_FAILED_SRC_PREPROC	: The source code preprocessing failed
- SNIPSTATE_FAILED_SRC_COMPILE	: The object code compilation failed
- SNIPSTATE_FAILED_OBJ_PREPROC	: The object code preprocessing failed
- SNIPSTATE_FAILED_RUN			: The snippet failed to run

*/
class Snippet : public Tuple {

	public:

};
typedef Snippet *pSnippet;		///< A pointer to a Snippet

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_SNIPPET

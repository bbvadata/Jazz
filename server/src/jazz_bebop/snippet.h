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


#include "src/jazz_bebop/opcodes.h"

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

/** The kind of a Snippet
*/
#define KIND_SNIPPET	"{\"object\" : BYTE[obj_size], \"body\" : STRING[body_len], \"calls\" : STRING[calls_len]," \
						"\"input\" : STRING[input_len], \"output\" : STRING[output_len], \"reads\" : STRING[reads_len]," \
						"\"source\" : STRING[source_len], \"writes\" : STRING[writes_len]}"

/** The empty Snippet
*/
#define EMPTY_SNIPPET	"(\"object\" : [], \"body\" : [], \"calls\" : [], \"input\" : [], \"output\" : [], \"reads\" : []," \
						"\"source\" : [], \"writes\" : [])"

const char SNIPSTATE_EMPTY_SNIPPET[]		= "____";	///< The snippet is empty
const char SNIPSTATE_SOURCE_AVAILABLE[]		= "s___";	///< The source code is available
const char SNIPSTATE_SOURCE_PREPROCESSED[]	= "sx__";	///< The source code has been preprocessed successfully
const char SNIPSTATE_SOURCE_COMPILED[]		= "sxo_";	///< The source code has been compiled successfully
const char SNIPSTATE_OBJECT_AVAILABLE[]		= "__o_";	///< The snippet is created from an onnx object
const char SNIPSTATE_OBJECT_PREPROCESSED[]	= "_xo_";	///< The snippet (from onnx) has been reverse engineered successfully
const char SNIPSTATE_CAN_RUN[]				= "___a";	///< The global state is neither error nor empty state() >= SNIPSTATE_CAN_RUN
const char SNIPSTATE_CAN_RUN_OBJECT[]		= "_xor";	///< The snippet (from onnx) is ready to run (or has run before)
const char SNIPSTATE_CAN_RUN_SOURCE[]		= "sxor";	///< The snippet (from source) is ready to run (or has run before)
const char SNIPSTATE_IS_RUNNING_OBJECT[]	= "_xoi";	///< The snippet is currently running (from onnx)
const char SNIPSTATE_IS_RUNNING_SOURCE[]	= "sxoi";	///< The snippet is currently running (from source)
const char SNIPSTATE_FAILED_SRC_PREPROC[] 	= "sX_!";	///< The source code preprocessing failed
const char SNIPSTATE_FAILED_SRC_COMPILE[] 	= "sxO!";	///< The source code compilation failed
const char SNIPSTATE_FAILED_OBJ_PREPROC[] 	= "_Xo!";	///< The snippet (from onnx) reverse engineering failed
const char SNIPSTATE_FAILED_RUN_OBJECT[]	= "_xo!";	///< The snippet failed to run (from onnx)
const char SNIPSTATE_FAILED_RUN_SOURCE[]	= "sxo!";	///< The snippet failed to run (from source)

#define MASK_SNIPSTATE_GENERAL	0xff000000				///< The mask for the general state
#define MASK_SNIPSTATE_OBJECT	0x00ff0000				///< The mask for the object state
#define MASK_SNIPSTATE_INTER	0x0000ff00				///< The mask for the intermediate state
#define MASK_SNIPSTATE_SOURCE	0x000000ff				///< The mask for the source state
#define SNIPSTATE_UNDEFINED		0x7fffffff				///< An error state returned when there is no attribute


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

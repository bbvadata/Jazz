/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

   2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

		Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

	  This product includes software developed at

	   BBVA (https://www.bbva.com/)

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   3. LMDB: Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

   Licensed under http://www.OpenLDAP.org/license.html

   Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/* NOTES from the Jazz draft description including tasks for this
   --------------------------------------------------------------

In Jazz an lvalue is one of:

A chain of keeprs abstracted as block ids starting from root ending with the name of a block. They must all exist except, possibly, the last name. If the last name is new, it is created, if it exists, overridden.
A data block that will be returned as a Python object, R object or http (GET) resource.
In Jazz an rvalue is one of:

A block constructor. A constant expression that can be used to build a block from it.
Chains of keeprs abstracted as existing blocks starting from root. This includes functions and blocks passed as arguments to functions.
A combination of the previous two.
A data block that will be passed as a Python object, R object or http (PUT) resource.
A delete predicate. This deletes the corresponding lvalue.
Since the API has to be REST compatible and is intended for using over a network.

All rvalue evaluations are safe. They cannot have side effects. Function calls cannot have side effects.
All lvalue assignments are idempotent. Assigning twice has the same effect than assigning once. There is no += operator.

GET with a valid rvalue. To read from Jazz.
HEAD with a valid rvalue. Internally the same as GET, but returns the header only.
PUT with a valid lvalue. To write blocks into a Jazz keepr.
DELETE with a valid lvalue. To delete blocks or keeprs (even recursively).
OPTIONS with a string. Parses the string and returns the commands that would accept that string as a URL.
GET with lvalue=rvalue. Assignment in the server. Similar to “PUT(lvalue, GET(rvalue))” without traffic.

//TODO: Consider these statements when implementing jazz_api
//TODO: Create explanation on jazz_api for the doxy page and linked from the reference

jazz_get(rvalue)
jazz_put(object, lvalue)
jazz_delete(lvalue)
jazz_assign(lvalue, rvalue)

//TODO: Confirm top-level Python and R interfaces match this.

*/

#include "src/include/jazz.h"
#include "src/jazz_functional/jazz_bebop.h"


#ifndef INCLUDED_JAZZ_MAIN_API
#define INCLUDED_JAZZ_MAIN_API


/**< \brief TODO

//TODO: Write module description for jazz_api when implemented.
*/
namespace jazz_api
{

using namespace jazz_containers;

/*
all the characters are explained in containers:

	+ - * / &		- One char operators
	:sum:			- Composed operators
	. @ $			- First char in a block name
	()				- Evaluate (block must be a function)
	(arg_1,arg_2)	- Evaluate with arguments
	?(id;expr)		- A block constructor
	=				- Assignment lvalue=rvalue
	!				- Modifier
	!if_exists		- Assign only if exists			.x!if_exists=.y
	!if_not_exists	- Assign only if not exists		.x!if_not_exists=.y
	%20				- urlencode (space or any other char 2 digit hex)

#define JAZZ_REGEX_VALID_CHARS		   "^[a-zA-Z0-9\\+\\*\\.\\$\\(\\)\\?\\-_,;%@&:=/!]*$"
#define JAZZ_REGEX_VALID_SPACING										   "[\\n\\r \\t]"	///< Regex validating any spacing (is removed)
#define JAZZ_REGEX_VALIDATE_BLOCK_ID					"^(\\.|@|\\$)[a-zA-Z0-9_]{1,30}$"	///< Regex validating a JazzBlockIdentifier

#define JAZZ_MAX_BLOCK_ID_LENGTH								 32		///< Maximum length for a block name
#define JAZZ_BLOCK_ID_PREFIX_LOCAL								'.'		///< First char of a LOCAL JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_DISTRIB							'@'		///< First char of a DISTRIBUTED JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_UBIQUITOUS							'$'		///< First char of a UBIQUITOUS JazzBlockIdentifier

//TODO: Remove this (there is just one definition in jazz_containers) when the parser is written.

*/

/** \brief The JazzAPI is a keepr (JazzCache) named / (root in Unix) where all the keeprs and blocks are linked.

//TODO: Write the JazzAPI description
*/
class JazzAPI: public JazzCache {

	public:
		 JazzAPI(jazz_utils::pJazzLogger	 a_logger,
			     jazz_utils::pJazzConfigFile a_config);
		~JazzAPI();

		/// A StartService/ShutDown interface
		API_ErrorCode StartService ();
		API_ErrorCode ShutDown	   (bool restarting_service = false);

};


/** \brief R objects are limited to R core vectors, ergo not all the possible complexity of Jazz types is directly readable or writable to R.

//TODO: Write the rAPI description
*/
class rAPI: public JazzAPI {

	public:
		 rAPI(jazz_utils::pJazzLogger	  a_logger,
			  jazz_utils::pJazzConfigFile a_config);
		~rAPI();

};


/** \brief Same as before, objects are Python vectors, not numpy or Pandas objects, so similar limitations apply.

//TODO: Write the pyAPI description
*/
class pyAPI: public JazzAPI {

	public:
		 pyAPI(jazz_utils::pJazzLogger	   a_logger,
			   jazz_utils::pJazzConfigFile a_config);
		~pyAPI();

};

}

#endif

/* Jazz (c) 2018-2019 kaalam.ai (The Authors of Jazz), using (under the same license):

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

/* HUGE TODO LIST - not from this file
   ===================================

   This list is collected here for convenience. It is mostly documentation related and belong to all the project

	MAIN PAGE & INTRODUCTION


	NAMESPACES

//TODO: Edit doc of NAMESPACE jazz_bebop, current is: TODO
//TODO: Edit doc of NAMESPACE jazz_classes, current is: Definitions of Jazz root classes
//TODO: Edit doc of NAMESPACE jazz_cluster, current is: TODO
//TODO: Edit doc of NAMESPACE jazz_column, current is: TODO
//TODO: Edit doc of NAMESPACE jazz_containers, current is: Container classes for JazzBlock objects
//TODO: Edit doc of NAMESPACE jazz_datablocks, current is empty.
//TODO: Edit doc of NAMESPACE jazz_dataframe, current is: TODO
//TODO: Edit doc of NAMESPACE jazz_filesystem, current is: TODO
//TODO: Edit doc of NAMESPACE jazz_httpclient, current is: Simplest functionality to operate ...
//TODO: Edit doc of NAMESPACE jazz_persistence, current is: Jazz class JazzPersistedSource
//TODO: Edit doc of NAMESPACE jazz_restapi, current is empty.
//TODO: Edit doc of NAMESPACE jazz_stdcore, current is: Arithmetic, logic and type conversion ...
//TODO: Edit doc of NAMESPACE jazz_utils, current is empty.
//TODO: Update the list of NAMESPACE to be documented from the doxygen page.

	DATA STRUCTURES

//TODO: Edit doc of STRUC AATBlockQueue, current is empty.
//TODO: Edit doc of STRUC JazzBlockIdentifier, current is empty.
//TODO: Edit doc of STRUC JazzBlockKeepr, current is empty.
//TODO: Edit doc of STRUC JazzBlockKeeprItem, current is empty.
//TODO: Edit doc of STRUC JazzCache, current is empty.
//TODO: Edit doc of STRUC JazzQueueItem, current is empty.
//TODO: Edit doc of STRUC JazzTree, current is empty.
//TODO: Edit doc of STRUC JazzTreeItem, current is empty.
//TODO: Edit doc of STRUC FilterSize, current is: Two names for the first two elements in a JazzTensorDim
//TODO: Edit doc of STRUC JazzBlock, current is empty.
//TODO: Edit doc of STRUC JazzBlockHeader, current is: Header for a JazzBlock
//TODO: Edit doc of STRUC JazzFilter, current is empty.
//TODO: Edit doc of STRUC JazzStringBuffer, current is: Structure at the end of a JazzBlock, initially ...
//TODO: Edit doc of STRUC JazzTensor, current is: A tensor of cell size 1, 4 or 8
//TODO: Edit doc of STRUC JazzTensorDim, current is empty.
//TODO: Edit doc of STRUC JazzPersistence, current is empty.
//TODO: Edit doc of STRUC JazzPersistenceItem, current is empty.
//TODO: Edit doc of STRUC JazzSource, current is empty.
//TODO: Edit doc of STRUC JazzConfigFile, current is empty.
//TODO: Edit doc of STRUC JazzLogger, current is empty.
//TODO: Update the list of STRUC to be documented from the doxygen page.

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


} //namespace jazz_containers;

#endif

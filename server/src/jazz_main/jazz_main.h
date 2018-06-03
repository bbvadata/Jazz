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

   Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


/**< \brief	 The application entry point.

	Most high level server logic: see the description of main()
	The entry point is replaced by the catch2 entry point for unit testing.
*/


#include <iostream>
#include <signal.h>


#include "src/jazz_main/jazz_instances.h"

/* HUGE TODO LIST - not from this file
   ===================================

   This list is collected here for convenience. It is mostly documentation related and belong to all the project

	MAIN PAGE & INTRODUCTION


	NAMESPACES

//TODO: Edit doc of NAMESPACE jazz_bebop		current is: TODO
//TODO: Edit doc of NAMESPACE jazz_classes		current is: Definitions of Jazz root classes
//TODO: Edit doc of NAMESPACE jazz_cluster		current is: TODO
//TODO: Edit doc of NAMESPACE jazz_column		current is: TODO
//TODO: Edit doc of NAMESPACE jazz_containers	current is: Container classes for JazzBlock objects
//TODO: Edit doc of NAMESPACE jazz_datablocks	current is:
//TODO: Edit doc of NAMESPACE jazz_dataframe	current is: TODO
//TODO: Edit doc of NAMESPACE jazz_filesystem	current is: TODO
//TODO: Edit doc of NAMESPACE jazz_httpclient	current is: Simplest functionality to operate as an http client to GET, PUT, DELETE ...
//TODO: Edit doc of NAMESPACE jazz_persistence	current is: Jazz class JazzPersistedSource
//TODO: Edit doc of NAMESPACE jazz_restapi		current is:
//TODO: Edit doc of NAMESPACE jazz_stdcore		current is: Arithmetic, logic and type conversion stdcore applicable to JazzDataBlock ...
//TODO: Edit doc of NAMESPACE jazz_utils		current is:

	DATA STRUCTURES

//TODO: Edit doc of STRUC AATBlockQueue				current is:
//TODO: Edit doc of STRUC JazzBlockIdentifier		current is:
//TODO: Edit doc of STRUC JazzBlockKeepr			current is:
//TODO: Edit doc of STRUC JazzBlockKeeprItem		current is:
//TODO: Edit doc of STRUC JazzCache					current is:
//TODO: Edit doc of STRUC JazzQueueItem				current is:
//TODO: Edit doc of STRUC JazzTree					current is:
//TODO: Edit doc of STRUC JazzTreeItem				current is:
//TODO: Edit doc of STRUC FilterSize				current is: Two names for the first two elements in a JazzTensorDim
//TODO: Edit doc of STRUC JazzBlock					current is:
//TODO: Edit doc of STRUC JazzBlockHeader			current is: Header for a JazzBlock
//TODO: Edit doc of STRUC JazzFilter				current is:
//TODO: Edit doc of STRUC JazzStringBuffer			current is: Structure at the end of a JazzBlock, initially created with ...
//TODO: Edit doc of STRUC JazzTensor				current is: A tensor of cell size 1, 4 or 8
//TODO: Edit doc of STRUC JazzTensorDim				current is:
//TODO: Edit doc of STRUC JazzPersistence			current is:
//TODO: Edit doc of STRUC JazzPersistenceItem		current is:
//TODO: Edit doc of STRUC JazzSource				current is:
//TODO: Edit doc of STRUC JazzConfigFile			current is:
//TODO: Edit doc of STRUC JazzLogger				current is:

*/


// Command line arguments for the Jazz server

#define CMND_HELP		0	///< Command 'help' as a numerical constant (see parse_arg())
#define CMND_START		1	///< Command 'start' as a numerical constant (see parse_arg())
#define CMND_STOP		2	///< Command 'stop' as a numerical constant (see parse_arg())
#define CMND_STATUS		3	///< Command 'status' as a numerical constant (see parse_arg())

using namespace std;

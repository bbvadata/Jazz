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


#include "src/include/jazz.h"

/* HUGE TODO LIST - not from this file
   ===================================

   This list is collected here for convenience. It is mostly documentation related and belong to all the project

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

*/


// Command line arguments for the Jazz server

#define CMND_HELP		0	///< Command 'help' as a numerical constant (see parse_arg())
#define CMND_START		1	///< Command 'start' as a numerical constant (see parse_arg())
#define CMND_STOP		2	///< Command 'stop' as a numerical constant (see parse_arg())
#define CMND_STATUS		3	///< Command 'status' as a numerical constant (see parse_arg())

using namespace std;

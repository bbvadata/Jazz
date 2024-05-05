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


// #include <stl_whatever>


#include "src/jazz_bebop/core.h"


namespace jazz_bebop
{

using namespace jazz_elements;

/*	-----------------------------------------------
	 Core : I m p l e m e n t a t i o n
--------------------------------------------------- */

Core::Core(pLogger	   a_logger,
		   pConfigFile a_config,
		   pPack	   a_pack,
		   pField	   a_field) : Container(a_logger, a_config) {}


Core::~Core() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const Core::id() {
    static char arr[] = "Core from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the Core service
*/
StatusCode Core::start() {

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Core::shut_down() {

	return SERVICE_NO_ERROR;
}


#ifdef CATCH_TEST

Core CORE(&LOGGER, &CONFIG, &PAK);

#endif

} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_core.ctest"
#endif

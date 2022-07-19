/* Jazz (c) 2018-2022 kaalam.ai (The Authors of Jazz), using (under the same license):

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


namespace jazz_bebop
{

using namespace jazz_elements;

/*	-----------------------------------------------
	 Fields : I m p l e m e n t a t i o n
--------------------------------------------------- */

Fields::Fields(pLogger	   a_logger,
			   pConfigFile a_config,
			   pPack	   a_pack) : Container(a_logger, a_config) {}


Fields::~Fields() { destroy_container(); }


/** Starts the Fields service
*/
StatusCode Fields::start() {

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Fields::shut_down() {

	return SERVICE_NO_ERROR;
}


#ifdef CATCH_TEST

Fields FLS(&LOGGER, &CONFIG, &PAK);

#endif

} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_fields.ctest"
#endif

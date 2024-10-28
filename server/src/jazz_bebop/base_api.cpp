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


#include "src/jazz_bebop/base_api.h"


namespace jazz_bebop
{

/*	-----------------------------------------------
	 BaseAPI : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Creates a BaseAPI service without starting it.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
	\param a_channels	A pointer to an initialized Channels Container.
	\param a_volatile	A pointer to an initialized Volatile Container.
	\param a_persisted	A pointer to an initialized Persisted Container.

*/
BaseAPI::BaseAPI(pLogger a_logger,
				 pConfigFile a_config,
				 pChannels a_channels,
				 pVolatile a_volatile,
				 pPersisted a_persisted) : Container(a_logger, a_config) {}

BaseAPI::~BaseAPI() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const BaseAPI::id() {
    static char arr[] = "BaseAPI from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the BaseAPI service
*/
StatusCode BaseAPI::start() {

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode BaseAPI::shut_down() {

	return SERVICE_NO_ERROR;
}


#ifdef CATCH_TEST

BaseAPI BAPI(&LOGGER, &CONFIG, &CHN, &VOL, &PER);

#endif

} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_base_api.ctest"
#endif

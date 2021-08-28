/* Jazz (c) 2018-2021 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_bebop/core.h"


namespace jazz_bebop
{

/*	-----------------------------------------------
	 Bebop : I m p l e m e n t a t i o n
--------------------------------------------------- */

Bebop::Bebop(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


/** \brief Starts the service, checking the configuration and starting the Service.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

*/
StatusCode Bebop::start() {

	return SERVICE_NO_ERROR;
}


/** Shuts down the Bebop Service
*/
StatusCode Bebop::shut_down() {

	return SERVICE_NO_ERROR;
}


/** Run a function on an argument.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param function	The function to be called: entity == field, key == opcode.
	\param args		A Tuple passed as an argument.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Bebop::call (pTransaction &p_txn, Locator function, pTuple args) {

	return SERVICE_NOT_IMPLEMENTED;
}


} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_core.ctest"
#endif

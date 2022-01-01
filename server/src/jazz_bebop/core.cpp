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


#include "src/jazz_bebop/core.h"


//TODO: Implement cores

namespace jazz_bebop
{

/*	-----------------------------------------------
	 Bebop : I m p l e m e n t a t i o n
--------------------------------------------------- */

Bebop::Bebop(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


Bebop::~Bebop() { destroy_container(); }


/** \brief Starts the service, checking the configuration and starting the Service.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

*/
StatusCode Bebop::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	return SERVICE_NO_ERROR;
}


/** Shuts down the Bebop Service
*/
StatusCode Bebop::shut_down() {

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** The function call interface for **exec**: Execute an opcode in a formal field.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param function	Some description of a service. In general base/entity/key. In Channels the key must be empty and the entity is
					the pipeline. In Bebop, the key is the opcode and the entity, the field, In Agents, the entity is a context.
	\param p_args	A Tuple passed as argument to the call that is not modified. This may be a pure function in Bebop or have context
					in Agency.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Bebop::exec(pTransaction &p_txn, Locator &function, pTuple p_args) {

	return SERVICE_NOT_IMPLEMENTED;
}


/** The function call interface for **modify**: Execute and modify the argument Tuple by reference.

	\param function	Some description of a service. In general base/entity/key. In Channels the key must be empty and the entity is
					the pipeline. In Bebop, the key is the opcode and the entity, the field, In Agents, the entity is a context.
	\param p_args	In Channels: A Tuple with two items, "input" with the data passed to the service and "result" with the data returned.
					The result will be overridden in-place without any allocation.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

modify() is similar to exec(), but, rather than creating a new block with the result, it modifies the Tuple p_args.

NOTE: The http API will only call this on empty keys, but inside the algorithms, this can be supported to run anything.
*/
StatusCode Bebop::modify(Locator &function, pTuple p_args) {

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_bebop

#ifdef CATCH_TEST
#include "src/jazz_bebop/tests/test_core.ctest"
#endif

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


#include "src/jazz_elements/persisted.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Persisted : I m p l e m e n t a t i o n
--------------------------------------------------- */

Persisted::Persisted(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}

/**
//TODO: Document Persisted::start()
*/
StatusCode Persisted::start()
{
//TODO: Implement Persisted::start()

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Persisted::shut_down()
{
//TODO: Implement Persisted::shut_down()

	return SERVICE_NO_ERROR;
}


/**
//TODO: Document this.
*/
StatusCode Persisted::get (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	if (!check_block(p_txn->p_block))
		log_printf(LOG_WARN, "hash64 check failed for //%s/%s/%s", what.base, what.entity, what.key);

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::get (pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

//TODO: Implement this.

	if (!check_block(p_txn->p_block))
		log_printf(LOG_WARN, "hash64 check failed for //%s/%s/%s", what.base, what.entity, what.key);

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::get (pTransaction &p_txn, Locator &what, pChar name) {

//TODO: Implement this.

	if (!check_block(p_txn->p_block))
		log_printf(LOG_WARN, "hash64 check failed for //%s/%s/%s", what.base, what.entity, what.key);

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::header (StaticBlockHeader &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::header (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**

	\param where	Some **destination** parsed by as_locator()
	\param p_block	The Block to be stored in Persistence. The Block hash and dated will be updated by this call!!
	\param mode		Some writing restriction, either WRITE_ONLY_IF_EXISTS or WRITE_ONLY_IF_NOT_EXISTS. WRITE_TENSOR_DATA_AS_RAW returns
					the error SERVICE_ERROR_WRONG_ARGUMENTS

//TODO: Document this.

**NOTE**: This updates the calling block's creation time and hash64
*/
StatusCode Persisted::put (Locator &where, pBlock p_block, int mode) {

//TODO: Implement this.

	close_block(p_block);

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::new_entity (Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::remove (Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Persisted::copy (Locator &where, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

	The Persisted object has used-defined databases containing anything, these databases can have any names as long as they do not
interfere with existing base names. The Api object will forward names that do not match any base names to Persisted in case they
are the name of a database (and will fail otherwise).

	Besides these user-defined names, there is a number of reserved databases that keep track of objects. "sys" keeps cluster-level
config, "group" keeps track of all groups (of nodes sharing a sharded resource), "kind" the kinds, "field" the fields, etc.
"static" is a database of objects with attributes BLOCK_ATTRIB_URL and BLOCK_ATTRIB_MIMETYPE exposed via the / API.
*/
void Persisted::base_names (BaseNames &base_names)
{
	base_names["sys"]	 = this;
	base_names["group"]  = this;
	base_names["kind"]	 = this;
	base_names["field"]  = this;
	base_names["flux"]	 = this;
	base_names["agent"]	 = this;
	base_names["static"] = this;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_persisted.ctest"
#endif

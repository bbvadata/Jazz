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


#include "src/jazz_elements/volatile.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Volatile : I m p l e m e n t a t i o n
--------------------------------------------------- */

Volatile::Volatile(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


/** \brief Starts the service, checking the configuration and starting the Service.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

*/
StatusCode Volatile::start() {

//TODO: Implement Volatile::start()

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Volatile::shut_down() {

//TODO: Implement Volatile::shut_down()

	return SERVICE_NO_ERROR;
}


/** Native (Volatile) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy()-ed when the caller is done.
*/
StatusCode Volatile::get (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface **selection of rows in a Block** retrieval.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container.
	\param what			Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean of the same length as the tensor in
						p_from (or all of them if it is a Tuple) (p_row_filter->filter_type() == FILTER_TYPE_BOOLEAN) or a vector of
						integers (p_row_filter->filter_type() == FILTER_TYPE_INTEGER) in that range.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy()-ed when the caller is done.
*/
StatusCode Volatile::get (pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface **selection of a tensor in a Tuple** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)
	\param name		The name of the item to be selected.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy()-ed when the caller is done.
*/
StatusCode Volatile::get (pTransaction &p_txn, Locator &what, pChar name) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface **metadata of a Block** retrieval.

	\param hea		A StaticBlockHeader structure that will receive the metadata.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

This is a faster, not involving RAM allocation version of the other form of header. For a tensor, is will be the only thing you need, but
for a Kind or a Tuple, you probably want the types of all its items and need to pass a pTransaction to hold the data.
*/
StatusCode Volatile::header (StaticBlockHeader &hea, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface **metadata of a Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike its faster form, this allocates a Block and therefore, it is equivalent to a new_block() call. On success, it will return a
Transaction that belongs to the Container and must be destroy()-ed when the caller is done.

For Tensors it will allocate a block that only has the StaticBlockHeader (What you can more efficiently get from the other form.)
For Kinds, the metadata of all the items is exactly the same a .get() call returns.
For Tuples, it does what you expect: returning a Block with the metadata of all the items without the data.
*/
StatusCode Volatile::header (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface for **Block storing**

	\param where	Some **destination** Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)
	\param p_block	The Block to be stored in Volatile. The Block hash and dated will be updated by this call!!
	\param mode		Some writing restriction, either WRITE_ONLY_IF_EXISTS or WRITE_ONLY_IF_NOT_EXISTS. WRITE_TENSOR_DATA_AS_RAW returns
					the error SERVICE_ERROR_WRONG_ARGUMENTS

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This updates the calling block's creation time and hash64.
*/
StatusCode Volatile::put (Locator &where, pBlock p_block, int mode) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface for **creating databases**

	\param where	Some **destination** Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::new_entity (Locator &where) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface for **deleting databases and blocks**:

	\param where	The block or entity to be removed. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::remove (Locator &where) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Volatile) interface for **Block copying** (inside the Volatile).

	\param where	The block or entity to be written. (See Node Method Reference in the documentation of the class Volatile.)
	\param what		The block or entity to be read. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::copy (Locator &where, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Volatile::base_names (BaseNames &base_names) {

	base_names["deque"]	= this;		// Just a key/value store for any kind of blocks
	base_names["index"]	= this;		// A way to API-fy the IndexIS and IndexSS (Again, a block storage but only for Index*S)
	base_names["queue"] = this;		// The AA-tree priority queue with methods, possibly used instead of keys. E.g. key~_first_
	base_names["tree"]	= this;		// An API-fied tree. Can use IndexIS to return. E.g. key~_child_
}


} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_volatile.ctest"
#endif

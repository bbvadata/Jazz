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


Volatile::~Volatile() { destroy_volatile(); }


/** \brief Starts the service, checking the configuration and starting the Service.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

*/
StatusCode Volatile::start() {

	time_t rawtime;
	time(&rawtime);
	key_seed = MurmurHash64A(&rawtime, sizeof(rawtime)) & 0x0fffffFFFFFFFF;

	if (!get_conf_key("VOLATILE_MAX_TRANSACTIONS", max_transactions)) {
		log(LOG_ERROR, "Config key VOLATILE_MAX_TRANSACTIONS not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	int i = 0;

	if (!get_conf_key("VOLATILE_WARN_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key VOLATILE_WARN_BLOCK_KBYTES not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}
	warn_alloc_bytes = 1024; warn_alloc_bytes *= i;

	if (!get_conf_key("VOLATILE_ERROR_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key VOLATILE_ERROR_BLOCK_KBYTES not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}
	fail_alloc_bytes = 1024; fail_alloc_bytes *= i;

	return new_volatile();
}


/** Shuts down the Persisted Service
*/
StatusCode Volatile::shut_down() {

	return destroy_volatile();
}


/** Creates the buffers for new_transaction()/destroy_transaction()

	\return	SERVICE_NO_ERROR or SERVICE_ERROR_NO_MEM on RAM alloc failure.
*/
StatusCode Volatile::new_volatile() {

	if (p_buffer != nullptr || max_transactions <= 0) {
#if defined CATCH_TEST
		destroy_volatile();
#else
		log(LOG_ERROR, "new_volatile() called on a running Volatile().");
		return SERVICE_ERROR_STARTING;
#endif
	}

	alloc_bytes = 0;

	alloc_warning_issued = false;

	_lock_ = 0;

	p_buffer = (pVolatileTransaction) malloc(max_transactions*sizeof(VolatileTransaction));

	if (p_buffer == nullptr)
		return SERVICE_ERROR_NO_MEM;

	p_free = p_buffer;

	pVolatileTransaction pt = (pVolatileTransaction) p_buffer;

	for (int i = 1; i < max_transactions; i++) {
		pt->status = BLOCK_STATUS_DESTROYED;
		pVolatileTransaction l_pt = pt++;
		l_pt->p_next = pt;
	}
	pt->status = BLOCK_STATUS_DESTROYED;
	pt->p_next = nullptr;

	return SERVICE_NO_ERROR;
}


/** Destroys everything: all transactions and the buffer itself

	\return	SERVICE_NO_ERROR.
*/
StatusCode Volatile::destroy_volatile() {

	if (p_buffer != nullptr) {
		pVolatileTransaction pt = (pVolatileTransaction) p_buffer;

		for (int i = 0; i < max_transactions; i++) {
			if (pt->status != BLOCK_STATUS_DESTROYED) {
				pTransaction p_txn = pt;
				destroy_transaction(p_txn);
			}
			pt++;
		}
		free(p_buffer);
	}
	alloc_bytes = 0;
	p_buffer = p_free = nullptr;
	_lock_ = 0;

	return SERVICE_NO_ERROR;
}


/** Allocate a Transaction to share a block via the API.

	\param p_txn	A pointer to a valid Transaction passed by reference. On failure, it will assign nullptr to it.

	\return			SERVICE_NO_ERROR on success (and a valid p_txn), or some error.

NOTE: Volatile overrides the original virtual method from Container. This way, the original new_block() methods can be used and the
Transaction returned is actually a VolatileTransaction which is good for any of the bases (deque, queue, tree or index).
*/
StatusCode Volatile::new_transaction(pTransaction &p_txn) {

	if (alloc_bytes > warn_alloc_bytes && !alloc_warning_issued) {
		log_printf(LOG_WARN, "Service Container exceeded RAM %0.2f Mb of %0.2f Mb",
				   (double) alloc_bytes/ONE_MB, (double) warn_alloc_bytes/ONE_MB);

		alloc_warning_issued = true;
	}

	lock_container();

	if (p_free == nullptr) {
		unlock_container();
		p_txn = nullptr;

		return SERVICE_ERROR_NO_MEM;
	}

	p_txn  = p_free;
	p_free = pVolatileTransaction(p_free)->p_next;

	p_txn->p_block = nullptr;
	p_txn->status  = BLOCK_STATUS_EMPTY;
	p_txn->_lock_  = 0;
	p_txn->p_owner = this;

	unlock_container();

	return SERVICE_NO_ERROR;
}


/** Dealloc the Block in the p_tnx->p_block (if not null) and free the Transaction inside a Container.

	\param p_txn	A pointer to a valid Transaction passed by reference. Once finished, p_txn is set to nullptr to avoid reusing.

NOTE: Volatile overrides the original virtual method from Container. This way, the original new_block() methods can be used and the
Transaction returned is actually a VolatileTransaction which is good for any of the bases (deque, queue, tree or index).
*/
void Volatile::destroy_transaction  (pTransaction &p_txn) {

	if (p_txn->p_owner == nullptr) {
		log_printf(LOG_ERROR, "Transaction %p has no p_owner", p_txn);

		return;
	}
	if (p_txn->p_owner != this) {
		p_txn->p_owner->destroy_transaction(p_txn);

		return;
	}

	enter_write(p_txn);

	if (p_txn->p_block != nullptr) {
		if (p_txn->p_block->cell_type == CELL_TYPE_INDEX) {
			p_txn->p_hea->index.~map();
			alloc_bytes -= sizeof(BlockHeader);
		} else
			alloc_bytes -= p_txn->p_block->total_bytes;

		free(p_txn->p_block);

		p_txn->p_block = nullptr;
	}

	lock_container();

	pVolatileTransaction(p_txn)->p_next = (pVolatileTransaction) p_free;

	p_txn->status = BLOCK_STATUS_DESTROYED;

	p_free = p_txn;
	p_txn  = nullptr;

	unlock_container();
}


/** Native (Volatile) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Volatile::get(pTransaction &p_txn, Locator &what) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR) {
		p_txn = nullptr;

		return ret;
	}

	if (p_int_txn != nullptr) {
		if ((ret = new_transaction(p_txn)) != SERVICE_NO_ERROR)
			return ret;

		int size = p_int_txn->p_block->total_bytes;

		p_txn->p_block = block_malloc(size);

		if (p_txn->p_block == nullptr) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NO_MEM;
		}

		memcpy(p_txn->p_block, p_int_txn->p_block, size);
		p_txn->status = BLOCK_STATUS_READY;

		if (pop_ent != 0)
			destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

		return SERVICE_NO_ERROR;
	}

	if (p_str != nullptr)
		return new_block(p_txn, CELL_TYPE_STRING, nullptr, FILL_WITH_TEXTFILE, nullptr, 0, p_str->c_str(), 0);

	return new_block(p_txn, index_ent[pop_ent]->p_hea->index);
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
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Volatile::get(pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR || p_int_txn == nullptr) {
		p_txn = nullptr;

		return ret != SERVICE_NO_ERROR ? ret : SERVICE_ERROR_PARSING_COMMAND;
	}

	AttributeMap att = {};

	p_int_txn->p_block->get_attributes(&att);

	ret = new_block(p_txn, p_int_txn->p_block, p_row_filter, &att);

	if (pop_ent != 0)
		destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

	return ret;
}


/** Native (Volatile) interface **selection of a tensor in a Tuple** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)
	\param name		The name of the item to be selected.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Volatile::get(pTransaction &p_txn, Locator &what, pChar name) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR || p_int_txn == nullptr) {
		p_txn = nullptr;

		return ret != SERVICE_NO_ERROR ? ret : SERVICE_ERROR_PARSING_COMMAND;
	}

	AttributeMap att = {};

	p_int_txn->p_block->get_attributes(&att);

	ret = new_block(p_txn, (pTuple) p_int_txn->p_block, name, &att);

	if (pop_ent != 0)
		destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

	return ret;
}


/** Native (Volatile) interface: Locate a block by some relation that is understandable by the descendant.

	\param location	The solved location of the block.
	\param what		A valid reference to a block. E.g. //deque/ent/~first, //tree/ent/key~parent, //queue/ent/~highest

	\return	SERVICE_NO_ERROR on success (and a valid location), or some negative value (error).

*/
StatusCode Volatile::locate(Locator &location, Locator &what) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR)
		return ret;

	location.p_extra = nullptr;
	strcpy(location.base, what.base);
	strcpy(location.entity, what.entity);

	if (p_int_txn != nullptr) {
		strcpy(location.key, name[pVolatileTransaction(p_int_txn)->key_hash].name);

		return SERVICE_NO_ERROR;
	}

	if (p_str != nullptr) {
		strcpy(location.key, what.key);

		return SERVICE_NO_ERROR;
	}

	return SERVICE_ERROR_PARSING_COMMAND;
}


/** Native (Volatile) interface **metadata of a Block** retrieval.

	\param hea		A StaticBlockHeader structure that will receive the metadata.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

This is a faster, not involving RAM allocation version of the other form of header. For a tensor, is will be the only thing you need, but
for a Kind or a Tuple, you probably want the types of all its items and need to pass a pTransaction to hold the data.
*/
StatusCode Volatile::header(StaticBlockHeader &hea, Locator &what) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR || p_int_txn == nullptr)
		return ret != SERVICE_NO_ERROR ? ret : SERVICE_ERROR_PARSING_COMMAND;

	memcpy(&hea, p_int_txn->p_block, sizeof(StaticBlockHeader));

	if (pop_ent != 0)
		destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

	return SERVICE_NO_ERROR;
}


/** Native (Volatile) interface **metadata of a Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike its faster form, this allocates a Block and therefore, it is equivalent to a new_block() call. On success, it will return a
Transaction that belongs to the Container and must be destroy_transaction()-ed when the caller is done.

For Tensors it will allocate a block that only has the StaticBlockHeader (What you can more efficiently get from the other form.)
For Kinds, the metadata of all the items is exactly the same a .get() call returns.
For Tuples, it does what you expect: returning a Block with the metadata of all the items without the data.
*/
StatusCode Volatile::header(pTransaction &p_txn, Locator &what) {

	pTransaction p_int_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR || p_int_txn == nullptr) {
		p_txn = nullptr;

		return ret != SERVICE_NO_ERROR ? ret : SERVICE_ERROR_PARSING_COMMAND;
	}

	if ((ret = new_transaction(p_txn)) != SERVICE_NO_ERROR)
		return ret;

	int hea_size = sizeof(StaticBlockHeader);

	switch (p_int_txn->p_block->cell_type) {
	case CELL_TYPE_TUPLE_ITEM:
	case CELL_TYPE_KIND_ITEM:
		hea_size += p_int_txn->p_block->size*sizeof(ItemHeader);
	}

	p_txn->p_block = block_malloc(hea_size);

	if (p_txn->p_block == nullptr) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

	memcpy(p_txn->p_block, p_int_txn->p_block, hea_size);

	p_txn->p_block->total_bytes = hea_size;

	p_txn->status = BLOCK_STATUS_READY;

	if (pop_ent != 0)
		destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

	return SERVICE_NO_ERROR;
}


/** Native (Volatile) interface for **Block storing**

	\param where	Some **destination** Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)
	\param p_block	The Block to be stored in Volatile. The Block hash and dated will be updated by this call!!
	\param mode		Some writing restriction, either WRITE_ONLY_IF_EXISTS or WRITE_ONLY_IF_NOT_EXISTS. WRITE_TENSOR_DATA returns
					the error SERVICE_ERROR_WRONG_ARGUMENTS

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This updates the calling block's creation time and hash64.
*/
StatusCode Volatile::put(Locator &where, pBlock p_block, int mode) {

	int base;
	EntityKeyHash ek;
	HashVolXctMap::iterator	it_ent;
	HashQueueEntMap::iterator it_queue;
	ek.ent_hash = hash(where.entity);

	switch (base = TenBitsAtAddress(where.base)) {
	case BASE_DEQUE_10BIT:
		if (mode & WRITE_TENSOR_DATA)
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		it_ent = deque_ent.find(ek.ent_hash);

		if (it_ent == deque_ent.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		break;

	case BASE_INDEX_10BIT:
		if ((mode & WRITE_TENSOR_DATA) == 0)
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		it_ent = index_ent.find(ek.ent_hash);

		if (it_ent == index_ent.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		break;

	case BASE_QUEUE_10BIT:
		if (mode & WRITE_TENSOR_DATA)
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		it_queue = queue_ent.find(ek.ent_hash);

		if (it_queue == queue_ent.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		break;

	case BASE_TREE_10BIT:
		if (mode & WRITE_TENSOR_DATA)
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		it_ent = tree_ent.find(ek.ent_hash);

		if (it_ent == tree_ent.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		break;

	default:
		return SERVICE_ERROR_WRONG_BASE;
	}

	Name key, second;
	int	 command;

	if (!parse_command(key, command, second, where.key, true))
		return SERVICE_ERROR_PARSING_COMMAND;

	switch (command) {
	case COMMAND_JUST_THE_KEY: {
		if (base == BASE_INDEX_10BIT)
			return put_index(it_ent->second->p_hea->index, key, p_block, mode);

		if (base != BASE_DEQUE_10BIT)
			return SERVICE_ERROR_PARSING_COMMAND;

		ek.key_hash = hash(key);
		EntKeyVolXctMap::iterator it;

		if ((it = deque_key.find(ek)) != deque_key.end()) {
			if (mode & WRITE_ONLY_IF_NOT_EXISTS)
				return SERVICE_ERROR_WRITE_FORBIDDEN;

			return put_replace(it->second, p_block);
		}
		if (mode & WRITE_ONLY_IF_EXISTS)
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		return put_in_deque(it_ent, ek, key, p_block); }

	case COMMAND_FIRST_10BIT:
		if (base != BASE_DEQUE_10BIT)
			return SERVICE_ERROR_PARSING_COMMAND;

		new_key(key);ek.key_hash = hash(key);

		return put_in_deque(it_ent, ek, key, p_block, true);

	case COMMAND_LAST_10BIT:
		if (base != BASE_DEQUE_10BIT)
			return SERVICE_ERROR_PARSING_COMMAND;

		new_key(key);ek.key_hash = hash(key);

		return put_in_deque(it_ent, ek, key, p_block);

	case COMMAND_PUT_10BIT:
		if (base != BASE_INDEX_10BIT)
			return SERVICE_ERROR_PARSING_COMMAND;

		return populate_index(it_ent->second->p_hea->index, p_block);

	case COMMAND_SECOND_ARG:
		if (base == BASE_QUEUE_10BIT) {
			double priority;
			if (sscanf(second, "%lf", &priority) != 1)
				return SERVICE_ERROR_PARSING_COMMAND;

			put_queue_insert(it_queue, key, priority, p_block);
		}
		if (base == BASE_TREE_10BIT)
			return put_tree(ek.ent_hash, second, key, p_block);

		return SERVICE_ERROR_PARSING_COMMAND;

	default:
		break;
	}

	return SERVICE_ERROR_PARSING_COMMAND;
}


/** Native (Volatile) interface for **creating databases**

	\param where	Some **destination** Locator to the block. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::new_entity(Locator &where) {

	if (where.key[0] != 0)
		return SERVICE_ERROR_PARSING_COMMAND;

	uint64_t ent_hash = hash(where.entity);

	switch (TenBitsAtAddress(where.base)) {
	case BASE_DEQUE_10BIT:
		if (deque_ent.find(ent_hash) !=  deque_ent.end())
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		deque_ent[ent_hash] = nullptr;
		return SERVICE_NO_ERROR;

	case BASE_QUEUE_10BIT: {
		if (queue_ent.find(ent_hash) != queue_ent.end())
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		Name key, second;
		int	 command;

		if (!parse_command(key, command, second, where.key, true) || command <= COMMAND_SIZE)
			return SERVICE_ERROR_PARSING_COMMAND;

		QueueEnt queue = {command - COMMAND_SIZE, 0, nullptr};

		queue_ent[ent_hash] = queue; }
		return SERVICE_NO_ERROR;

	case BASE_TREE_10BIT:

		if (tree_ent.find(ent_hash) != tree_ent.end())
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		tree_ent[ent_hash] = nullptr;
		return SERVICE_NO_ERROR;

	case BASE_INDEX_10BIT:
		if (index_ent.find(ent_hash) != index_ent.end())
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		pTransaction p_txn;
		if (int ret = new_block(p_txn, CELL_TYPE_INDEX) != SERVICE_NO_ERROR)
			return ret;

		index_ent[ent_hash] = (pVolatileTransaction) p_txn;
		return SERVICE_NO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Native (Volatile) interface for **deleting databases and blocks**:

	\param where	The block or entity to be removed. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::remove(Locator &where) {

	int base = TenBitsAtAddress(where.base);

	EntityKeyHash ek;
	ek.ent_hash = hash(where.entity);

	if (where.key[0] == 0) {
		switch (base) {
		case BASE_DEQUE_10BIT: {
			HashVolXctMap::iterator it;

			if ((it = deque_ent.find(ek.ent_hash)) == deque_ent.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			pVolatileTransaction p_item = it->second, p_next;

			while (p_item != nullptr) {
				p_next = p_item->p_next == p_item ? nullptr : p_item->p_next;
				destroy_item(BASE_DEQUE_10BIT, ek.ent_hash, p_item);
				p_item = p_next;
			}
			deque_ent.erase(it); }
			break;

		case BASE_QUEUE_10BIT: {
			HashQueueEntMap::iterator it;
			if ((it = queue_ent.find(ek.ent_hash)) == queue_ent.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			destroy_queue(ek.ent_hash, it->second.p_root);

			queue_ent.erase(it); }
			break;

		case BASE_TREE_10BIT: {
			if (tree_ent.find(ek.ent_hash) == tree_ent.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			remove_tree(ek.ent_hash); }
			break;

		case BASE_INDEX_10BIT: {
			HashVolXctMap::iterator it;

			if ((it = index_ent.find(ek.ent_hash)) == index_ent.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			pTransaction p_txn = index_ent[ek.ent_hash];
			destroy_transaction(p_txn);

			index_ent.erase(it); }
			break;

		default:
			return SERVICE_ERROR_WRONG_BASE;
		}
		return SERVICE_NO_ERROR;
	}

	Name key, second;
	int	 command;

	if (!parse_command(key, command, second, where.key, true) || command != COMMAND_JUST_THE_KEY)
		return SERVICE_ERROR_PARSING_COMMAND;

	ek.key_hash = hash(key);

	EntKeyVolXctMap::iterator it_key;

	switch (base) {
	case BASE_DEQUE_10BIT: {
		if ((it_key = deque_key.find(ek)) == deque_key.end())
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		destroy_item(base, ek.ent_hash, it_key->second); }
		break;

	case BASE_QUEUE_10BIT: {
		if ((it_key = queue_key.find(ek)) == queue_key.end())
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		destroy_item(base, ek.ent_hash, it_key->second); }
		break;

	case BASE_TREE_10BIT: {
		if ((it_key = tree_key.find(ek)) == tree_key.end())
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		destroy_item(base, ek.ent_hash, it_key->second); }
		break;

	case BASE_INDEX_10BIT: {
		HashVolXctMap::iterator it_ent = index_ent.find(ek.ent_hash);
		if (it_ent == index_ent.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		Index::iterator it_itm = it_ent->second->p_hea->index.find(key);

		if (it_itm == it_ent->second->p_hea->index.end())
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		it_ent->second->p_hea->index.erase(it_itm); }
		break;

	default:
		return SERVICE_ERROR_WRONG_BASE;
	}
	return SERVICE_NO_ERROR;
}


/** Native (Volatile) interface for **Block copying** (inside the Volatile).

	\param where	The block or entity to be written. (See Node Method Reference in the documentation of the class Volatile.)
	\param what		The block or entity to be read. (See Node Method Reference in the documentation of the class Volatile.)

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Volatile::copy(Locator &where, Locator &what) {

	pTransaction p_int_txn, p_txn;
	pString		 p_str;
	uint64_t	 pop_ent;
	StatusCode	 ret;

	if ((ret = internal_get(p_int_txn, p_str, pop_ent, what)) != SERVICE_NO_ERROR)
		return ret;

	if (p_int_txn != nullptr) {
		ret = put(where, p_int_txn->p_block);

		if (pop_ent != 0)
			destroy_item(TenBitsAtAddress(what.base), pop_ent, (pVolatileTransaction) p_int_txn);

		return ret;
	}

	if (p_str != nullptr) {
		if ((ret = new_block(p_txn, CELL_TYPE_STRING, nullptr, FILL_WITH_TEXTFILE, nullptr, 0, p_str->c_str(), 0)) != SERVICE_NO_ERROR)
			return ret;

		ret = put(where, p_txn->p_block, WRITE_TENSOR_DATA);

		destroy_transaction(p_txn);

		return ret;
	}

	if ((ret = new_block(p_txn, index_ent[pop_ent]->p_hea->index)) != SERVICE_NO_ERROR)
		return ret;

	ret = put(where, p_txn->p_block);

	destroy_transaction(p_txn);

	return ret;
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Volatile::base_names(BaseNames &base_names) {

	base_names["deque"]	= this;		// Just a key/value store for any kind of blocks
	base_names["index"]	= this;		// A way to APIfy and serialize Tuple <-> IndexXX (Again, a block storage but only for Index*S)
	base_names["queue"] = this;		// The AA-tree priority queue with methods, possibly used instead of keys. E.g. key~_first_
	base_names["tree"]	= this;		// An APIfied tree. Can use IndexIS to return. E.g. key~_child_
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_volatile.ctest"
#endif

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


#include <sys/stat.h>


#include "src/jazz_elements/persisted.h"


namespace jazz_elements
{

#define INVALID_MDB_DBI			0xefefEFEF		// A constant to flag invalid MDB_dbi handle values.

/*	-----------------------------------------------
	 Persisted : I m p l e m e n t a t i o n
--------------------------------------------------- */

Persisted::Persisted(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


Persisted::~Persisted() { destroy_container(); }


/** \brief Starts the service, checking the environment and building the databases.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

	This service initialization checks configuration values related with persistence and starts LMDB with configured values.
*/
StatusCode Persisted::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	if (lmdb_env != nullptr) {
		log(LOG_ERROR, "Persisted::start() failed: nested start() call.");

		return SERVICE_ERROR_STARTING;
	}

	std::string db_path;

	bool ok = get_conf_key("MDB_PERSISTENCE_PATH", db_path);

	if (!ok || db_path.length() > MAX_LMDB_HOME_LEN - 1) {
		log(LOG_ERROR, "Persisted::start() failed. Missing or invalid MDB_PERSISTENCE_PATH.");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	int fixedmap, writemap, nometasync, nosync, mapasync, nolock, noreadahead, nomeminit;

	ok =	get_conf_key("MDB_ENV_SET_MAPSIZE",	   lmdb_opt.env_set_mapsize)
		 && get_conf_key("MDB_ENV_SET_MAXREADERS", lmdb_opt.env_set_maxreaders)
		 && get_conf_key("MDB_ENV_SET_MAXDBS",	   lmdb_opt.env_set_maxdbs)
		 && get_conf_key("MDB_FIXEDMAP",		   fixedmap)
		 && get_conf_key("MDB_WRITEMAP",		   writemap)
		 && get_conf_key("MDB_NOMETASYNC",		   nometasync)
		 && get_conf_key("MDB_NOSYNC",			   nosync)
		 && get_conf_key("MDB_MAPASYNC",		   mapasync)
		 && get_conf_key("MDB_NOLOCK",			   nolock)
		 && get_conf_key("MDB_NOREADAHEAD",		   noreadahead)
		 && get_conf_key("MDB_NOMEMINIT",		   nomeminit);

	if (!ok) {
		log(LOG_ERROR, "Persisted::start() failed. Invalid MHD_* config (integer values).");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	ok = ((fixedmap | writemap | nometasync | nosync | mapasync | nolock | noreadahead | nomeminit) & 0xfffffffe) == 0;

	if (!ok) {
		log(LOG_ERROR, "Persisted::start() failed. Flags must be 0 or 1.");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	lmdb_opt.flags =  MDB_FIXEDMAP*fixedmap
					+ MDB_WRITEMAP*writemap
					+ MDB_NOMETASYNC*nometasync
					+ MDB_NOSYNC*nosync
					+ MDB_MAPASYNC*mapasync
					+ MDB_NOLOCK*nolock
					+ MDB_NORDAHEAD*noreadahead
					+ MDB_NOMEMINIT*nomeminit;

	if (lmdb_opt.env_set_maxdbs > MAX_POSSIBLE_SOURCES) {
		log(LOG_ERROR, "Persisted::start() failed. The number of databases cannot exceed MAX_POSSIBLE_SOURCES");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	strcpy(lmdb_opt.path, db_path.c_str());

	struct stat st = {0};
	if (stat(lmdb_opt.path, &st) != 0) {
		log_printf(LOG_INFO, "Path \"%s\" does not exist, creating it.", db_path.c_str());

		mkdir(lmdb_opt.path, 0700);
	}

	log(LOG_INFO, "Creating an LMDB environment.");

	if (mdb_env_create(&lmdb_env) != MDB_SUCCESS) {
		log(LOG_ERROR, "Persisted::start() failed: mdb_env_create() failed.");

		return SERVICE_ERROR_STARTING;
	}

	if (mdb_env_set_maxreaders(lmdb_env, lmdb_opt.env_set_maxreaders) != MDB_SUCCESS) {
		log(LOG_ERROR, "Persisted::start() failed: mdb_env_set_maxreaders() failed.");

		return SERVICE_ERROR_STARTING;
	}

	if (mdb_env_set_maxdbs(lmdb_env, lmdb_opt.env_set_maxdbs) != MDB_SUCCESS) {
		log(LOG_ERROR, "Persisted::start() failed: mdb_env_set_maxdbs() failed.");

		return SERVICE_ERROR_STARTING;
	}

	if (mdb_env_set_mapsize(lmdb_env, ((mdb_size_t)1024)*1024*lmdb_opt.env_set_mapsize) != MDB_SUCCESS) {
		log(LOG_ERROR, "Persisted::start() failed: mdb_env_set_mapsize() failed.");

		return SERVICE_ERROR_STARTING;
	}

	log_printf(LOG_INFO, "Opening LMDB environment at : \"%s\"", lmdb_opt.path);

	if (ret = mdb_env_open(lmdb_env, lmdb_opt.path, lmdb_opt.flags, LMDB_UNIX_FILE_PERMISSIONS) != MDB_SUCCESS) {
		log_lmdb_err(ret, "Persisted::start() failed: mdb_env_open() failed.");

		return SERVICE_ERROR_STARTING;
	}

	if (!open_all_databases()) {
		log(LOG_ERROR, "Persisted::start() failed: open_all_sources() failed.");

		return SERVICE_ERROR_STARTING;
	}

	return SERVICE_NO_ERROR;
}


/** \brief Shuts down the Persisted Service

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

*/
StatusCode Persisted::shut_down() {

	if (lmdb_env != nullptr) {
		log(LOG_INFO, "Closing all LMDB databases.");

		close_all_databases();

		log(LOG_INFO, "Flushing LMDB environment.");
		mdb_env_sync(lmdb_env, true);

		log(LOG_INFO, "Closing LMDB environment.");
		mdb_env_close(lmdb_env);

		log(LOG_INFO, "Persisted stopped.");

		lmdb_env = nullptr;
	}

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** Native (Persistence) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. E.g. //lmdb/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Persisted::get(pTransaction &p_txn, Locator &what) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr) {
		p_txn = nullptr;

		return SERVICE_ERROR_BLOCK_NOT_FOUND;
	}

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR) {
		done_pointer_to_block(p_l_txn);

		return ret;
	}

	p_txn->p_block = block_malloc(p_blx->total_bytes);

	if (p_txn->p_block == nullptr) {
		done_pointer_to_block(p_l_txn);
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

	memcpy(p_txn->p_block, p_blx, p_blx->total_bytes);

	done_pointer_to_block(p_l_txn);

	p_txn->status = BLOCK_STATUS_READY;

	if (!check_block(p_txn->p_block))
		log_printf(LOG_WARN, "hash64 check failed for //%s/%s/%s", what.base, what.entity, what.key);

	return SERVICE_NO_ERROR;
}


/** Native (Persistence) interface **selection of rows in a Block** retrieval.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container.
	\param what			Some Locator to the block. E.g. //lmdb/entity/key
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean of the same length as the tensor in
						p_from (or all of them if it is a Tuple) (p_row_filter->filter_type() == FILTER_TYPE_BOOLEAN) or a vector of
						integers (p_row_filter->filter_type() == FILTER_TYPE_INTEGER) in that range.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Persisted::get(pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr) {
		p_txn = nullptr;

		return SERVICE_ERROR_BLOCK_NOT_FOUND;
	}

	StatusCode ret = new_block(p_txn, p_blx, p_row_filter);

	done_pointer_to_block(p_l_txn);

	return ret;
}


/** Native (Persistence) interface **selection of a tensor in a Tuple** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. E.g. //lmdb/entity/key
	\param name		The name of the item to be selected.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Persisted::get(pTransaction &p_txn, Locator &what, pChar name) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr) {
		p_txn = nullptr;

		return SERVICE_ERROR_BLOCK_NOT_FOUND;
	}

	StatusCode ret = new_block(p_txn, p_blx, name);

	done_pointer_to_block(p_l_txn);

	return ret;
}


/** Native (Persistence) interface **metadata of a Block** retrieval.

	\param hea		A StaticBlockHeader structure that will receive the metadata.
	\param what		Some Locator to the block. E.g. //lmdb/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

This is a faster, not involving RAM allocation version of the other form of header. For a tensor, is will be the only thing you need, but
for a Kind or a Tuple, you probably want the types of all its items and need to pass a pTransaction to hold the data.
*/
StatusCode Persisted::header(StaticBlockHeader &hea, Locator &what) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr)
		return SERVICE_ERROR_BLOCK_NOT_FOUND;

	memcpy(&hea, p_blx, sizeof(StaticBlockHeader));

	done_pointer_to_block(p_l_txn);

	return SERVICE_NO_ERROR;
}


/** Native (Persistence) interface **metadata of a Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some Locator to the block. E.g. //lmdb/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike its faster form, this allocates a Block and therefore, it is equivalent to a new_block() call. On success, it will return a
Transaction that belongs to the Container and must be destroy_transaction()-ed when the caller is done.

For Tensors it will allocate a block that only has the StaticBlockHeader (What you can more efficiently get from the other form.)
For Kinds, the metadata of all the items is exactly the same a .get() call returns.
For Tuples, it does what you expect: returning a Block with the metadata of all the items without the data.
*/
StatusCode Persisted::header(pTransaction &p_txn, Locator &what) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr) {
		p_txn = nullptr;

		return SERVICE_ERROR_BLOCK_NOT_FOUND;
	}

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR) {
		done_pointer_to_block(p_l_txn);

		return ret;
	}

	int hea_size = sizeof(StaticBlockHeader);

	switch (p_blx->cell_type) {
	case CELL_TYPE_TUPLE_ITEM:
	case CELL_TYPE_KIND_ITEM:
		hea_size += p_blx->size*sizeof(ItemHeader);
	}

	p_txn->p_block = block_malloc(hea_size);

	if (p_txn->p_block == nullptr) {
		done_pointer_to_block(p_l_txn);
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

	memcpy(p_txn->p_block, p_blx, hea_size);

	p_txn->p_block->total_bytes = hea_size;

	p_txn->status = BLOCK_STATUS_READY;

	done_pointer_to_block(p_l_txn);

	return SERVICE_NO_ERROR;
}


/** Native (Persistence) interface for **Block storing**

	\param where	Some **destination** parsed by as_locator()
	\param p_block	The Block to be stored in Persistence. The Block hash and dated will be updated by this call!!
	\param mode		Some writing restriction, either WRITE_ONLY_IF_EXISTS or WRITE_ONLY_IF_NOT_EXISTS. WRITE_TENSOR_DATA returns
					the error SERVICE_ERROR_WRONG_ARGUMENTS

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This updates the calling block's creation time and hash64.
*/
StatusCode Persisted::put(Locator &where, pBlock p_block, int mode) {

	pMDB_txn p_txn;

	if (mode != WRITE_EVERYTHING) {
		if (mode & WRITE_TENSOR_DATA)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		pBlock p_blx = lock_pointer_to_block(where, p_txn);

		bool already_exists = p_blx != nullptr;

		if (already_exists) {
			done_pointer_to_block(p_txn);

			if (mode & WRITE_ONLY_IF_NOT_EXISTS)
				return SERVICE_ERROR_WRITE_FORBIDDEN;

		} else if (mode & WRITE_ONLY_IF_EXISTS)
			return SERVICE_ERROR_WRITE_FORBIDDEN;
	}

	p_block->close_block();

	DBImap::iterator it = source_dbi.find(where.entity);

	if (it == source_dbi.end()) {
		log(LOG_MISS, "Invalid source in Persisted::put().");

		return SERVICE_ERROR_WRITE_FAILED;
	}

	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &p_txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::put().");

		return SERVICE_ERROR_WRITE_FAILED;
	}

	MDB_dbi hh = it->second;

	if (hh == INVALID_MDB_DBI) {
		if (int err = mdb_dbi_open(p_txn, where.entity, MDB_CREATE, &hh)) {
			log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::put().");

			goto release_txn_and_fail;
		}

		source_dbi [where.entity] = hh;
	}

	MDB_val l_key, l_data;

	l_key.mv_size  = strlen(where.key);
	l_key.mv_data  = (void *) &where.key;
	l_data.mv_size = p_block->total_bytes;
	l_data.mv_data = p_block;

	if (int err = mdb_put(p_txn, hh, &l_key, &l_data, 0)) {
		log_lmdb_err(err, "mdb_put() failed in Persisted::put().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(p_txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::put().");

		goto release_txn_and_fail;
	}

	return SERVICE_NO_ERROR;

release_txn_and_fail:

	mdb_txn_abort(p_txn);

	return SERVICE_ERROR_WRITE_FAILED;
}


/** Native (Persistence) interface for **creating databases**

	\param where	The name of the LMDB database to be created. E.g. //lmdb/entity compiled.

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Persisted::new_entity(Locator &where) {

	if (where.key[0] != 0)
		return SERVICE_ERROR_PARSING_COMMAND;

	return new_database(where.entity);
}


/** Native (Persistence) interface for **deleting databases and blocks**:

	\param where	Some Locator to the block or database to be removed. E.g. //lmdb/entity/key or //lmdb/entity compiled.

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Persisted::remove(Locator &where) {

	if (where.key[0] == 0)
		return remove_database(where.entity);

	DBImap::iterator it = source_dbi.find(where.entity);

	if (it == source_dbi.end()) {
		log(LOG_MISS, "Invalid source in Persisted::remove().");

		return SERVICE_ERROR_REMOVE_FAILED;
	}

	pMDB_txn p_txn;

	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &p_txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::remove().");

		return SERVICE_ERROR_REMOVE_FAILED;
	}

	MDB_dbi hh = it->second;

	if (hh == INVALID_MDB_DBI) {

		if (int err = mdb_dbi_open(p_txn, where.entity, MDB_CREATE, &hh)) {
			log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::remove().");

			goto release_txn_and_fail;
		}

		source_dbi [where.entity] = hh;
	}

	MDB_val l_key;

	l_key.mv_size = strlen(where.key);
	l_key.mv_data = (void *) &where.key;

	if (int err = mdb_del(p_txn, hh, &l_key, NULL)) {
		log_lmdb_err(err, "mdb_del() failed in Persisted::remove().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(p_txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::remove().");

		goto release_txn_and_fail;
	}

	return SERVICE_NO_ERROR;

release_txn_and_fail:

	mdb_txn_abort(p_txn);

	return SERVICE_ERROR_REMOVE_FAILED;
}


/** Native (Persistence) interface for **Block copying** (inside the Persistence).

	\param where	Some **destination** parsed by as_locator()
	\param what		Some Locator to the block. E.g. //lmdb/entity/key

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Persisted::copy(Locator &where, Locator &what) {

	pMDB_txn p_l_txn;

	pBlock p_blx = lock_pointer_to_block(what, p_l_txn);

	if (p_blx == nullptr)
		return SERVICE_ERROR_BLOCK_NOT_FOUND;

	StatusCode ret = put(where, p_blx);

	done_pointer_to_block(p_l_txn);

	return ret;
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
void Persisted::base_names(BaseNames &base_names) {

	base_names["lmdb"] = this;		// Provides LMDB block persistence for any movable (Tensor, Kind or Tuple).
}


/** \brief Check the internal std::map to see if a (dbi) database name exists.

	\param dbi_name	The location of a Block inside LMDB.

	\return	True if exists.
*/
bool Persisted::dbi_exists(Name dbi_name) {

	return source_dbi.find(dbi_name) != source_dbi.end();
}


/** \brief Locates a block doing an mdb_get() leaving the transaction open.

	\param what	 The location of a Block inside LMDB.
	\param p_txn the transcation created by a lock_pointer_to_block() call.

	\return	The pointer to the block (can **only** be read and **requires** a done_pointer_to_block() to close the transaction) or
			nullptr (+ log INFO) on error.

NOTE: This requires a subsequent done_pointer_to_block() call.
*/
pBlock Persisted::lock_pointer_to_block(Locator &what, pMDB_txn &p_txn) {

	DBImap::iterator it = source_dbi.find(what.entity);

	if (it == source_dbi.end()) {
		log(LOG_MISS, "Invalid source in Persisted::lock_pointer_to_block().");

		return nullptr;
	}

	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &p_txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::lock_pointer_to_block().");

		return nullptr;
	}

	MDB_dbi hh = it->second;

	if (hh == INVALID_MDB_DBI) {

		if (int err = mdb_dbi_open(p_txn, what.entity, MDB_CREATE, &hh)) {
			log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::lock_pointer_to_block().");

			goto release_txn_and_fail;
		}

		source_dbi [what.entity] = hh;
	}

	MDB_val l_key, l_data;

	l_key.mv_size = strlen(what.key);
	l_key.mv_data = (void *) &what.key;

	if (int err = mdb_get(p_txn, hh, &l_key, &l_data)) {

#ifndef DEBUG
		if (err == MDB_NOTFOUND)
			goto release_txn_and_fail; // Not found does not log anything
#endif
		log_lmdb_err(err, "mdb_get() failed in Persisted::lock_pointer_to_block() with a code other than MDB_NOTFOUND.");

		goto release_txn_and_fail;
	}

	return (pBlock) l_data.mv_data;


release_txn_and_fail:

	mdb_txn_abort(p_txn);

	p_txn = nullptr;

	return nullptr;
}


/** \brief Completes the transaction started by lock_pointer_to_block() doing a mdb_txn_commit() which invalidates the pointer.

	\param p_txn the transcation created by a lock_pointer_to_block() call.
*/
void Persisted::done_pointer_to_block(pMDB_txn &p_txn) {

	if (int err = mdb_txn_commit(p_txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::done_pointer_to_block().");

		mdb_txn_abort(p_txn);
	}

	p_txn = nullptr;
}


/** Locate all the named databases in the current LMDB environment, add them to the source[] vector and open them all for reading.

	\return true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool Persisted::open_all_databases() {

	lock_container();

	MDB_txn *txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::open_all_databases().");

		goto release_lock_and_fail;
	}

	MDB_dbi dbi;
	if (int err = mdb_dbi_open(txn, NULL, 0, &dbi)) {
		log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::open_all_databases().");

		goto release_txn_and_fail;
	}

	MDB_cursor *cursor;
	if (int err = mdb_cursor_open(txn, dbi, &cursor)) {
		log_lmdb_err(err, "mdb_cursor_open() failed in Persisted::open_all_databases().");

		goto release_dbi_and_fail;
	}

	MDB_val key, data;
	while (!mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) {
		std::string name((pChar) key.mv_data);

		source_dbi[name] = INVALID_MDB_DBI;
	}

	mdb_cursor_close(cursor);
	mdb_dbi_close(lmdb_env, dbi);

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::open_all_databases().");

		goto release_txn_and_fail;
	}

	unlock_container();

	return true;

release_dbi_and_fail:

	mdb_dbi_close(lmdb_env, dbi);

release_txn_and_fail:

	mdb_txn_abort(txn);

release_lock_and_fail:

	unlock_container();

	return false;
}


/** Close all named databases on LMDB leaving them ready for a subsequent opening.

	This makes numsources == 0 by closing used LMDB handles via mdb_dbi_close().
*/
void Persisted::close_all_databases() {

	lock_container();

	for (DBImap::iterator it = source_dbi.begin(); it != source_dbi.end(); ++it)
		if (it->second != INVALID_MDB_DBI)
			mdb_dbi_close(lmdb_env, it->second);

	source_dbi.clear();

	mdb_env_sync(lmdb_env, true);

	unlock_container();
}


/** Create a new LMDB named database and add it as a new source to the source_dbi[] map.

	\param name The name of the source to be added.

	\return	SERVICE_NO_ERROR on success or some negative value and log(LOG_MISS, "further details") on failure.
*/
StatusCode Persisted::new_database(pChar name) {

	char key [] = {"."};
	int	 val	= 0xbadF00D, ret;

	lock_container();

	if (source_dbi.size() >= MAX_POSSIBLE_SOURCES) {
		log(LOG_MISS, "Persisted::new_database(): too many sources.");

		ret = SERVICE_ERROR_TOO_MANY_ENTITIES;

		goto release_lock_and_fail;
	}

	if (source_dbi.find(name) != source_dbi.end()) {
		log(LOG_MISS, "Persisted::new_database(): source already exists.");

		ret = SERVICE_ERROR_WRITE_FORBIDDEN;

		goto release_lock_and_fail;
	}

	MDB_txn *txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::new_database().");

		ret = SERVICE_ERROR_CREATE_FAILED;

		goto release_lock_and_fail;
	}

	MDB_dbi hh;

	if (int err = mdb_dbi_open(txn, name, MDB_CREATE, &hh)) {
		log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::new_database().");

		goto release_txn_and_fail;
	}

	MDB_val l_key, l_data;

	l_key.mv_size  = 1;
	l_key.mv_data  = &key;
	l_data.mv_size = sizeof(int);
	l_data.mv_data = &val;

	if (int err = mdb_put(txn, hh, &l_key, &l_data, 0)) {
		log_lmdb_err(err, "mdb_put() failed in Persisted::new_database().");

		goto release_dbi_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::new_database().");

		goto release_dbi_and_fail;
	}

	source_dbi [name] = hh;

	unlock_container();

	return SERVICE_NO_ERROR;

release_dbi_and_fail:

	mdb_dbi_close(lmdb_env, hh);

release_txn_and_fail:

	mdb_txn_abort(txn);

	ret = SERVICE_ERROR_CREATE_FAILED;

release_lock_and_fail:

	unlock_container();

	return ret;
}


/** Kill a source, both from the LMDB persistence and from the source[] vector.
	\param name The name of the source to be killed.

	\return	SERVICE_NO_ERROR on success or some negative value and log(LOG_MISS, "further details") on failure.

	NOTE: kill_source() is EXTREMELY not thread safe! Indices to ALL sources may change. Unsafe use of: numsources, source_nam, source_open,
source_dbi.
*/
StatusCode Persisted::remove_database(pChar name) {

	if (source_dbi.find(name) == source_dbi.end()) {
		log(LOG_MISS, "Persisted::remove_database(): source does not exist.");

		return SERVICE_ERROR_ENTITY_NOT_FOUND;
	}

	lock_container();

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err(err, "mdb_txn_begin() failed in Persisted::remove_database().");

		goto release_lock_and_fail;
	}

	if (source_dbi[name] == INVALID_MDB_DBI) {
		MDB_dbi hh;

		if (int err = mdb_dbi_open(txn, name, MDB_CREATE, &hh)) {
			log_lmdb_err(err, "mdb_dbi_open() failed in Persisted::remove_database().");

			goto release_txn_and_fail;
		}

		source_dbi [name] = hh;
	}

	if (int err = mdb_drop(txn, source_dbi[name], 1)) {
		log_lmdb_err(err, "mdb_drop() failed in Persisted::remove_database().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err(err, "mdb_txn_commit() failed in Persisted::remove_database().");

		goto release_txn_and_fail;
	}

	source_dbi.erase(name);

	unlock_container();

	return SERVICE_NO_ERROR;

release_txn_and_fail:

	mdb_txn_abort(txn);

release_lock_and_fail:

	unlock_container();

	return SERVICE_ERROR_REMOVE_FAILED;
}


/** \brief A nicer presentation for LMDB error messages.
*/
void Persisted::log_lmdb_err(int err, const char * msg) {

	char errmsg [128];

	switch (err) {
	case MDB_KEYEXIST:
		strcpy(errmsg, "LMDB MDB_KEYEXIST: Key/data pair already exists.");
		break;
	case MDB_NOTFOUND:
		strcpy(errmsg, "LMDB MDB_NOTFOUND: Key/data pair not found (EOF).");
		break;
	case MDB_PAGE_NOTFOUND:
		strcpy(errmsg, "LMDB MDB_PAGE_NOTFOUND: Requested page not found - this usually indicates corruption.");
		break;
	case MDB_CORRUPTED:
		strcpy(errmsg, "LMDB MDB_CORRUPTED: Located page was wrong type.");
		break;
	case MDB_PANIC:
		strcpy(errmsg, "LMDB MDB_PANIC: Update of meta page failed or environment had a fatal error.");
		break;
	case MDB_VERSION_MISMATCH:
		strcpy(errmsg, "LMDB MDB_VERSION_MISMATCH: Environment version mismatch.");
		break;
	case MDB_INVALID:
		strcpy(errmsg, "LMDB MDB_INVALID: File is not a valid LMDB file.");
		break;
	case MDB_MAP_FULL:
		strcpy(errmsg, "LMDB MDB_MAP_FULL: Environment mapsize reached.");
		break;
	case MDB_DBS_FULL:
		strcpy(errmsg, "LMDB MDB_DBS_FULL: Environment maxdbs reached.");
		break;
	case MDB_READERS_FULL:
		strcpy(errmsg, "LMDB MDB_READERS_FULL: Environment maxreaders reached.");
		break;
	case MDB_TLS_FULL:
		strcpy(errmsg, "LMDB MDB_TLS_FULL: Too many TLS keys in use - Windows only.");
		break;
	case MDB_TXN_FULL:
		strcpy(errmsg, "LMDB MDB_TXN_FULL: Txn has too many dirty pages.");
		break;
	case MDB_CURSOR_FULL:
		strcpy(errmsg, "LMDB MDB_CURSOR_FULL: Cursor stack too deep - internal error.");
		break;
	case MDB_PAGE_FULL:
		strcpy(errmsg, "LMDB MDB_PAGE_FULL: Page has not enough space - internal error.");
		break;
	case MDB_MAP_RESIZED:
		strcpy(errmsg, "LMDB MDB_MAP_RESIZED: Database contents grew beyond environment mapsize.");
		break;
	case MDB_INCOMPATIBLE:
		strcpy(errmsg, "LMDB MDB_INCOMPATIBLE: Operation and DB incompatible, or DB type changed (see doc).");
		break;
	case MDB_BAD_RSLOT:
		strcpy(errmsg, "LMDB MDB_BAD_RSLOT: Invalid reuse of reader locktable slot.");
		break;
	case MDB_BAD_TXN:
		strcpy(errmsg, "LMDB MDB_BAD_TXN: Transaction must abort, has a child, or is invalid.");
		break;
	case MDB_BAD_VALSIZE:
		strcpy(errmsg, "LMDB MDB_BAD_VALSIZE: Unsupported size of key/DB name/data, or wrong DUPFIXED size.");
		break;
	case MDB_BAD_DBI:
		strcpy(errmsg, "LMDB MDB_BAD_DBI: The specified DBI was changed unexpectedly.");
		break;
	case MDB_PROBLEM:
		strcpy(errmsg, "LMDB MDB_PROBLEM: Unexpected problem - txn should abort.");
		break;
	case EACCES:
		strcpy(errmsg, "LMDB EACCES: An attempt was made to write in a read-only transaction.");
		break;
	case EINVAL:
		strcpy(errmsg, "LMDB EINVAL: An invalid parameter was specified.");
		break;
	case EIO:
		strcpy(errmsg, "LMDB EIO: A low-level I/O error occurred while writing.");
		break;
	case ENOSPC:
		strcpy(errmsg, "LMDB ENOSPC: No more disk space.");
		break;
	case ENOMEM:
		strcpy(errmsg, "LMDB ENOMEM: Out of memory.");
		break;
	default:
		sprintf(errmsg,"LMDB Unknown code %d.", err);
	}

	log(LOG_ERROR, errmsg);
	log(LOG_MISS, msg);
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_persisted.ctest"
#endif

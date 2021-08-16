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


/** \brief Starts the service, checking the environment and building the databases.

	\return SERVICE_NO_ERROR if successful, some error and log(LOG_MISS, "further details") if not.

	This service initialization checks configuration values related with persistence and starts LMDB with configured values.
*/
StatusCode Persisted::start()
{
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

	if (int ret = mdb_env_open(lmdb_env, lmdb_opt.path, lmdb_opt.flags, LMDB_UNIX_FILE_PERMISSIONS) != MDB_SUCCESS) {
		log_lmdb_err(ret, "Persisted::start() failed: mdb_env_open() failed.");

		return SERVICE_ERROR_STARTING;
	}

	if (!open_all_databases()) {
		log(LOG_ERROR, "Persisted::start() failed: open_all_sources() failed.");

		return SERVICE_ERROR_STARTING;
	}

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

	if (mode & WRITE_TENSOR_DATA_AS_RAW)
		return SERVICE_ERROR_WRONG_ARGUMENTS;

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

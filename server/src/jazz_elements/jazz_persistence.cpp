/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Also includes LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

   Licensed under http://www.OpenLDAP.org/license.html

   Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <sys/stat.h>

#include "src/jazz_elements/jazz_persistence.h"

namespace jazz_persistence
{

#define MAX_POSSIBLE_SOURCES		 32		///< The number of databases is configurable via MDB_ENV_SET_MAXDBS, but cannot exceed this.
#define MAX_LMDB_HOME_LEN			128		///< The number of char for the LMDB home path
#define LMDB_UNIX_FILE_PERMISSIONS 0664		///< The file permissions (as in chmod) for the database files


/** All the necessary LMDB options (a binary representation of the values in the config file)
*/
struct JazzLmdbOptions
{
	char path[MAX_LMDB_HOME_LEN];

	int env_set_mapsize,
		env_set_maxreaders,
		env_set_maxdbs,
		flags;
};


/**
//TODO: Document JazzPersistence constructor
*/
JazzPersistence::JazzPersistence(jazz_utils::pJazzLogger	 a_logger,
								 jazz_utils::pJazzConfigFile a_config)	: JazzBlockKeepr(a_logger, a_config)
{
//TODO: Implement JazzPersistence constructor
}


/**
//TODO: Document JazzSource constructor
*/
JazzSource::JazzSource(jazz_utils::pJazzLogger	   a_logger,
					   jazz_utils::pJazzConfigFile a_config)	: JazzPersistence(a_logger, a_config)
{
//TODO: Implement JazzSource constructor
}


/**
//TODO: Document ~JazzSource
*/
JazzSource::~JazzSource()
{
//TODO: Implement ~JazzSource
}


/**
//TODO: Document destroy_keeprs
*/
void JazzSource::destroy_keeprs()
{
//TODO: Implement destroy_keeprs
}


/** JazzSource::StartService

	\return JAZZ_API_NO_ERROR if successful, JAZZ_API_ERROR_xxx and log(LOG_MISS, "further details") if not.

	This service initialization checks configuration values related with persistence and starts lmdb with configured values.
*/
API_ErrorCode JazzSource::StartService ()
{
	JazzLmdbOptions lmdb_opt;
	int fixedmap, writemap, nometasync, nosync, mapasync, nolock, noreadahead, nomeminit;

	bool ok =	p_config->get_key("MDB_ENV_SET_MAPSIZE",	lmdb_opt.env_set_mapsize)
			  & p_config->get_key("MDB_ENV_SET_MAXREADERS", lmdb_opt.env_set_maxreaders)
			  & p_config->get_key("MDB_ENV_SET_MAXDBS",		lmdb_opt.env_set_maxdbs)
			  & p_config->get_key("MDB_FIXEDMAP",			fixedmap)
			  & p_config->get_key("MDB_WRITEMAP",			writemap)
			  & p_config->get_key("MDB_NOMETASYNC",			nometasync)
			  & p_config->get_key("MDB_NOSYNC",				nosync)
			  & p_config->get_key("MDB_MAPASYNC",			mapasync)
			  & p_config->get_key("MDB_NOLOCK",				nolock)
			  & p_config->get_key("MDB_NOREADAHEAD",		noreadahead)
			  & p_config->get_key("MDB_NOMEMINIT",			nomeminit);

	if (!ok) {
		log(LOG_MISS, "JazzSource::StartService() failed. Invalid MHD_* config (integer values).");

		return JAZZ_API_ERROR_PARSING_CONFIG;
	}

	ok = ((fixedmap | writemap | nometasync | nosync | mapasync | nolock | noreadahead | nomeminit) & 0xfffffffe) == 0;

	if (!ok) {
		log(LOG_MISS, "JazzSource::StartService() failed. Flags must be 0 or 1.");

		return JAZZ_API_ERROR_PARSING_CONFIG;
	}

	lmdb_opt.flags =   MDB_FIXEDMAP*fixedmap
					 + MDB_WRITEMAP*writemap
					 + MDB_NOMETASYNC*nometasync
					 + MDB_NOSYNC*nosync
					 + MDB_MAPASYNC*mapasync
					 + MDB_NOLOCK*nolock
					 + MDB_NORDAHEAD*noreadahead
					 + MDB_NOMEMINIT*nomeminit;

	if (lmdb_opt.env_set_maxdbs > MAX_POSSIBLE_SOURCES) {
		log(LOG_MISS, "JazzSource::StartService() failed. The number of databases cannot exceed MAX_POSSIBLE_SOURCES");

		return JAZZ_API_ERROR_PARSING_CONFIG;
	}

	std::string pat;

	ok = p_config->get_key("MDB_PERSISTENCE_PATH", pat);

	if (!ok || pat.length() > MAX_LMDB_HOME_LEN - 1) {
		log(LOG_MISS, "JazzSource::StartService() failed. Missing or invalid MDB_PERSISTENCE_PATH.");

		return JAZZ_API_ERROR_PARSING_CONFIG;
	}

	strcpy(lmdb_opt.path, pat.c_str());

	struct stat st = {0};
	if (stat(lmdb_opt.path, &st) != 0) {
		log_printf(LOG_INFO, "Path \"%s\" does not exist, creating it.", pat.c_str());

		mkdir(lmdb_opt.path, 0700);
	}

	log(LOG_INFO, "Creating an lmdb environment.");

	if (mdb_env_create(&lmdb_env) != MDB_SUCCESS) {
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_create() failed.");

		return JAZZ_API_ERROR_STARTING_SERVICE;
	}

	if (mdb_env_set_maxreaders(lmdb_env, lmdb_opt.env_set_maxreaders) != MDB_SUCCESS) {
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_maxreaders() failed.");

		return JAZZ_API_ERROR_STARTING_SERVICE;
	}

	if (mdb_env_set_maxdbs(lmdb_env, lmdb_opt.env_set_maxdbs) != MDB_SUCCESS) {
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_maxdbs() failed.");

		return JAZZ_API_ERROR_STARTING_SERVICE;
	}

	if (mdb_env_set_mapsize(lmdb_env, ((mdb_size_t)1024)*1024*lmdb_opt.env_set_mapsize) != MDB_SUCCESS) {
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_mapsize() failed.");

		return JAZZ_API_ERROR_STARTING_SERVICE;
	}

	log_printf(LOG_INFO, "Opening LMDB environment at : \"%s\"", lmdb_opt.path);

	if (int ret = mdb_env_open(lmdb_env, lmdb_opt.path, lmdb_opt.flags, LMDB_UNIX_FILE_PERMISSIONS) != MDB_SUCCESS) {
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_open() failed.");

		return JAZZ_API_ERROR_STARTING_SERVICE;
	}
/*
	if (!open_all_sources())
	{
		log(LOG_MISS, "JazzSource::StartService() failed: open_all_sources() failed.");

		return false;
	}

	log(LOG_INFO, "jzzBLOCKS started.");
*/
	return JAZZ_API_NO_ERROR;
}


/** Close the JazzSource object persisting pending cached write operations, freeing resources, etc.

	\param restarting_service Tell the object that it will be used again immediately if true and that makes any difference.
	\return					  JAZZ_API_NO_ERROR or any other API_ErrorCode in cases errors occurred. (Errors will also be logged out.)
*/
API_ErrorCode JazzSource::ShutDown (bool restarting_service)
{
	log(LOG_INFO, "Closing all LMDB sources.");

//	close_all_sources();

//TODO: Flushing mechanism (Is it necessary?)
	log(LOG_INFO, "Flushing LMDB environment.");
	mdb_env_sync(lmdb_env, true);
//end of TODO

	log(LOG_INFO, "Closing LMDB environment.");

	mdb_env_close(lmdb_env);

	log(LOG_INFO, "jzzBLOCKS stopped.");

	return JAZZ_API_NO_ERROR;
}


/**
//TODO: Document new_jazz_block (1)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockId64 id64,
												pJazzBlock			p_as_block,
												pJazzFilter			p_row_filter,
												AllAttributes	   *att,
												uint64_t			time_to_build)
{
//TODO: Implement new_jazz_block (1)
}


/**
//TODO: Document new_jazz_block (2)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockId64 id64,
												int					cell_type,
												int				   *dim,
												AllAttributes	   *att,
												int					fill_tensor,
												bool			   *p_bool_filter,
												int					stringbuff_size,
												const char		   *p_text,
												char				eoln,
												uint64_t			time_to_build)
{
//TODO: Implement new_jazz_block (2)
}


/**
//TODO: Document new_jazz_block (3)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockIdentifier *p_id,
												pJazzBlock				   p_as_block,
												pJazzFilter				   p_row_filter,
												AllAttributes			  *att,
												uint64_t				   time_to_build)
{
//TODO: Implement new_jazz_block (3)
}


/**
//TODO: Document new_jazz_block (4)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockIdentifier *p_id,
												int						   cell_type,
												int						  *dim,
												AllAttributes			  *att,
												int						   fill_tensor,
												bool					  *p_bool_filter,
												int						   stringbuff_size,
												const char				  *p_text,
												char					   eoln,
												uint64_t				   time_to_build)
{
//TODO: Implement new_jazz_block (4)
}


/**
//TODO: Document find_jazz_block (1)
*/
pJazzPersistenceItem JazzSource::find_jazz_block(const JazzBlockIdentifier *p_id)
{
//TODO: Implement find_jazz_block (1)
}


/**
//TODO: Document find_jazz_block (2)
*/
pJazzPersistenceItem JazzSource::find_jazz_block(JazzBlockId64 id64)
{
//TODO: Implement find_jazz_block (2)
}


/**
//TODO: Document free_jazz_block (1)
*/
void JazzSource::free_jazz_block(pJazzPersistenceItem p_item)
{
//TODO: Implement free_jazz_block (1)
}


/**
//TODO: Document free_jazz_block (2)
*/
bool JazzSource::free_jazz_block(const JazzBlockIdentifier *p_id)
{
//TODO: Implement free_jazz_block (2)
}


/**
//TODO: Document free_jazz_block (3)
*/
bool JazzSource::free_jazz_block(JazzBlockId64 id64)
{
//TODO: Implement free_jazz_block (3)
}


/**
//TODO: Document alloc_cache
*/
bool JazzSource::alloc_cache(int num_items, int cache_mode)
{
//TODO: Implement alloc_cache
}


/**
//TODO: Document copy_to_keepr
*/
bool JazzSource::copy_to_keepr(JazzBlockKeepr keepr,
							   JazzBlockList  p_id,
							   int			  num_blocks)
{
//TODO: Implement copy_to_keepr
}


/**
//TODO: Document copy_from_keepr
*/
bool JazzSource::copy_from_keepr(JazzBlockKeepr keepr,
								 JazzBlockList	p_id,
								 int			num_blocks)
{
//TODO: Implement copy_from_keepr
}


/**
//TODO: Document open_jazz_file
*/
int JazzSource::open_jazz_file(const char *file_name)
{
//TODO: Implement open_jazz_file
}


/**
//TODO: Document flush_jazz_file
*/
int JazzSource::flush_jazz_file()
{
//TODO: Implement flush_jazz_file
}


/**
//TODO: Document file_errors
*/
int JazzSource::file_errors()
{
//TODO: Implement file_errors
}


/**
//TODO: Document close_jazz_file
*/
int JazzSource::close_jazz_file()
{
//TODO: Implement close_jazz_file
}


} // namespace jazz_persistence


#if defined LEGACY_LMDB_CODE

/*	--------------------------------------------------
	  M a n a g e	s o u r c e s
--------------------------------------------------- */

/** Locate all the sources in the current lmdb environment, add them to the source[] vector and open them all for reading.

	\return true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzBLOCKS::open_all_sources()
{
	if (numsources) return true;

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::open_all_sources().");

		return false;
	}

	MDB_dbi dbi;
	if (int err = mdb_dbi_open(txn, NULL, 0, &dbi)) {
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::open_all_sources().");

		goto release_txn_and_fail;
	}

	MDB_cursor *cursor;
	if (int err = mdb_cursor_open(txn, dbi, &cursor)) {
		log_lmdb_err_as_miss(err, "mdb_cursor_open() failed in jzzBLOCKS::open_all_sources().");

		goto release_dbi_and_fail;
	}

	source_open[SYSTEM_SOURCE_SYS] = false;
	source_open[SYSTEM_SOURCE_WWW] = false;

	MDB_val key, data;
	while (!mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) {
		sourceName name;

		if (!char_to_key((char *) key.mv_data, name)) {
			log(LOG_MISS, "mdb_cursor_get() returned an impossible source name in jzzBLOCKS::open_all_sources().");

			goto release_cur_and_fail;
		}

		if (!strcmp(name.key, "sys")) {
			numsources = max(numsources, SYSTEM_SOURCE_SYS + 1);
			source_nam [SYSTEM_SOURCE_SYS] = name;
			source_open[SYSTEM_SOURCE_SYS] = false;
		} else {
			if (!strcmp(name.key, "www")) {
				numsources = max(numsources, SYSTEM_SOURCE_WWW + 1);
				source_nam [SYSTEM_SOURCE_WWW] = name;
				source_open[SYSTEM_SOURCE_WWW] = false;
			} else {
				numsources = max(numsources, SYSTEM_SOURCE_WWW + 1);
				source_nam [numsources]	  = name;
				source_open[numsources++] = false;
			}
		}
	}

	mdb_cursor_close(cursor);
	mdb_dbi_close(lmdb_env, dbi);
	mdb_txn_abort(txn);

	if (numsources == 0) {
		if (!new_source("sys")) {
			log(LOG_MISS, "new_source('sys') failed in jzzBLOCKS::open_all_sources().");

			return false;
		}
		if (!new_source("www")) {
			log(LOG_MISS, "new_source('www') failed in jzzBLOCKS::open_all_sources().");

			return false;
		}
	} else {
		if (numsources < 2 || strcmp(source_nam[SYSTEM_SOURCE_SYS].key, "sys") || strcmp(source_nam[SYSTEM_SOURCE_WWW].key, "www")) {
			log(LOG_MISS, "sources 'sys' or 'www' not found in jzzBLOCKS::open_all_sources().");

			return false;
		}

		update_source_idx(false);
	}

	return true;


release_cur_and_fail:

	mdb_cursor_close(cursor);

release_dbi_and_fail:

	mdb_dbi_close(lmdb_env, dbi);

release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}


/** Return the value of the private variable numsources.
*/
int jzzBLOCKS::num_sources()
{
	return numsources;
}


/** Return the name of a source stored in source[] by index.
	\param idx	The index.
	\param name A pointer to return the name to.

	\return true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzBLOCKS::source_name(int idx, sourceName &name)
{
	if (idx < 0 || idx >= numsources) {
		log(LOG_MISS, "jzzBLOCKS::source_name(): wrong index.");

		return false;
	}

	name = source_nam[idx];

	return true;
}


/** Return the index for a source name stored in source[] by searching its name.
	\param name The name of the source to be found.

	\return The index or -1 if failed;
*/
int jzzBLOCKS::get_source_idx(const char * name)
{
	int i_max = source_idx[jazz_utils::TenBitsAtAddress(name)];

	for (int i = i_max; i >= 0; i--) {
		if (!strcmp(name, source_nam[i].key)) return i;
	}

	return -1;
}


/** Create a new lmdb dbi and add it as a new source to the source[] vector.
	\param name The name of the source to be added.

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	NOTE: new_source() is not thread safe! Unsafe use of: numsources, source_nam, source_open, source_dbi.
*/
bool jzzBLOCKS::new_source(const char * name)
{
	if (numsources >= MAX_POSSIBLE_SOURCES) {
		log(LOG_MISS, "jzzBLOCKS::new_source(): too many sources.");

		return false;
	}

	sourceName s_name;

	if (!char_to_key(name, s_name)) {
		log(LOG_MISS, "jzzBLOCKS::new_source(): invalid name.");

		return false;
	}

	if (get_source_idx(name) >= 0) {
		log(LOG_MISS, "jzzBLOCKS::new_source(): source already exists.");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::new_source().");

		return false;
	}

	int idx = numsources;

	block_SourceAttributes src_attr = {};

	if (int err = mdb_dbi_open(txn, s_name.key, MDB_CREATE, &source_dbi[idx])) {
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::new_source().");

		goto release_txn_and_fail;
	}

	source_nam [idx] = s_name;
	source_open[idx] = true;

	MDB_val l_key, data;

	persistedKey key;
	key.key[0] = '.';

	src_attr.type	 = BLOCKTYPE_SOURCE_ATTRIB;
	src_attr.length	 = 1;
	src_attr.size	 = sizeof(block_SourceAttributes) - sizeof(jzzBlockHeader);
	src_attr.version = 1001;
	src_attr.hash64	 = 0;

	l_key.mv_size = 1;
	l_key.mv_data = &key.key;
	data.mv_size = sizeof(block_SourceAttributes);
	data.mv_data = &src_attr;

	if (int err = mdb_put(txn, source_dbi[idx], &l_key, &data, 0)) {
		log_lmdb_err_as_miss(err, "mdb_put() failed in jzzBLOCKS::new_source().");

		goto release_dbi_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::new_source().");

		goto release_dbi_and_fail;
	}

	numsources++;

	update_source_idx(true);

	return true;


release_dbi_and_fail:

	mdb_dbi_close(lmdb_env, source_dbi[idx]);

release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}


/** Kill a source, both from the lmdb persistence and from the source[] vector.
	\param name The name of the source to be killed.

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	NOTE: kill_source() is EXTREMELY not thread safe! Indices to ALL sources may change. Unsafe use of: numsources, source_nam, source_open,
source_dbi.
*/
bool jzzBLOCKS::kill_source(const char * name)
{
	int idx = get_source_idx(name);

	if (idx < 0) {
		log(LOG_MISS, "jzzBLOCKS::kill_source(): source does not exist.");

		return false;
	}

	if (idx < 2) {
		log(LOG_MISS, "jzzBLOCKS::kill_source(): attempt to kill system source.");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::kill_source().");

		return false;
	}

	if (!source_open[idx]) {
		if (!internal_open_dbi(idx)) {
			log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::kill_source().");

			goto release_txn_and_fail;
		}
	}

	if (int err = mdb_drop(txn, source_dbi[idx], 1)) {
		log_lmdb_err_as_miss(err, "mdb_drop() failed in jzzBLOCKS::kill_source().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::kill_source().");

		goto release_txn_and_fail;
	}

	for (int i = idx;i < numsources - 1; i++) {
		source_nam [i] = source_nam [i + 1];
		source_open[i] = source_open[i + 1];
		source_dbi [i] = source_dbi [i + 1];
	}

	numsources--;

	update_source_idx(false);

	return true;


release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}


/** Close all sources on lmdb leaving them ready for a subsequent opening.

	This makes numsources == 0 by closing used lmdb handles via mdb_dbi_close().
*/
void jzzBLOCKS::close_all_sources()
{
//_in_deprecated_code_TODO: Remove remark

	for(int idx = 0; idx < numsources; idx++)
		if (source_open[idx]) mdb_dbi_close(lmdb_env, source_dbi[idx]);

	numsources = 0;

//_in_deprecated_code_TODO: Flushing mechanism (Is it necessary?)
//	mdb_env_sync(lmdb_env, true);

	update_source_idx(false);
}

/*	--------------------------------------------------
	  R e a d	a n d	w r i t e	b l o c k s
--------------------------------------------------- */

/** Write a block to persistence.
	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  A key identifying the block in that source.
	\param block  The block to be written.

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	Writing to persistence requires no protected pointer in the source. In case many simultaneous read/write operations concur, the reader can
	preemptively copy the protected blocks to JAZZALLOC()ed blocks and return copies of the blocks to make unprotection unnecessary. It has yet
	to be determined if such a mechanism is necessary and it will not be implemented initially.
*/
bool jzzBLOCKS::block_put(int source, const persistedKey &key, pJazzBlock block)
{
	if (source < 0 || source >= numsources) {
		log(LOG_MISS, "Invalid source in jzzBLOCKS::block_put().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_put().");

		return false;
	}

	if (!source_open[source]) {
		if (!internal_open_dbi(source)) {
			log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_put().");

			goto release_txn_and_fail;
		}
	}

	MDB_val l_key, data;

	l_key.mv_size = strlen(key.key);
	l_key.mv_data = (void *) &key;
	data.mv_size = block->size + sizeof(jzzBlockHeader);
	data.mv_data = block;

	if (int err = mdb_put(txn, source_dbi[source], &l_key, &data, 0)) {
		log_lmdb_err_as_miss(err, "mdb_put() failed in jzzBLOCKS::block_put().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::block_put().");

		goto release_txn_and_fail;
	}

	return true;


release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}


/** Read block from persistence.
	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  A key identifying the block in that source.
	\param block  The address of a pJazzBlock where the result is returned. That pointer MUST be block_unprotect()ed.

	\return true if successful, false and log(LOG_MISS, "further details") EXCEPT for MDB_NOTFOUND where block_get() returns false silently.

From lmdb:
> The memory pointed to by the returned values is owned by the lmdb. The caller need not dispose of the memory, and may not modify it in any way.
> For values returned in a read-only transaction any modification attempts will cause a SIGSEGV. Values returned from the database are valid only
> until a subsequent update operation, or the end of the transaction.

	The caller does not own this pointer and can only new_block_copy() it in case it needs to be modified and MUST block_unprotect() it when no
longer needed.

	WARNING: A lockless, non-atomic assignment via lock_get_protect_slot()/lock_release_protect_slot() could require a shared use of prot_SP using
a mutex to be thread safe.
*/
bool jzzBLOCKS::block_get(int source, const persistedKey &key, pJazzBlock &block)
{
	if (source < 0 || source >= numsources) {
		log(LOG_MISS, "Invalid source in jzzBLOCKS::block_get().");

		return false;
	}

	int prot_idx = lock_get_protect_slot();
	if (prot_idx < 0) {
		log(LOG_MISS, "No slot in block protection found in jzzBLOCKS::block_get().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn)) {
		lock_release_protect_slot(prot_idx);

		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_get().");

		return false;
	}

	if (!source_open[source]) {
		if (!internal_open_dbi(source)) {
			log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_get().");

			goto release_txn_and_fail;
		}
	}

	MDB_val l_key, data;

	l_key.mv_size = strlen(key.key);
	l_key.mv_data = (void *) &key;

	if (int err = mdb_get(txn, source_dbi[source], &l_key, &data)) {
		if (err == MDB_NOTFOUND) goto release_txn_and_fail; // Not found does not log anything

		log_lmdb_err_as_miss(err, "mdb_get() failed in jzzBLOCKS::block_get() with a code other than MDB_NOTFOUND.");

		goto release_txn_and_fail;
	}

	block = (pJazzBlock) data.mv_data;

	protect_txn[prot_idx] = txn;
	protect_ptr[prot_idx] = block;

	return true;


release_txn_and_fail:

	lock_release_protect_slot(prot_idx);

	mdb_txn_abort(txn);

	return false;
}


/** Delete block in persistence.
	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  A key identifying the block to be deleted in that source.

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	Writing to persistence requires no protected pointer in the source. In case many simultaneous read/write operations concur, the reader can
	preemptively copy the protected blocks to JAZZALLOC()ed blocks and return copies of the blocks to make unprotection unnecessary. It has yet
	to be determined if such a mechanism is necessary and it will not be implemented initially.
*/
bool jzzBLOCKS::block_kill(int source, const persistedKey &key)
{
	if (source < 0 || source >= numsources) {
		log(LOG_MISS, "Invalid source in jzzBLOCKS::block_kill().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_kill().");

		return false;
	}

	if (!source_open[source]) {
		if (!internal_open_dbi(source)) {
			log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_kill().");

			goto release_txn_and_fail;
		}
	}

	MDB_val l_key;

	l_key.mv_size = strlen(key.key);
	l_key.mv_data = (void *) &key;

	if (int err = mdb_del(txn, source_dbi[source], &l_key, NULL)) {
		log_lmdb_err_as_miss(err, "mdb_del() failed in jzzBLOCKS::block_kill().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn)) {
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::block_kill().");

		goto release_txn_and_fail;
	}

	return true;


release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}

/*	----------------------------------------------------------
	  F o r e i g n	  p o i n t e r	  m a n a g e m e n t .
----------------------------------------------------------- */

/** Not using lock_despite the name in current version (see WARNING in block_unprotect() & block_get().)
*/
int jzzBLOCKS::lock_get_protect_slot()
{
	int i = prot_SP++;

	if (i >= MAX_PROTECTED_BLOCKS) {
		prot_SP--;

		return -1;
	}

	return i;
}


/** Not using lock_despite the name in current version (see WARNING in block_unprotect() & block_get().)
*/
void jzzBLOCKS::lock_release_protect_slot(int idx)
{
	protect_ptr[idx] = NULL;
	protect_txn[idx] = NULL;

	if (idx == prot_SP - 1) {
		prot_SP--;
		while (prot_SP > 0 && protect_ptr[prot_SP - 1] == NULL) prot_SP--;
	}
}


/** Unprotect a pointer (owned by lmdb) returned by block_get()
	\param block  The pointer returned by block_get().

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	Pointers returned by block_get() must be block_unprotect()ed as soon as a decision on the block can be made. If the caller needs the information
contained in the block for a longer time, that block has to be new_block_copy()ed.

	WARNING: A lockless, non-atomic assignment via lock_get_protect_slot()/lock_release_protect_slot() could require a shared use of prot_SP using
a mutex to be thread safe.
*/
bool jzzBLOCKS::block_unprotect (pJazzBlock block)
{
	for (int i = prot_SP - 1; i >= 0; i--) {
		if (protect_ptr[i] == block) {
			MDB_txn * txn = protect_txn[i];

			lock_release_protect_slot(i);

			if (txn) mdb_txn_abort(txn);

			return true;
		}
	}

	log(LOG_MISS, "'block' not found in 'protect_ptr[]' in jzzBLOCKS::block_unprotect().");

	return false;
}

/*	--------------------------------------------------
	  i n t e r n a l _ o p e n_ d b i
--------------------------------------------------- */

/** Open a dbi in a way it becomes available to other transactions.

	This is the key explanation taken from the LMDB doc to understand the necessity of this.

	"The database handle will be private to the current transaction until the transaction is successfully committed. If the transaction is
	aborted the handle will be closed automatically. After a successful commit the handle will reside in the shared environment, and may be
	used by other transactions."

	NOTE: It is only called inside this module when source_open[source] is false.
*/
bool jzzBLOCKS::internal_open_dbi (int source)
{
	MDB_txn * txo;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txo)) {
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::internal_open_dbi().");

		return false;
	}

	if (int err = mdb_dbi_open(txo, source_nam[source].key, 0, &source_dbi[source])) {
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::internal_open_dbi().");

		mdb_txn_abort(txo);

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txo)) {
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::internal_open_dbi().");

		goto release_txn_and_fail;
	}

	source_open[source] = true;

	return true;


release_txn_and_fail:

	mdb_txn_abort(txo);

	return false;
}


/** Initialize the source_idx[] array to enable get_source_idx() operation.

	\param incremental Just update all the references without clearing the buffer (use by new_source() only.)
*/
void jzzBLOCKS::update_source_idx(bool incremental)
{
	if (!incremental) memset(source_idx, -1, sizeof(source_idx));

	for (int i = 0; i < numsources; i++) {
		int j = jazz_utils::TenBitsAtAddress(source_nam[i].key);

		source_idx[j] = max(i, source_idx[j]);
	}
}

/*	--------------------------------------------------
	  D E B U G	  u t i l s
--------------------------------------------------- */

/** A nicer presentation for LMDB error messages.
*/
void jzzBLOCKS::log_lmdb_err_as_miss(int err, const char * msg)
{
	char errmsg [128];
	switch (err) {
		case MDB_KEYEXIST:		  strcpy(errmsg, "LMDB MDB_KEYEXIST: Key/data pair already exists.");									  break;
		case MDB_NOTFOUND:		  strcpy(errmsg, "LMDB MDB_NOTFOUND: Key/data pair not found (EOF).");									  break;
		case MDB_PAGE_NOTFOUND:	  strcpy(errmsg, "LMDB MDB_PAGE_NOTFOUND: Requested page not found - this usually indicates corruption.");break;
		case MDB_CORRUPTED:		  strcpy(errmsg, "LMDB MDB_CORRUPTED: Located page was wrong type.");									  break;
		case MDB_PANIC:			  strcpy(errmsg, "LMDB MDB_PANIC: Update of meta page failed or environment had a fatal error.");		  break;
		case MDB_VERSION_MISMATCH:strcpy(errmsg, "LMDB MDB_VERSION_MISMATCH: Environment version mismatch.");							  break;
		case MDB_INVALID:		  strcpy(errmsg, "LMDB MDB_INVALID: File is not a valid LMDB file.");									  break;
		case MDB_MAP_FULL:		  strcpy(errmsg, "LMDB MDB_MAP_FULL: Environment mapsize reached.");									  break;
		case MDB_DBS_FULL:		  strcpy(errmsg, "LMDB MDB_DBS_FULL: Environment maxdbs reached.");										  break;
		case MDB_READERS_FULL:	  strcpy(errmsg, "LMDB MDB_READERS_FULL: Environment maxreaders reached.");								  break;
		case MDB_TLS_FULL:		  strcpy(errmsg, "LMDB MDB_TLS_FULL: Too many TLS keys in use - Windows only.");						  break;
		case MDB_TXN_FULL:		  strcpy(errmsg, "LMDB MDB_TXN_FULL: Txn has too many dirty pages.");									  break;
		case MDB_CURSOR_FULL:	  strcpy(errmsg, "LMDB MDB_CURSOR_FULL: Cursor stack too deep - internal error.");						  break;
		case MDB_PAGE_FULL:		  strcpy(errmsg, "LMDB MDB_PAGE_FULL: Page has not enough space - internal error.");					  break;
		case MDB_MAP_RESIZED:	  strcpy(errmsg, "LMDB MDB_MAP_RESIZED: Database contents grew beyond environment mapsize.");			  break;
		case MDB_INCOMPATIBLE:	  strcpy(errmsg, "LMDB MDB_INCOMPATIBLE: Operation and DB incompatible, or DB type changed (see doc).");  break;
		case MDB_BAD_RSLOT:		  strcpy(errmsg, "LMDB MDB_BAD_RSLOT: Invalid reuse of reader locktable slot.");						  break;
		case MDB_BAD_TXN:		  strcpy(errmsg, "LMDB MDB_BAD_TXN: Transaction must abort, has a child, or is invalid.");				  break;
		case MDB_BAD_VALSIZE:	  strcpy(errmsg, "LMDB MDB_BAD_VALSIZE: Unsupported size of key/DB name/data, or wrong DUPFIXED size.");  break;
		case MDB_BAD_DBI:		  strcpy(errmsg, "LMDB MDB_BAD_DBI: The specified DBI was changed unexpectedly.");						  break;
		case MDB_PROBLEM:		  strcpy(errmsg, "LMDB MDB_PROBLEM: Unexpected problem - txn should abort.");							  break;
		case EACCES:			  strcpy(errmsg, "LMDB EACCES: An attempt was made to write in a read-only transaction.");				  break;
		case EINVAL:			  strcpy(errmsg, "LMDB EINVAL: An invalid parameter was specified.");									  break;
		case EIO:				  strcpy(errmsg, "LMDB EIO: A low-level I/O error occurred while writing.");							  break;
		case ENOSPC:			  strcpy(errmsg, "LMDB ENOSPC: No more disk space.");													  break;
		case ENOMEM:			  strcpy(errmsg, "LMDB ENOMEM: Out of memory.");														  break;
		default:				  sprintf(errmsg,"LMDB Unknown code %d.", err);
	}

	log(LOG_ERROR, errmsg);
	log(LOG_MISS, msg);
}
#endif


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_persistence.ctest"
#endif

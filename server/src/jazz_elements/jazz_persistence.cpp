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

		return false;
	}

	ok = ((fixedmap | writemap | nometasync | nosync | mapasync | nolock | noreadahead | nomeminit) & 0xfffffffe) == 0;

	if (!ok) {
		log(LOG_MISS, "JazzSource::StartService() failed. Flags must be 0 or 1.");

		return false;
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

		return false;
	}

	std::string pat;

	ok = p_config->get_key("MDB_PERSISTENCE_PATH", pat);

	if (!ok || pat.length() > MAX_LMDB_HOME_LEN - 1) {
		log(LOG_MISS, "JazzSource::StartService() failed. Missing or invalid MDB_PERSISTENCE_PATH.");

		return false;
	}

	strcpy(lmdb_opt.path, pat.c_str());
/*
	struct stat st = {0};
	if (stat(lmdb.path, &st) != 0)
	{
		log_printf(LOG_INFO, "Path \"%s\" does not exist, creating it.", pat.c_str());

		mkdir(lmdb.path, 0700);
	}

	log(LOG_INFO, "Creating an lmdb environment.");

	if (mdb_env_create(&lmdb_env) != MDB_SUCCESS)
	{
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_create() failed.");

		return false;
	}

	if (mdb_env_set_maxreaders(lmdb_env, lmdb.env_set_maxreaders) != MDB_SUCCESS)
	{
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_maxreaders() failed.");

		return false;
	}

	if (mdb_env_set_maxdbs(lmdb_env, lmdb.env_set_maxdbs) != MDB_SUCCESS)
	{
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_maxdbs() failed.");

		return false;
	}

	if (mdb_env_set_mapsize(lmdb_env, ((mdb_size_t)1024)*1024*lmdb.env_set_mapsize) != MDB_SUCCESS)
	{
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_set_mapsize() failed.");

		return false;
	}

	log_printf(LOG_INFO, "Opening LMDB environment at : \"%s\"", lmdb.path);

	if (int ret = mdb_env_open(lmdb_env, lmdb.path, lmdb.flags, LMDB_UNIX_FILE_PERMISSIONS) != MDB_SUCCESS)
	{
		cout << "Returned:" << ret << endl;
		log(LOG_MISS, "JazzSource::StartService() failed: mdb_env_open() failed.");

		return false;
	}

	if (!open_all_sources())
	{
		log(LOG_MISS, "JazzSource::StartService() failed: open_all_sources() failed.");

		return false;
	}

	log(LOG_INFO, "jzzBLOCKS started.");
*/
	return JAZZ_API_NO_ERROR;
}


API_ErrorCode JazzSource::ShutDown (bool restarting_service)
{

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


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_persistence.ctest"
#endif

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


// #include <stl_whatever>


#include "src/jazz_elements/volatile.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_PERSISTED
#define INCLUDED_JAZZ_ELEMENTS_PERSISTED

#include "src/lmdb/lmdb.h"


namespace jazz_elements
{

#define MAX_POSSIBLE_SOURCES		 32		///< The number of databases is configurable via MDB_ENV_SET_MAXDBS, but cannot exceed this.
#define MAX_LMDB_HOME_LEN			128		///< The number of char for the LMDB home path
#define LMDB_UNIX_FILE_PERMISSIONS 0664		///< The file permissions (as in chmod) for the database files


/** \brief All the necessary LMDB options (a binary representation of the values in the config file)
*/
struct JazzLmdbOptions {
	char path[MAX_LMDB_HOME_LEN];

	int env_set_mapsize,
		env_set_maxreaders,
		env_set_maxdbs,
		flags;
};

/** \brief Persisted: A Service to manage data objects in LMDB.

*/
class Persisted : public Container {

	public:

		Persisted (pLogger	   a_logger,
				   pConfigFile a_config);

		StatusCode start	 ();
		StatusCode shut_down ();

		// The easy interface (Requires explicit pulling because of the native interface using the same names.)

		using Container::get;
		using Container::header;
		using Container::put;
		using Container::new_entity;
		using Container::remove;
		using Container::copy;

		// The "native" interface

		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what,
										pBlock				p_row_filter);
		virtual StatusCode get		   (pTransaction	   &p_txn,
							  			Locator			   &what,
							  			pChar				name);
		virtual StatusCode header	   (StaticBlockHeader  &p_txn,
										Locator			   &what);
		virtual StatusCode header	   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode put		   (Locator			   &where,
										pBlock				p_block,
										int					mode = WRITE_ALWAYS_COMPLETE);
		virtual StatusCode new_entity  (Locator			   &what);
		virtual StatusCode remove	   (Locator			   &what);
		virtual StatusCode copy		   (Locator			   &where,
										Locator			   &what);

		// Support for container names in the API .base_names()

		void base_names (BaseNames &base_names);

#ifndef CATCH_TEST
	private:
#endif

		inline void close_block(pBlock p_block) {
			p_block->hash64  = MurmurHash64A(&p_block->tensor, p_block->total_bytes - sizeof(StaticBlockHeader));
			p_block->created = std::chrono::steady_clock::now();
		}

		inline bool check_block(pBlock p_block) {
			return p_block->hash64 == MurmurHash64A(&p_block->tensor, p_block->total_bytes - sizeof(StaticBlockHeader));
		}
};
typedef Persisted *pPersisted;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_PERSISTED

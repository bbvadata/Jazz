/* BBVA - Jazz: A lightweight analytical web server for data-driven applications.
   ------------

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

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

using namespace std;

/**< \brief The lowest level of persisted data management: the blocks.

	This module includes all the LMDB management in the server.
*/

#include "src/include/jazz_commons.h"
#include "src/lmdb/lmdb.h"

/*~ end of automatic header ~*/

#ifndef jzz_IG_JZZBLOCKS
#define jzz_IG_JZZBLOCKS

#define MAX_POSSIBLE_SOURCES		 32		///< The number of databases is configurable via MDB_ENV_SET_MAXDBS, but cannot exceed this.
#define MAX_PROTECTED_BLOCKS		 16		///< The number of blocks that can be simultaneously protected (all threads included).
#define MAX_LMDB_HOME_LEN			128		///< The number of char for the LMDB home path
#define LMDB_UNIX_FILE_PERMISSIONS 0664		///< The file permissions (as in chmod) for the database files

typedef persistedKey sourceName;			///< A source name is just a (persistedKey) key.

struct jazz_lmdb_opt
{
	char path[MAX_LMDB_HOME_LEN];

	int env_set_mapsize,
		env_set_maxreaders,
		env_set_maxdbs,
		flags;
};

/** block_C_OFFS_CHARS are optimized for reading. When writing, get_string_idx_C_OFFS_CHARS() will only search if the number of total strings
stored is below this value. When a search fails after searching a number above this value, the isBig field will be set and all subsequent
call to get_string_idx_C_OFFS_CHARS() will allocate new space for each string without searching.
*/
#define STR_SEARCH_BIG_ABOVE	25

#define STRING_BUFFER(pstr) reinterpret_cast<pStringBuff>(&pstr->data[pstr->length])	///< Returns a pointer to the string buffer in a pCharBlock
#define PCHAR(pstr, idx)((char *)STRING_BUFFER(pstr) + (uintptr_t)pstr->data[idx])		///< Returns a string by index inside a pCharBlock

typedef persistedKey * pKey;
typedef sourceName	 * pSource;

#define SYSTEM_SOURCE_SYS	0
#define SYSTEM_SOURCE_WWW	1


class jzzBLOCKS: public jazzService {

	public:

		 jzzBLOCKS();
		~jzzBLOCKS();

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();

// Functions to create simple types of blocks.

		bool new_block_C_BOOL_rep	 (pBoolBlock &pb, unsigned char	x,	int times = 1);
		bool new_block_C_INTEGER_rep (pIntBlock	 &pb, int	x,			int times = 1);
		bool new_block_C_INTEGER_seq (pIntBlock	 &pb, int	from,		int to,		   int	  by = 1);
		bool new_block_C_REAL_rep	 (pRealBlock &pb, double x,			int times = 1);
		bool new_block_C_REAL_seq	 (pRealBlock &pb, double from,		double to,	   double by = 1.0);
		bool new_block_C_CHARS_rep	 (pCharBlock &pb, const char * str, int times = 1);
		bool new_block_C_RAW_once	 (pRawBlock	 &pb, const void * ps,	int size);

		bool reinterpret_cast_block	 (pJazzBlock pb,	  int		  type);
		bool new_block_copy			 (pJazzBlock psrc, pJazzBlock &pdest);

// Functions to resize string vectors and add strings to vectors.

		bool format_C_OFFS_CHARS		 (pCharBlock  pstr, int			 length);
		int	 get_string_idx_C_OFFS_CHARS (pCharBlock  pstr, const char * string, int len);
		bool realloc_C_OFFS_CHARS		 (pCharBlock &pstr, int			 extra_length);

// Functions to manage sources.

		bool open_all_sources  ();
		int	 num_sources	   ();
		bool source_name	   (int			 idx,  sourceName &name);
		int	 get_source_idx	   (const char * name);
		bool new_source		   (const char * name);
		bool kill_source	   (const char * name);
		void close_all_sources ();

// Functions to read and write blocks.

		bool block_put		 (int source, const persistedKey &key, pJazzBlock	block);
		bool block_get		 (int source, const persistedKey &key, pJazzBlock &block);
		bool block_kill		 (int source, const persistedKey &key);

// Functions on block utils.

		void hash_block		 (pJazzBlock pblock);
		bool compare_headers (pJazzBlock pb1, pJazzBlock pb2);
		bool compare_content (pJazzBlock pb1, pJazzBlock pb2);

// Functions about key utils.

		bool char_to_key	 (const char * pkey,		persistedKey &key);
		int	 strcmp_keys	 (const persistedKey key1,	const persistedKey key2);
		bool next_key		 (int source,				persistedKey &key);
		void set_first_key	 (persistedKey &key);
		bool is_last_key	 (const persistedKey &key);

// Foreign pointer management.

		bool block_unprotect (pJazzBlock block);

#ifndef CATCH_TEST
	private:
#endif

		typedef jazzService super;

		int	 lock_get_protect_slot();
		void lock_release_protect_slot(int idx);

		bool internal_open_dbi	  (int source);
		void log_lmdb_err_as_miss (int err, const char * msg);

		void update_source_idx	  (bool incremental);

		int			  source_idx [TEN_BITS_RANGE];
		int			  numsources, prot_SP;
		sourceName	  source_nam [MAX_POSSIBLE_SOURCES];
		bool		  source_open[MAX_POSSIBLE_SOURCES];
		MDB_dbi		  source_dbi [MAX_POSSIBLE_SOURCES];
		MDB_txn *	  protect_txn[MAX_PROTECTED_BLOCKS];
		pJazzBlock	  protect_ptr[MAX_PROTECTED_BLOCKS];

		jazz_lmdb_opt lmdb;
		MDB_env *	  lmdb_env;
};

#endif


#if defined CATCH_TEST

	// Paths for CATCH_TEST only

	#define TEST_LMDB_PATH	"/tmp/test_lmdb"
	#define TEST_LMDB_DBI	TEST_LMDB_PATH "/data.mdb"

#endif

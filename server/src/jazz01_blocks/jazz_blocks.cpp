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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

/**< \brief The lowest level of persisted data management: the blocks.

	This module includes all the LMDB management in the server.
*/

#include "src/jazz01_blocks/jazz_blocks.h"

/*~ end of automatic header ~*/

#include <float.h>

/*	--------------------------------------------------
	  C o n s t r u c t o r / d e s t r u c t o r
--------------------------------------------------- */

/** The constructor implements the initialization logic that does not depend on configuration since configuration is not available at that time.
Starting LMDB also requires configuration. Therefore, most of the initialization is done in Start() and not here.
*/
jzzBLOCKS::jzzBLOCKS()
{
	numsources = 0;
	prot_SP	   = 0;

	update_source_idx(false);
}


/** Any service (logger, configuration, etc.) can be closed before this. Only do non lmdb related finalization here, if necessary.
*/
jzzBLOCKS::~jzzBLOCKS()
{
}

/*	-----------------------------------------------------
	  Methods inherited from   j a z z S e r v i c e
------------------------------------------------------ */

int blocks_lock = 0;

/** Start of jzzBLOCKS

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	This service initialization checks configuration values related with persistence and starts lmdb with configured values.
*/
bool jzzBLOCKS::start()
{
	bool ok = super::start();

	if (!ok) return false;

	if (++blocks_lock != 1)
	{
		blocks_lock--;

		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed. failed to get blocks_lock.");

		return false;
	}

	int fixedmap, writemap, nometasync, nosync, mapasync, nolock, nordahead, nomeminit;

	ok =  jCommons.get_config_key("JazzPERSISTENCE.MDB_ENV_SET_MAPSIZE",	lmdb.env_set_mapsize)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_ENV_SET_MAXREADERS", lmdb.env_set_maxreaders)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_ENV_SET_MAXDBS",		lmdb.env_set_maxdbs)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_FIXEDMAP",			fixedmap)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_WRITEMAP",			writemap)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_NOMETASYNC",			nometasync)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_NOSYNC",				nosync)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_MAPASYNC",			mapasync)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_NOLOCK",				nolock)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_NORDAHEAD",			nordahead)
		& jCommons.get_config_key("JazzPERSISTENCE.MDB_NOMEMINIT",			nomeminit);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed. Invalid MHD_* config (integer values).");

		return false;
	}

	ok = ((fixedmap | writemap | nometasync | nosync | mapasync | nolock | nordahead | nomeminit) & 0xfffffffe) == 0;

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed. Flags must be 0 or 1.");

		return false;
	}

	lmdb.flags =   MDB_FIXEDMAP*fixedmap
				 + MDB_WRITEMAP*writemap
				 + MDB_NOMETASYNC*nometasync
				 + MDB_NOSYNC*nosync
				 + MDB_MAPASYNC*mapasync
				 + MDB_NOLOCK*nolock
				 + MDB_NORDAHEAD*nordahead
				 + MDB_NOMEMINIT*nomeminit;

	if (lmdb.env_set_maxdbs > MAX_POSSIBLE_SOURCES)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed. The number of databases cannot exceed MAX_POSSIBLE_SOURCES");

		return false;
	}

	string pat;

	ok = jCommons.get_config_key("JazzPERSISTENCE.MDB_PERSISTENCE_PATH", pat);

	if (!ok || pat.length() > MAX_LMDB_HOME_LEN - 1)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed. Missing or invalid MDB_PERSISTENCE_PATH.");

		return false;
	}

	strcpy(lmdb.path, pat.c_str());

//TODO: Understand why this breaks CATCH_TEST

#ifndef CATCH_TEST

//TODO: Now every run removes the database from file to avoid syssegv. -- Remove this when fixed -------//
	struct stat st1 = {0};																				//
																										//
	if (stat(lmdb.path, &st1) == 0 && S_ISDIR(st1.st_mode))												//
	{																									//
		string dbi = pat;																				//
		dbi = dbi + "/data.mdb";																		//
		remove(dbi.c_str());																			//
																										//
		if (remove(lmdb.path))																			//
		{																								//
			return false;																				//
		}																								//
	}																									//
//TODO: Now every run removes the database from file to avoid syssegv. -- Remove this when fixed -------//

#endif

	struct stat st = {0};
	if (stat(lmdb.path, &st) != 0)
	{
		jCommons.log_printf(LOG_INFO, "Path \"%s\" does not exist, creating it.", pat.c_str());

		mkdir(lmdb.path, 0700);
	}

	jCommons.log(LOG_INFO, "Creating an lmdb environment.");

	if (mdb_env_create(&lmdb_env) != MDB_SUCCESS)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: mdb_env_create() failed.");

		return false;
	}

	if (mdb_env_set_maxreaders(lmdb_env, lmdb.env_set_maxreaders) != MDB_SUCCESS)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: mdb_env_set_maxreaders() failed.");

		return false;
	}

	if (mdb_env_set_maxdbs(lmdb_env, lmdb.env_set_maxdbs) != MDB_SUCCESS)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: mdb_env_set_maxdbs() failed.");

		return false;
	}

	if (mdb_env_set_mapsize(lmdb_env, ((mdb_size_t)1024)*1024*lmdb.env_set_mapsize) != MDB_SUCCESS)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: mdb_env_set_mapsize() failed.");

		return false;
	}

	jCommons.log_printf(LOG_INFO, "Opening LMDB environment at : \"%s\"", lmdb.path);

	if (int ret = mdb_env_open(lmdb_env, lmdb.path, lmdb.flags, LMDB_UNIX_FILE_PERMISSIONS) != MDB_SUCCESS)
	{
		cout << "Returned:" << ret << endl;
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: mdb_env_open() failed.");

		return false;
	}

	if (!open_all_sources())
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::start() failed: open_all_sources() failed.");

		return false;
	}

	jCommons.log(LOG_INFO, "jzzBLOCKS started.");

	return true;
}


/** Stop of jzzBLOCKS

	\return true and log(LOG_INFO, "further details"). No error checking.

	see jazzService::stop()
*/
bool jzzBLOCKS::stop()
{
	jCommons.log(LOG_INFO, "Closing all LMDB sources.");

	close_all_sources();

//TODO: Flushing mechanism (Is it necessary?)
	jCommons.log(LOG_INFO, "Flushing LMDB environment.");
	mdb_env_sync(lmdb_env, true);
//TODO: Flushing mechanism (Is it necessary?)

	jCommons.log(LOG_INFO, "Closing LMDB environment.");

	mdb_env_close(lmdb_env);

	jCommons.log(LOG_INFO, "jzzBLOCKS stopped.");

	blocks_lock--;

	return super::stop();
}


/** Reload of jzzBLOCKS

	\return true and log(LOG_INFO, "further details"). No reloading implemented.

	see jazzService::reload()
*/
bool jzzBLOCKS::reload()
{
	bool ok = super::reload();

	if (!ok) return false;

//TODO: reloading lmdb
//	jCommons.log(LOG_INFO, "Flushing LMDB environment.");
//
//	mdb_env_sync(lmdb_env, true);
//
//	jCommons.log(LOG_INFO, "Closing all LMDB sources.");
//
//	close_all_sources();
//
//	if (!open_all_sources())
//	{
//		jCommons.log(LOG_MISS, "jzzBLOCKS::reload() failed: open_all_sources() failed.");
//
//		return false;
//	}

	jCommons.log(LOG_INFO, "jzzBLOCKS reloaded, but no configuration changes applied.");

	return true;
}

/*	----------------------------------------------------------------
	  C r e a t e	s i m p l e	  t y p e s	  o f	b l o c k s
----------------------------------------------------------------- */

double TWO_ORDER_DBL_EPSILON = 100*DBL_EPSILON; ///< Some acceptable rounding error used for size estimation in new_block_C_REAL_seq()

/** Create a block_C_BOOL by repeating a value a number of times.

	\param pb	 The address of a pBoolBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(., RAM_ALLOC_C_BOOL).
	\param x	 The value to be initialized.
	\param times The number of times to repeat that value == length of the resulting vector.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzBLOCKS::new_block_C_BOOL_rep(pBoolBlock &pb, unsigned char x, int times)
{
	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_BOOL, times);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_BOOL_rep() alloc failed.");

		return false;
	}

	for (int i = 0; i < times; i++) pb->data[i] = x;

	return true;
}


/** Create a block_C_INTEGER by repeating a value a number of times.

	\param pb	 The address of a pIntBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
				 RAM_ALLOC_C_INTEGER).
	\param x	 The value to be initialized.
	\param times The number of times to repeat that value == length of the resulting vector.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	The type of the block can be changed later from BLOCKTYPE_C_INTEGER to BLOCKTYPE_C_FACTOR or BLOCKTYPE_C_GRADE using
reinterpret_cast_block().
*/
bool jzzBLOCKS::new_block_C_INTEGER_rep(pIntBlock &pb, int x, int times)
{
	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_INTEGER, times);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_INTEGER_rep() alloc failed.");

		return false;
	}

	for (int i = 0; i < times; i++) pb->data[i] = x;

	return true;
}


/** Create a block_C_INTEGER using a simple sequence.

	\param pb	The address of a pIntBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
				RAM_ALLOC_C_INTEGER).
	\param from The starting value.
	\param to	The end value (may not be included, is the supremum or infimum when the increment is negative).
	\param by	The increment.

	\return		true if successful, false and log(LOG_MISS, "further details") if not.

	The increment may be negative, in that case "from" must be bigger than "to".

	The type of the block can be changed later from BLOCKTYPE_C_INTEGER to BLOCKTYPE_C_FACTOR or BLOCKTYPE_C_GRADE using
reinterpret_cast_block().
*/
bool jzzBLOCKS::new_block_C_INTEGER_seq(pIntBlock &pb, int from, int to, int by)
{
	int step = by != 0 ? by : 1;	// Silently replace 0 by 1

	int len = step > 0 ? (to - from + step)/step : (from - to - step)/(-step);

	if (len <= 0)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_INTEGER_seq() negative length in seq.");

		return false;
	}

	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_INTEGER, len);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_INTEGER_seq() alloc failed.");

		return false;
	}

	for (int i = 0; i < len; i++)
	{
		pb->data[i]	 = from;
		from		+= step;
	}

	return true;
}


/** Create a block_C_REAL by repeating a value a number of times.

	\param pb	 The address of a pRealBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
				 RAM_ALLOC_C_REAL).
	\param x	 The value to be initialized.
	\param times The number of times to repeat that value == length of the resulting vector.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	The type of the block can be changed later from BLOCKTYPE_C_REAL to BLOCKTYPE_C_TIMESEC using reinterpret_cast_block().
*/
bool jzzBLOCKS::new_block_C_REAL_rep(pRealBlock &pb, double x, int times)
{
	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_REAL, times);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_REAL_rep() alloc failed.");

		return false;
	}

	for (int i = 0; i < times; i++) pb->data[i] = x;

	return true;
}


/** Create a block_C_REAL using a simple sequence.

	\param pb	The address of a pRealBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
				RAM_ALLOC_C_REAL).
	\param from The starting value.
	\param to	The end value (may not be included, is the supremum or infimum when the increment is negative).
	\param by	The increment.

	\return		true if successful, false and log(LOG_MISS, "further details") if not.

	The increment may be negative, in that case "from" must be bigger than "to".

	The type of the block can be changed later from BLOCKTYPE_C_REAL to BLOCKTYPE_C_TIMESEC using reinterpret_cast_block().
*/
bool jzzBLOCKS::new_block_C_REAL_seq(pRealBlock &pb, double from, double to, double by)
{
	double step = by != 0 ? by : 1; // Silently replace 0 by 1

	int len = step > 0 ? (to - from + step + TWO_ORDER_DBL_EPSILON)/step : (from - to - step + TWO_ORDER_DBL_EPSILON)/(-step);

	if (len <= 0)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_REAL_seq() negative length in seq.");

		return false;
	}

	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_REAL, len);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_REAL_seq() alloc failed.");

		return false;
	}

	for (int i = 0; i < len; i++)
	{
		pb->data[i] = from + i*step;
	}

	return true;
}


/** Create a block_C_OFFS_CHARS by repeating a value a number of times.

	\param pb	 The address of a pCharBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(.,
				 RAM_ALLOC_C_OFFS_CHARS).
	\param str	 The string to be copied many times. NULL can be used to indicate NA (also a string of length 0).
	\param times The number of times to repeat that value == length of the resulting vector.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This is not the usual way to create a block_C_OFFS_CHARS since it only allocates the necessary RAM to store the block. It does exactly as
announced, but managing strings is not trivial since the amount of RAM that will be necessary can only be guessed. See functions JAZZALLOC(.,
RAM_ALLOC_C_OFFS_CHARS, .) format_C_OFFS_CHARS(), get_string_idx_C_OFFS_CHARS() and realloc_C_OFFS_CHARS() for proper string management and use
this only when it does exactly what you need (i.e, you have no intentions to further expand the block).
*/
bool jzzBLOCKS::new_block_C_CHARS_rep(pCharBlock &pb, const char * str, int times)
{
	int len = str == NULL ? 0 : strlen(str);

	// 1 for the end of string + end of buffer, 7 for rounding up to 8 bytes.
	unsigned int size = (sizeof(int)*times + sizeof(string_buffer) + len + 1 + 7) & 0xFFFFfff8;

	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_OFFS_CHARS, size);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_CHARS_rep() alloc failed.");

		return false;
	}

	if (!times)
	{
		memset(&pb->data, 0, pb->size);

		return true;
	}

	format_C_OFFS_CHARS(pb, times);							// Cannot fail when alloc is Ok.
	int j = get_string_idx_C_OFFS_CHARS(pb, str, len);		// Cannot fail when alloc is Ok.

	for (int i = 0; i < times; i++) pb->data[i] = j;

	return true;
}


/** Create a block_C_RAW by loading anything into it.

	\param pb	The address of a pRawBlock allocated for the object. The caller is responsible of freeing this with JAZZFREE(., RAM_ALLOC_C_RAW).
	\param ps	The string to be copied many times.
	\param size The size in bytes of the object pointed by ps.

	\return		true if successful, false and log(LOG_MISS, "further details") if not.

	The type of the block can be changed later from BLOCKTYPE_RAW_ANYTHING to any type >= BLOCKTYPE_RAW_ANYTHING using reinterpret_cast_block().
*/
bool jzzBLOCKS::new_block_C_RAW_once(pRawBlock &pb, const void * ps, int size)
{
	bool ok = JAZZALLOC(pb, RAM_ALLOC_C_RAW, size);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_C_RAW_once() alloc failed.");

		return false;
	}

	memcpy(&pb->data, ps, size);

	return true;
}


/** Change the type of an integer, real or raw block.

	\param pb	A pJazzBlock to be changed
	\param type The new type.

	\return		true if the change is possible and was done.

The allowed changes are:

	BLOCKTYPE_C_INTEGER with BLOCKTYPE_C_FACTOR or BLOCKTYPE_C_GRADE in any combination,
	BLOCKTYPE_C_REAL to BLOCKTYPE_C_TIMESEC and vice-versa,
	Any type >= BLOCKTYPE_RAW_ANYTHING to any other type >= BLOCKTYPE_RAW_ANYTHING

	No allocation, conversion or change of block headers other than the type is done. Use this function to check errors, rather that doing the
change directly.
*/
bool jzzBLOCKS::reinterpret_cast_block(pJazzBlock pb, int type)
{
	if (pb->type == type)
		return true;

	switch (pb->type)
	{
		case BLOCKTYPE_C_INTEGER:
		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
			switch (type)
			{
				case BLOCKTYPE_C_INTEGER:
				case BLOCKTYPE_C_FACTOR:
				case BLOCKTYPE_C_GRADE:
					pb->type = type;

					return true;

				default:
					return false;
			}

		case BLOCKTYPE_C_REAL:
		case BLOCKTYPE_C_TIMESEC:
			switch (type)
			{
				case BLOCKTYPE_C_REAL:
				case BLOCKTYPE_C_TIMESEC:
					pb->type = type;

					return true;

				default:
					return false;
			}

		default:
			if (pb->type >= BLOCKTYPE_RAW_ANYTHING && type >= BLOCKTYPE_RAW_ANYTHING)
			{
				pb->type = type;

				return true;
			}
	}

	return false;
}


/** Copy a block to a new JAZZALLOC()ed block.

	\param psrc	 The pJazzBlock to be copied.
	\param pdest The address of a pJazzBlock where a block of the same size will be allocated.

	\return		 true if successful, false and log(LOG_MISS, "further details") if not.

	This function does nothing with the source block, except copying it. The typical use is to create a block from an lmdb block returned by
	block_get() but this will not block_unprotect() the source pointer. It works on any source regardless of being JAZZALLOC()ed or block_get()ed.

	The destination is JAZZALLOC()ed and owned by the caller who must JAZZFREE it later.
*/
bool jzzBLOCKS::new_block_copy (pJazzBlock psrc, pJazzBlock &pdest)
{
	bool ok;
	switch (psrc->type)
	{
		case BLOCKTYPE_C_BOOL:
			ok = JAZZALLOC(pdest, RAM_ALLOC_C_BOOL, psrc->length);

			break;

		case BLOCKTYPE_C_OFFS_CHARS:
			ok	= JAZZALLOC(pdest, RAM_ALLOC_C_OFFS_CHARS, psrc->size);
			ok &= format_C_OFFS_CHARS((pCharBlock)pdest, psrc->length);

			break;

		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			ok	= JAZZALLOC(pdest, RAM_ALLOC_C_INTEGER, psrc->length);
			ok &= reinterpret_cast_block(pdest, psrc->type);

			break;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			ok	= JAZZALLOC(pdest, RAM_ALLOC_C_REAL, psrc->length);
			ok &= reinterpret_cast_block(pdest, psrc->type);

			break;

		case BLOCKTYPE_RAW_ANYTHING:
		case BLOCKTYPE_RAW_STRINGS:
		case BLOCKTYPE_RAW_R_RAW:
		case BLOCKTYPE_RAW_MIME_APK:
		case BLOCKTYPE_RAW_MIME_CSS:
		case BLOCKTYPE_RAW_MIME_CSV:
		case BLOCKTYPE_RAW_MIME_GIF:
		case BLOCKTYPE_RAW_MIME_HTML:
		case BLOCKTYPE_RAW_MIME_IPA:
		case BLOCKTYPE_RAW_MIME_JPG:
		case BLOCKTYPE_RAW_MIME_JS:
		case BLOCKTYPE_RAW_MIME_JSON:
		case BLOCKTYPE_RAW_MIME_MP4:
		case BLOCKTYPE_RAW_MIME_PDF:
		case BLOCKTYPE_RAW_MIME_PNG:
		case BLOCKTYPE_RAW_MIME_TSV:
		case BLOCKTYPE_RAW_MIME_TXT:
		case BLOCKTYPE_RAW_MIME_XAP:
		case BLOCKTYPE_RAW_MIME_XML:
		case BLOCKTYPE_RAW_MIME_ICO:
		case BLOCKTYPE_FUN_R:
		case BLOCKTYPE_FUN_DFQL:
		case BLOCKTYPE_FUN_DFQO:
		case BLOCKTYPE_ALL_ROLES:
		case BLOCKTYPE_ALL_KEYSPACE:
		case BLOCKTYPE_ALL_SECURITY:
		case BLOCKTYPE_MY_LOG_EVENTS:
		case BLOCKTYPE_MY_PROFILING:
		case BLOCKTYPE_MY_SECURITY:
		case BLOCKTYPE_MY_URL_DICT:
		case BLOCKTYPE_SOURCE_ATTRIB:

			ok	= JAZZALLOC(pdest, RAM_ALLOC_C_RAW, psrc->size);
			ok &= reinterpret_cast_block(pdest, psrc->type);

			break;

		default:
			jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_copy() unexpected source type.");

			return false;
	}
	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_block_copy() alloc failed.");

		return false;
	}

	memcpy(&reinterpret_cast<pRawBlock>(pdest)->data, &reinterpret_cast<pRawBlock>(psrc)->data, psrc->size);

	return true;
}

/*	------------------------------------------------------------------------------------------------------
	  R e s i z e	s t r i n g	  v e c t o r s	  a n d	  a d d	  s t r i n g s	  t o	v e c t o r s
------------------------------------------------------------------------------------------------------- */

/** Set the length of a block_C_OFFS_CHARS and initialize its string_buffer[] to contain strings.

	\param pstr	  A just JAZZALLOC()ed block_C_OFFS_CHARS whose length has not been set yet.
	\param length The length of the allocated string vector.

	\return		  true if successful, false and log(LOG_MISS, "further details") if not.

	The procedure to create a block of strings is:

		pCharBlock pt;

		JAZZALLOC(pt, RAM_ALLOC_C_OFFS_CHARS, 16384);// Allocate 16K bytes for a 1000 element vector with many repeated values. Just a guess.
		format_C_OFFS_CHARS(pt, 1000);				 // Initialize and set the length to 1000, the buffer is ready and the data[] vector full of NA

		for(whatever i in 0..999)
			int j = get_string_idx_C_OFFS_CHARS(pt, "My whatever string", strlen(..));
			if (j >= 0)
				pt->data[i] = j;
			else
				realloc_C_OFFS_CHARS(pt, 4096);		// Add extra 4K each time get_string_idx_C_OFFS_CHARS() fails to allocate a string.
*/
bool jzzBLOCKS::format_C_OFFS_CHARS(pCharBlock pstr, int length)
{
	if (pstr->size < sizeof(int)*length + sizeof(string_buffer) + 1)	// One zero behind, but no possible allocation. The block is a block of NA.
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::format_C_OFFS_CHARS() length > allocation even with no string buffer.");

		return false;
	}
	pstr->length = length;

	memset(&pstr->data, JAZZC_NA_STRING, sizeof(int)*length);

	pStringBuff string_buffer = STRING_BUFFER(pstr);

	string_buffer->NA		 = 0;
	string_buffer->EMPTY	 = 0;
	string_buffer->isBig	 = false;
	string_buffer->last_idx	 = sizeof(string_buffer);
	string_buffer->buffer[0] = 0;

	return true;
}


/** Find an existing string in a block, or allocate a new one and return its offset in the string_buffer[].

	\param pstr	  A block_C_OFFS_CHARS containing strings.
	\param string The string to find or allocate in the string_buffer[].
	\param len	  The length of "string". This in mandatory to simplify operation with systems where it is known (R and std::string)

	\return		  The offset to the string if successfully allocated or -1 if allocation failed.

	See format_C_OFFS_CHARS() for an explanation on how block_C_OFFS_CHARS are filled.

	The block_C_OFFS_CHARS is designed to be fast to read or operate when the blocks are already created. Filling blocks is not efficient because
all the (possibly many) strings have to be compared with the new string each time. If that becomes a bottleneck, a more efficient structure like
a map should be used.
*/
int jzzBLOCKS::get_string_idx_C_OFFS_CHARS(pCharBlock pstr, const char * string, int len)
{
	if (string == NULL)
		return JAZZC_NA_STRING;

	if (!len)
		return JAZZC_EMPTY_STRING;

	pStringBuff string_buffer = STRING_BUFFER(pstr);

	char * pt;

	if (string_buffer->isBig)
	{
		pt = (char *)string_buffer + (uintptr_t) string_buffer->last_idx;
	}
	else
	{
		pt = (char *)string_buffer + (uintptr_t) sizeof(string_buffer);

		int t = 0;
		while (pt[0])
		{
			uintptr_t idx = pt - (char *)string_buffer;

			if (!strncmp(string, pt, len + 1)) return idx;

			int slen = strlen(pt);

			pt += slen + 1;

			t++;
		}
		if (t >= STR_SEARCH_BIG_ABOVE) string_buffer->isBig = true;
	}

	if (pstr->size >= (uintptr_t)pt - (uintptr_t)&pstr->data + len + 2)
	{
		uintptr_t idx = pt - (char *)string_buffer;

		strncpy(pt, string, len);
		pt[len] = 0;

		pt += len + 1;

		pt[0] = 0;

		string_buffer->last_idx = idx + len + 1;

		return idx;
	}

	return -1;
}


/** Reallocate a block_C_OFFS_CHARS to a bigger object when more string space is required.
	\param pstr			The address of an existing	pCharBlock of type block_C_OFFS_CHARS. It will be copied to a bigger one and freed.
	\param extra_length The difference between the old allocation size and the new one.

	\return				true if successful, false and log(LOG_MISS, "further details") if not.

	See format_C_OFFS_CHARS() for an explanation on how block_C_OFFS_CHARS are filled.

	The old object is freed and replaced by a new one where its content is copied first.
*/
bool jzzBLOCKS::realloc_C_OFFS_CHARS(pCharBlock &pstr, int extra_length)
{
	pCharBlock pnew;

	bool ok = JAZZALLOC(pnew, RAM_ALLOC_C_OFFS_CHARS, pstr->size + extra_length);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::realloc_C_OFFS_CHARS() alloc failed.");

		return false;
	}

	memcpy(&pnew->data, &pstr->data, pstr->size);

	pnew->length = pstr->length;

	JAZZFREE(pstr, RAM_ALLOC_C_OFFS_CHARS);

	pstr = pnew;

	return true;
}

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
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::open_all_sources().");

		return false;
	}

	MDB_dbi dbi;
	if (int err = mdb_dbi_open(txn, NULL, 0, &dbi))
	{
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::open_all_sources().");

		goto release_txn_and_fail;
	}

	MDB_cursor *cursor;
	if (int err = mdb_cursor_open(txn, dbi, &cursor))
	{
		log_lmdb_err_as_miss(err, "mdb_cursor_open() failed in jzzBLOCKS::open_all_sources().");

		goto release_dbi_and_fail;
	}

	source_open[SYSTEM_SOURCE_SYS] = false;
	source_open[SYSTEM_SOURCE_WWW] = false;

	MDB_val key, data;
	while (!mdb_cursor_get(cursor, &key, &data, MDB_NEXT))
	{
		sourceName name;

		if (!char_to_key((char *) key.mv_data, name))
		{
			jCommons.log(LOG_MISS, "mdb_cursor_get() returned an impossible source name in jzzBLOCKS::open_all_sources().");

			goto release_cur_and_fail;
		}

		if (!strcmp(name.key, "sys"))
		{
			numsources = max(numsources, SYSTEM_SOURCE_SYS + 1);
			source_nam [SYSTEM_SOURCE_SYS] = name;
			source_open[SYSTEM_SOURCE_SYS] = false;
		}
		else
		{
			if (!strcmp(name.key, "www"))
			{
				numsources = max(numsources, SYSTEM_SOURCE_WWW + 1);
				source_nam [SYSTEM_SOURCE_WWW] = name;
				source_open[SYSTEM_SOURCE_WWW] = false;
			}
			else
			{
				numsources = max(numsources, SYSTEM_SOURCE_WWW + 1);
				source_nam [numsources]	  = name;
				source_open[numsources++] = false;
			}
		}
	}

	mdb_cursor_close(cursor);
	mdb_dbi_close(lmdb_env, dbi);
	mdb_txn_abort(txn);

	if (numsources == 0)
	{
		if (!new_source("sys"))
		{
			jCommons.log(LOG_MISS, "new_source('sys') failed in jzzBLOCKS::open_all_sources().");

			return false;
		}
		if (!new_source("www"))
		{
			jCommons.log(LOG_MISS, "new_source('www') failed in jzzBLOCKS::open_all_sources().");

			return false;
		}
	}
	else
	{
		if (numsources < 2 || strcmp(source_nam[SYSTEM_SOURCE_SYS].key, "sys") || strcmp(source_nam[SYSTEM_SOURCE_WWW].key, "www"))
		{
			jCommons.log(LOG_MISS, "sources 'sys' or 'www' not found in jzzBLOCKS::open_all_sources().");

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
	if (idx < 0 || idx >= numsources)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::source_name(): wrong index.");

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
	int imax = source_idx[tenbits(name)];

	for (int i = imax; i >= 0; i--)
	{
		if (!strcmp(name, source_nam[i].key)) return i;
	}

	return -1;
}


/** Create a new lmbd dbi and add it as a new source to the source[] vector.
	\param name The name of the source to be added.

	\return true if successful, false and log(LOG_MISS, "further details") if not.

	NOTE: new_source() is not thread safe! Unsafe use of: numsources, source_nam, source_open, source_dbi.
*/
bool jzzBLOCKS::new_source(const char * name)
{
	if (numsources >= MAX_POSSIBLE_SOURCES)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_source(): too many sources.");

		return false;
	}

	sourceName snam;

	if (!char_to_key(name, snam))
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_source(): invalid name.");

		return false;
	}

	if (get_source_idx(name) >= 0)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::new_source(): source already exists.");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::new_source().");

		return false;
	}

	int idx = numsources;

	block_SourceAttributes src_attr = {};

	if (int err = mdb_dbi_open(txn, snam.key, MDB_CREATE, &source_dbi[idx]))
	{
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::new_source().");

		goto release_txn_and_fail;
	}

	source_nam [idx] = snam;
	source_open[idx] = true;

	MDB_val lkey, data;

	persistedKey key;
	key.key[0] = '.';

	src_attr.type	 = BLOCKTYPE_SOURCE_ATTRIB;
	src_attr.length	 = 1;
	src_attr.size	 = sizeof(block_SourceAttributes) - sizeof(jzzBlockHeader);
	src_attr.version = 1001;
	src_attr.hash64	 = 0;

	lkey.mv_size = 1;
	lkey.mv_data = &key.key;
	data.mv_size = sizeof(block_SourceAttributes);
	data.mv_data = &src_attr;

	if (int err = mdb_put(txn, source_dbi[idx], &lkey, &data, 0))
	{
		log_lmdb_err_as_miss(err, "mdb_put() failed in jzzBLOCKS::new_source().");

		goto release_dbi_and_fail;
	}

	if (int err = mdb_txn_commit(txn))
	{
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

	if (idx < 0)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::kill_source(): source does not exist.");

		return false;
	}

	if (idx < 2)
	{
		jCommons.log(LOG_MISS, "jzzBLOCKS::kill_source(): attempt to kill system source.");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::kill_source().");

		return false;
	}

	if (!source_open[idx])
	{
		if (!internal_open_dbi(idx))
		{
			jCommons.log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::kill_source().");

			goto release_txn_and_fail;
		}
	}

	if (int err = mdb_drop(txn, source_dbi[idx], 1))
	{
		log_lmdb_err_as_miss(err, "mdb_drop() failed in jzzBLOCKS::kill_source().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::kill_source().");

		goto release_txn_and_fail;
	}

	for (int i = idx;i < numsources - 1; i++)
	{
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
//TODO: Remove remark

	for(int idx = 0; idx < numsources; idx++)
		if (source_open[idx]) mdb_dbi_close(lmdb_env, source_dbi[idx]);

	numsources = 0;

//TODO: Flushing mechanism (Is it necessary?)
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
	if (source < 0 || source >= numsources)
	{
		jCommons.log(LOG_MISS, "Invalid source in jzzBLOCKS::block_put().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_put().");

		return false;
	}

	if (!source_open[source])
	{
		if (!internal_open_dbi(source))
		{
			jCommons.log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_put().");

			goto release_txn_and_fail;
		}
	}

	MDB_val lkey, data;

	lkey.mv_size = strlen(key.key);
	lkey.mv_data = (void *) &key;
	data.mv_size = block->size + sizeof(jzzBlockHeader);
	data.mv_data = block;

	if (int err = mdb_put(txn, source_dbi[source], &lkey, &data, 0))
	{
		log_lmdb_err_as_miss(err, "mdb_put() failed in jzzBLOCKS::block_put().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn))
	{
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
	if (source < 0 || source >= numsources)
	{
		jCommons.log(LOG_MISS, "Invalid source in jzzBLOCKS::block_get().");

		return false;
	}

	int prot_idx = lock_get_protect_slot();
	if (prot_idx < 0)
	{
		jCommons.log(LOG_MISS, "No slot in block protection found in jzzBLOCKS::block_get().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn))
	{
		lock_release_protect_slot(prot_idx);

		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_get().");

		return false;
	}

	if (!source_open[source])
	{
		if (!internal_open_dbi(source))
		{
			jCommons.log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_get().");

			goto release_txn_and_fail;
		}
	}

	MDB_val lkey, data;

	lkey.mv_size = strlen(key.key);
	lkey.mv_data = (void *) &key;

	if (int err = mdb_get(txn, source_dbi[source], &lkey, &data))
	{
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
	if (source < 0 || source >= numsources)
	{
		jCommons.log(LOG_MISS, "Invalid source in jzzBLOCKS::block_kill().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::block_kill().");

		return false;
	}

	if (!source_open[source])
	{
		if (!internal_open_dbi(source))
		{
			jCommons.log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::block_kill().");

			goto release_txn_and_fail;
		}
	}

	MDB_val lkey;

	lkey.mv_size = strlen(key.key);
	lkey.mv_data = (void *) &key;

	if (int err = mdb_del(txn, source_dbi[source], &lkey, NULL))
	{
		log_lmdb_err_as_miss(err, "mdb_del() failed in jzzBLOCKS::block_kill().");

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_commit() failed in jzzBLOCKS::block_kill().");

		goto release_txn_and_fail;
	}

	return true;


release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}

/*	----------------------------------------------------------
	  F u n c t i o n s	  o n	b l o c k	u t i l s .
----------------------------------------------------------- */

/** Hash a block in RAM

	\param pblock	 A correctly allocated pJazzBlock with a valid header (size).

	Writes the output to the hash64 field of the header.
*/
void jzzBLOCKS::hash_block (pJazzBlock pblock)
{
	pblock->hash64 = MurmurHash64A(&reinterpret_cast<pRawBlock>(pblock)->data, pblock->size);
}


/** Check if the headers of two blocks are identical

	\param pb1 A correctly allocated pJazzBlock with a valid header
	\param pb2 Another correctly allocated pJazzBlock with a valid header
	\return	   true if headers are identical

	Has no protection mechanism for invalid pointers.
*/
bool jzzBLOCKS::compare_headers (pJazzBlock pb1, pJazzBlock pb2)
{
	return pb1->type   == pb2->type
		&& pb1->length == pb2->length
		&& pb1->size   == pb2->size
		&& pb1->flags  == pb2->flags
		&& pb1->hash64 == pb2->hash64;
}


/** Check if the content of two blocks is identical

	\param pb1 A correctly allocated pJazzBlock with a valid header
	\param pb2 Another correctly allocated pJazzBlock with a valid header
	\return	   true if content (size bytes at data) is identical

	Has no protection mechanism for invalid pointers or corrupted headers. This does NOT check the headers (use compare_headers() for that.
*/
bool jzzBLOCKS::compare_content (pJazzBlock pb1, pJazzBlock pb2)
{
	if (pb1->size != pb2->size)
		return false;

	return memcmp(&reinterpret_cast<pRawBlock>(pb1)->data, &reinterpret_cast<pRawBlock>(pb2)->data, pb1->size) == 0;
}

/*	----------------------------------------------------------
	  F u n c t i o n s	  a b o u t	  k e y	  u t i l s .
----------------------------------------------------------- */

/// Key before any other. Is set by set_first_key() and a call to next_key() returns the first key.
persistedKey KEY_SPACE_START = {"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};

///< is_last_key() compares against this value returned by next_key()after the last key is passed.
persistedKey KEY_SPACE_END	 = {"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"};


/** Convert a char * into a persistedKey or fail if incorrect chars or incorrect length found.
	\param pkey The source key as char *
	\param key	The destination persistedKey

	\return		  true if successful, does not log.
*/
bool jzzBLOCKS::char_to_key(const char * pkey, persistedKey &key)
{
	if (!pkey[0]) return false;

	int i = 0;

	while (char c = pkey[i++])	// ... 0..9:;<=>?@A..Z[\]^_`a..z ...
	{
		if (i == MAX_KEY_LENGTH	 || c <	 '0' || c >	 'z') return false;
		if (c <= '9' || c >= 'a' || c == '_') continue;
		if (c <	 'A' || c > 'Z') return false;
	}

	strcpy(key.key, pkey);

	return true;
}


/** Compare two (persistedKey) keys as char * with strcmp().
	\param key1 The first key.
	\param key2 The second key.

	\return		Same as strcmp()
*/
int jzzBLOCKS::strcmp_keys(const persistedKey key1, const persistedKey key2)
{
	return strcmp((char *) &key1.key, (char *) &key2.key);
}


/** Scan all the keys stored in a source.
	\param source The index of an open source. (As returned by get_source_idx().)
	\param key	  Input: The last key returned by a previous call to next_key() or KEY_SPACE_START to search the first one.
				  Output: The next key after the input key or KEY_SPACE_END if the previous key was the last.

	\return		  true if successful, false and log(LOG_MISS, "further details") if errors such as invalid source happen.

	There are two functions set_first_key() and is_last_key() to give an simple approach to search:

		set_first_key(key);
		while (!is_last_key(key) next_key(idx, key);

	Reaching the end once is not an error and does not return false, but attempting to search after KEY_SPACE_END does.
*/
bool jzzBLOCKS::next_key(int source, persistedKey &key)
{
	if (source < 0 || source >= numsources)
	{
		jCommons.log(LOG_MISS, "Invalid source in jzzBLOCKS::next_key().");

		return false;
	}

	MDB_txn * txn;
	if (int err = mdb_txn_begin(lmdb_env, NULL, MDB_RDONLY, &txn))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::next_key().");

		return false;
	}

	if (!source_open[source])
	{
		if (!internal_open_dbi(source))
		{
			jCommons.log(LOG_MISS, "internal_open_dbi() failed in jzzBLOCKS::next_key().");

			goto release_txn_and_fail;
		}
	}

	MDB_cursor *cursor;
	if (int err = mdb_cursor_open(txn, source_dbi[source], &cursor))
	{
		log_lmdb_err_as_miss(err, "mdb_cursor_open() failed in jzzBLOCKS::next_key().");

		goto release_txn_and_fail;
	}

	MDB_val lkey, data;

	if (key.key[0])
	{
		lkey.mv_size = strlen(key.key);
		lkey.mv_data = (void *) &key;

		if (int err = mdb_cursor_get(cursor, &lkey, &data, MDB_SET))
		{
			log_lmdb_err_as_miss(err, "mdb_cursor_get(,,, MDB_SET) failed in jzzBLOCKS::next_key().");

			goto release_cur_and_fail;
		}
	}

	if (int err = mdb_cursor_get(cursor, &lkey, &data, MDB_NEXT))
	{
		if (err == MDB_NOTFOUND)
		{
			key = KEY_SPACE_END;

			goto release_cur_and_suceed;
		}

		log_lmdb_err_as_miss(err, "mdb_cursor_get(,,, MDB_NEXT) failed with other than MDB_NOTFOUND in jzzBLOCKS::next_key().");

		goto release_cur_and_fail;
	}

	if (lkey.mv_size == 1 && reinterpret_cast<char *>(lkey.mv_data)[0] == '.')
	{
		if (int err = mdb_cursor_get(cursor, &lkey, &data, MDB_NEXT))
		{
			if (err == MDB_NOTFOUND)
			{
				key = KEY_SPACE_END;

				goto release_cur_and_suceed;
			}

			log_lmdb_err_as_miss(err, "mdb_cursor_get(,,, MDB_NEXT) failed with other than MDB_NOTFOUND in jzzBLOCKS::next_key().");

			goto release_cur_and_fail;
		}
	}

	if (lkey.mv_size >= MAX_KEY_LENGTH)
	{
		jCommons.log(LOG_MISS, "mdb_cursor_get() returned an impossible key value in jzzBLOCKS::next_key().");

		goto release_cur_and_fail;
	}

	memcpy(&key, lkey.mv_data, lkey.mv_size);
	key.key[lkey.mv_size] = 0;

release_cur_and_suceed:

	mdb_cursor_close(cursor);

	mdb_txn_abort(txn);

	return true;


release_cur_and_fail:

	mdb_cursor_close(cursor);

release_txn_and_fail:

	mdb_txn_abort(txn);

	return false;
}


/** Initialize a key with the value in KEY_SPACE_START to start a next_key() search from the beginning of a source.
	\param key The first key.
*/
void jzzBLOCKS::set_first_key(persistedKey &key)
{
	key = KEY_SPACE_START;
}


/** Compare a key with KEY_SPACE_END.
	\param key The persistedKey to be compared.

	\return	   true if the key is identical to KEY_SPACE_END.
*/
bool jzzBLOCKS::is_last_key(const persistedKey &key)
{
	return !strcmp_keys(key, KEY_SPACE_END);
}

/*	----------------------------------------------------------
	  F o r e i g n	  p o i n t e r	  m a n a g e m e n t .
----------------------------------------------------------- */

/** Not using lock_despite the name in current version (see WARNING in block_unprotect() & block_get().)
*/
int jzzBLOCKS::lock_get_protect_slot()
{
	int i = prot_SP++;

	if (i >= MAX_PROTECTED_BLOCKS)
	{
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

	if (idx == prot_SP - 1)
	{
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
	for (int i = prot_SP - 1; i >= 0; i--)
	{
		if (protect_ptr[i] == block)
		{
			MDB_txn * txn = protect_txn[i];

			lock_release_protect_slot(i);

			if (txn) mdb_txn_abort(txn);

			return true;
		}
	}

	jCommons.log(LOG_MISS, "'block' not found in 'protect_ptr[]' in jzzBLOCKS::block_unprotect().");

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
	if (int err = mdb_txn_begin(lmdb_env, NULL, 0, &txo))
	{
		log_lmdb_err_as_miss(err, "mdb_txn_begin() failed in jzzBLOCKS::internal_open_dbi().");

		return false;
	}

	if (int err = mdb_dbi_open(txo, source_nam[source].key, 0, &source_dbi[source]))
	{
		log_lmdb_err_as_miss(err, "mdb_dbi_open() failed in jzzBLOCKS::internal_open_dbi().");

		mdb_txn_abort(txo);

		goto release_txn_and_fail;
	}

	if (int err = mdb_txn_commit(txo))
	{
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

	for (int i = 0; i < numsources; i++)
	{
		int j = tenbits(source_nam[i].key);

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
	switch (err)
	{
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

	jCommons.log(LOG_ERROR, errmsg);
	jCommons.log(LOG_MISS, msg);
}

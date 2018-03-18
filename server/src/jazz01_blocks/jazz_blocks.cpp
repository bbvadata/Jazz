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

/*	--------------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST

SCENARIO("Write/read some blocks in new sources jzzBLOCKS.")
{
	jzzBLOCKS jb;
	jCommons.load_config_file("./config/jazz_config.ini");

	jCommons.debug_config_put("JazzPERSISTENCE.MDB_PERSISTENCE_PATH", TEST_LMDB_PATH);

	REQUIRE(jb.start());

	REQUIRE(jb.new_source("a_bool"));
	REQUIRE(jb.new_source("some_int"));
	REQUIRE(jb.new_source("some_double"));
	REQUIRE(jb.new_source("a_string"));
	REQUIRE(jb.new_source("a_raw"));

	GIVEN("Some block_C_OFFS_CHARS")
	{
		pBoolBlock pboo;
		pIntBlock  pbi1, pbi2;
		pRealBlock pbr1, pbr2;
		pCharBlock pbst;
		pRawBlock  pbrw;

		REQUIRE(jb.new_block_C_BOOL_rep	  (pboo, true, 3));
		REQUIRE(jb.new_block_C_INTEGER_rep(pbi1, 123, 4));
		REQUIRE(jb.new_block_C_INTEGER_seq(pbi2, 6, 15, 3));
		REQUIRE(jb.new_block_C_REAL_rep	  (pbr1, 1.234, 5));
		REQUIRE(jb.new_block_C_REAL_seq	  (pbr2, 6, 7, 0.1));
		REQUIRE(jb.new_block_C_CHARS_rep  (pbst, "Penny!", 3));
		REQUIRE(jb.new_block_C_RAW_once	  (pbrw, "Something", strlen("Something")));

		persistedKey key;
		jb.char_to_key("boo", key);
		REQUIRE(jb.block_put(jb.get_source_idx("a_bool"),	   key, (pJazzBlock) pboo));
		jb.char_to_key("in1", key);
		REQUIRE(jb.block_put(jb.get_source_idx("some_int"),	   key, (pJazzBlock) pbi1));
		jb.char_to_key("in2", key);
		REQUIRE(jb.block_put(jb.get_source_idx("some_int"),	   key, (pJazzBlock) pbi2));
		jb.char_to_key("rr1", key);
		REQUIRE(jb.block_put(jb.get_source_idx("some_double"), key, (pJazzBlock) pbr1));
		jb.char_to_key("rr2", key);
		REQUIRE(jb.block_put(jb.get_source_idx("some_double"), key, (pJazzBlock) pbr2));
		jb.char_to_key("str", key);
		REQUIRE(jb.block_put(jb.get_source_idx("a_string"),	   key, (pJazzBlock) pbst));
		jb.char_to_key("raw", key);
		REQUIRE(jb.block_put(jb.get_source_idx("a_raw"),	   key, (pJazzBlock) pbrw));

		THEN("The content is identical")
		{
			pJazzBlock pjzz;

			jb.char_to_key("boo", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_bool"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pboo));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pboo));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("in1", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_int"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbi1));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbi1));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("in2", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_int"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbi2));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbi2));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("rr1", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_double"), key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbr1));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbr1));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("rr2", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_double"), key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbr2));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbr2));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("str", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_string"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbst));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbst));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("raw", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_raw"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbrw));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbrw));
			REQUIRE(jb.block_unprotect(pjzz));
		}

		JAZZFREE(pboo, AUTOTYPEBLOCK(pboo));
		JAZZFREE(pbi1, AUTOTYPEBLOCK(pbi1));
		JAZZFREE(pbi2, AUTOTYPEBLOCK(pbi2));
		JAZZFREE(pbr1, AUTOTYPEBLOCK(pbr1));
		JAZZFREE(pbr2, AUTOTYPEBLOCK(pbr2));
		JAZZFREE(pbst, AUTOTYPEBLOCK(pbst));
		JAZZFREE(pbrw, AUTOTYPEBLOCK(pbrw));

		REQUIRE(pboo == NULL);
		REQUIRE(pbi1 == NULL);
		REQUIRE(pbi2 == NULL);
		REQUIRE(pbr1 == NULL);
		REQUIRE(pbr2 == NULL);
		REQUIRE(pbst == NULL);
		REQUIRE(pbrw == NULL);

		REQUIRE(jazzPtrTrackClose());
	}

	jb.stop();
}


SCENARIO("Read again previous blocks in new sources jzzBLOCKS.")
{
//TODO: Enable this when TEST_PERSISTED_MODE is implemented.
#ifdef TEST_PERSISTED_MODE
	jzzBLOCKS jb;
	jCommons.load_config_file("./config/jazz_config.ini");

	jCommons.debug_config_put("JazzPERSISTENCE.MDB_PERSISTENCE_PATH", TEST_LMDB_PATH);

	REQUIRE(jb.start());

	GIVEN("Open again block_C_OFFS_CHARS")
	{
		pBoolBlock pboo;
		pIntBlock  pbi1, pbi2;
		pRealBlock pbr1, pbr2;
		pCharBlock pbst;
		pRawBlock  pbrw;

		REQUIRE(jb.new_block_C_BOOL_rep	  (pboo, true, 3));
		REQUIRE(jb.new_block_C_INTEGER_rep(pbi1, 123, 4));
		REQUIRE(jb.new_block_C_INTEGER_seq(pbi2, 6, 15, 3));
		REQUIRE(jb.new_block_C_REAL_rep	  (pbr1, 1.234, 5));
		REQUIRE(jb.new_block_C_REAL_seq	  (pbr2, 6, 7, 0.1));
		REQUIRE(jb.new_block_C_CHARS_rep  (pbst, "Penny!", 3));
		REQUIRE(jb.new_block_C_RAW_once	  (pbrw, "Something", strlen("Something")));

		persistedKey key;

		THEN("The content is identical")
		{
			pJazzBlock pjzz;

			jb.char_to_key("boo", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_bool"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pboo));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pboo));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("in1", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_int"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbi1));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbi1));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("in2", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_int"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbi2));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbi2));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("rr1", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_double"), key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbr1));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbr1));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("rr2", key);
			REQUIRE(jb.block_get(jb.get_source_idx("some_double"), key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbr2));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbr2));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("str", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_string"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbst));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbst));
			REQUIRE(jb.block_unprotect(pjzz));

			jb.char_to_key("raw", key);
			REQUIRE(jb.block_get(jb.get_source_idx("a_raw"),	   key, pjzz));
			REQUIRE(jb.compare_headers(pjzz, (pJazzBlock) pbrw));
			REQUIRE(jb.compare_content(pjzz, (pJazzBlock) pbrw));
			REQUIRE(jb.block_unprotect(pjzz));
		}

		JAZZFREE(pboo, AUTOTYPEBLOCK(pboo));
		JAZZFREE(pbi1, AUTOTYPEBLOCK(pbi1));
		JAZZFREE(pbi2, AUTOTYPEBLOCK(pbi2));
		JAZZFREE(pbr1, AUTOTYPEBLOCK(pbr1));
		JAZZFREE(pbr2, AUTOTYPEBLOCK(pbr2));
		JAZZFREE(pbst, AUTOTYPEBLOCK(pbst));
		JAZZFREE(pbrw, AUTOTYPEBLOCK(pbrw));

		REQUIRE(pboo == NULL);
		REQUIRE(pbi1 == NULL);
		REQUIRE(pbi2 == NULL);
		REQUIRE(pbr1 == NULL);
		REQUIRE(pbr2 == NULL);
		REQUIRE(pbst == NULL);
		REQUIRE(pbrw == NULL);

		REQUIRE(jazzPtrTrackClose());
	}

	jb.stop();
#endif
}


SCENARIO("Testing jzzBLOCKS.")
{
	jzzBLOCKS jb;
	jCommons.load_config_file("./config/jazz_config.ini");
	REQUIRE(jb.start());

	GIVEN("A configured, started and running jzzBLOCKS object.")
	{
		pBoolBlock pboo;
		pIntBlock  pii1, pii2, pii3, pii4, pii5, pii6, pii7, pii8, pii9, piiA, piiB, piiC, piiD, piiE, piiF;
		pRealBlock prr1, prr2, prr3, prr4, prr5, prr6, prr7, prr8, prr9, prrA, prrB, prrC, prrD, prrE, prrF;
		pCharBlock pss1, pss2, pss3;
		pRawBlock  praw;

		bool ok_pboo = jb.new_block_C_BOOL_rep(pboo, true, 50);

		bool ok_pii1 = jb.new_block_C_INTEGER_rep(pii1, 123456, 50);

		bool ok_pii2 = jb.new_block_C_INTEGER_seq(pii2,	 1,	 5);		// 1,2,3,4,5
		bool ok_pii3 = jb.new_block_C_INTEGER_seq(pii3,	 1,	 5, 0);		// 1,2,3,4,5 (silently replace 0 by 1)
		bool ok_pii4 = jb.new_block_C_INTEGER_seq(pii4,	 2, 11, 3);		// 2,5,8,11
		bool ok_pii5 = jb.new_block_C_INTEGER_seq(pii5,	 2, 15, 3);		// 2,5,8,11,14
		bool ok_pii6 = jb.new_block_C_INTEGER_seq(pii6,	 2, 16, 3);		// 2,5,8,11,14
		bool ok_pii7 = jb.new_block_C_INTEGER_seq(pii7,	 2, 17, 3);		// 2,5,8,11,14,17
		bool ok_pii8 = jb.new_block_C_INTEGER_seq(pii8, 17,	 2,-3);		// 17,14,11,8,5,2
		bool ok_pii9 = jb.new_block_C_INTEGER_seq(pii9, 17,	 3,-3);		// 17,14,11,8,5
		bool ok_piiA = jb.new_block_C_INTEGER_seq(piiA, 17,	 4,-3);		// 17,14,11,8,5
		bool ok_piiB = jb.new_block_C_INTEGER_seq(piiB, 17,	 5,-3);		// 17,14,11,8,5
		bool ok_piiC = jb.new_block_C_INTEGER_seq(piiC,	 5,	 7, 3);		// 5
		bool ok_piiD = jb.new_block_C_INTEGER_seq(piiD, 17, 15,-3);		// 17
		bool ok_piiE = jb.new_block_C_INTEGER_seq(piiE,	 5,	 7,-3);		// Error
		bool ok_piiF = jb.new_block_C_INTEGER_seq(piiF, 17, 15, 3);		// Error

		bool ok_prr1 = jb.new_block_C_REAL_rep(prr1, 3.141592, 50);

		bool ok_prr2 = jb.new_block_C_REAL_seq(prr2,	1.1,  5.1);			// 1.1, 2.1, 3.1, 4.1, 5.1
		bool ok_prr3 = jb.new_block_C_REAL_seq(prr3,	1.1,  5.1,	 0);	// 1.1, 2.1, 3.1, 4.1, 5.1 (silently replace 0 by 1)
		bool ok_prr4 = jb.new_block_C_REAL_seq(prr4,	2.0,  3.5,	 0.3);	// 2, 2.3, 2.6, 2.9, 3.2, 3.5
		bool ok_prr5 = jb.new_block_C_REAL_seq(prr5,	2.0,  3.49,	 0.3);	// 2, 2.3, 2.6, 2.9, 3.2
		bool ok_prr6 = jb.new_block_C_REAL_seq(prr6,	2.0,  3.21,	 0.3);	// 2, 2.3, 2.6, 2.9, 3.2
		bool ok_prr7 = jb.new_block_C_REAL_seq(prr7,	2.0,  3.2,	 0.3);	// 2, 2.3, 2.6, 2.9, 3.2
		bool ok_prr8 = jb.new_block_C_REAL_seq(prr8, 17.0,	15.8, -0.3);	// 17, 16.7, 16.4, 16.1, 15.8
		bool ok_prr9 = jb.new_block_C_REAL_seq(prr9, 17.0,	15.81,-0.3);	// 17, 16.7, 16.4, 16.1
		bool ok_prrA = jb.new_block_C_REAL_seq(prrA, -3.0, -4.0,	-0.3);	// -3, -3.3, -3.6, -3.9
		bool ok_prrB = jb.new_block_C_REAL_seq(prrB, -3.0, -3.9,	-0.3);	// -3, -3.3, -3.6, -3.9
		bool ok_prrC = jb.new_block_C_REAL_seq(prrC,	5.0,  5.1,	 0.3);	// 5
		bool ok_prrD = jb.new_block_C_REAL_seq(prrD, 17.0, 16.9,	-0.3);	// 17
		bool ok_prrE = jb.new_block_C_REAL_seq(prrE,	5.0,  7.0,	-0.3);	// Error
		bool ok_prrF = jb.new_block_C_REAL_seq(prrF, 17.0, 15.0,	 0.3);	// Error

		static const char* const sss[] = {"", "Hello, world!"};

		bool ok_pss1 = jb.new_block_C_CHARS_rep(pss1, NULL,	  20);
		bool ok_pss2 = jb.new_block_C_CHARS_rep(pss2, sss[0], 20);
		bool ok_pss3 = jb.new_block_C_CHARS_rep(pss3, sss[1], 20);

		bool ok_praw = jb.new_block_C_RAW_once(praw, sss[1], strlen(sss[1]));

		REQUIRE(ok_pboo);
		REQUIRE(pboo->data[0]);
		REQUIRE(pboo->data[49]);
		REQUIRE(pboo->length == 50);
		REQUIRE(pboo->size	 == 50);
		REQUIRE(pboo->type	 == BLOCKTYPE_C_BOOL);

		REQUIRE(ok_pii1);
		REQUIRE(pii1->data[0]  == 123456);
		REQUIRE(pii1->data[49] == 123456);
		REQUIRE(pii1->length   == 50);
		REQUIRE(pii1->size	   == 200);
		REQUIRE(pii1->type	   == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii2);
		REQUIRE(pii2->data[0] ==  1);
		REQUIRE(pii2->data[1] ==  2);
		REQUIRE(pii2->data[3] ==  4);
		REQUIRE(pii2->data[4] ==  5);
		REQUIRE(pii2->length  ==  5);
		REQUIRE(pii2->size	  == 20);
		REQUIRE(pii2->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii3);
		REQUIRE(pii3->data[0] ==  1);
		REQUIRE(pii3->data[1] ==  2);
		REQUIRE(pii3->data[3] ==  4);
		REQUIRE(pii3->data[4] ==  5);
		REQUIRE(pii3->length  ==  5);
		REQUIRE(pii3->size	  == 20);
		REQUIRE(pii3->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii4);
		REQUIRE(pii4->data[0] ==  2);
		REQUIRE(pii4->data[1] ==  5);
		REQUIRE(pii4->data[2] ==  8);
		REQUIRE(pii4->data[3] == 11);
		REQUIRE(pii4->length  ==  4);
		REQUIRE(pii4->size	  == 16);
		REQUIRE(pii4->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii5);
		REQUIRE(pii5->data[0] ==  2);
		REQUIRE(pii5->data[1] ==  5);
		REQUIRE(pii5->data[3] == 11);
		REQUIRE(pii5->data[4] == 14);
		REQUIRE(pii5->length  ==  5);
		REQUIRE(pii5->size	  == 20);
		REQUIRE(pii5->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii6);
		REQUIRE(pii6->data[0] ==  2);
		REQUIRE(pii6->data[1] ==  5);
		REQUIRE(pii6->data[3] == 11);
		REQUIRE(pii6->data[4] == 14);
		REQUIRE(pii6->length  ==  5);
		REQUIRE(pii6->size	  == 20);
		REQUIRE(pii6->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii7);
		REQUIRE(pii7->data[0] ==  2);
		REQUIRE(pii7->data[1] ==  5);
		REQUIRE(pii7->data[4] == 14);
		REQUIRE(pii7->data[5] == 17);
		REQUIRE(pii7->length  ==  6);
		REQUIRE(pii7->size	  == 24);
		REQUIRE(pii7->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii8);
		REQUIRE(pii8->data[0] == 17);
		REQUIRE(pii8->data[1] == 14);
		REQUIRE(pii8->data[4] ==  5);
		REQUIRE(pii8->data[5] ==  2);
		REQUIRE(pii8->length  ==  6);
		REQUIRE(pii8->size	  == 24);
		REQUIRE(pii8->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_pii9);
		REQUIRE(pii9->data[0] == 17);
		REQUIRE(pii9->data[1] == 14);
		REQUIRE(pii9->data[3] ==  8);
		REQUIRE(pii9->data[4] ==  5);
		REQUIRE(pii9->length  ==  5);
		REQUIRE(pii9->size	  == 20);
		REQUIRE(pii9->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_piiA);
		REQUIRE(piiA->data[0] == 17);
		REQUIRE(piiA->data[1] == 14);
		REQUIRE(piiA->data[3] ==  8);
		REQUIRE(piiA->data[4] ==  5);
		REQUIRE(piiA->length  ==  5);
		REQUIRE(piiA->size	  == 20);
		REQUIRE(piiA->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_piiB);
		REQUIRE(piiB->data[0] == 17);
		REQUIRE(piiB->data[1] == 14);
		REQUIRE(piiB->data[3] ==  8);
		REQUIRE(piiB->data[4] ==  5);
		REQUIRE(piiB->length  ==  5);
		REQUIRE(piiB->size	  == 20);
		REQUIRE(piiB->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_piiC);
		REQUIRE(piiC->data[0] ==  5);
		REQUIRE(piiC->length  ==  1);
		REQUIRE(piiC->size	  ==  4);
		REQUIRE(piiC->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(ok_piiD);
		REQUIRE(piiD->data[0] == 17);
		REQUIRE(piiD->length  ==  1);
		REQUIRE(piiD->size	  ==  4);
		REQUIRE(piiD->type	  == BLOCKTYPE_C_INTEGER);

		REQUIRE(!ok_piiE);
		REQUIRE(!ok_piiF);

		REQUIRE(ok_prr1);
		REQUIRE(prr1->data[0]  == 3.141592);
		REQUIRE(prr1->data[49] == 3.141592);
		REQUIRE(prr1->length   == 50);
		REQUIRE(prr1->size	   == 400);
		REQUIRE(prr1->type	   == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr2);
		REQUIRE(prr2->data[0] ==  1.1);
		REQUIRE(prr2->data[1] ==  2.1);
		REQUIRE(prr2->data[3] ==  4.1);
		REQUIRE(prr2->data[4] ==  5.1);
		REQUIRE(prr2->length  ==  5);
		REQUIRE(prr2->size	  == 40);
		REQUIRE(prr2->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr3);
		REQUIRE(prr3->data[0] ==  1.1);
		REQUIRE(prr3->data[1] ==  2.1);
		REQUIRE(prr3->data[3] ==  4.1);
		REQUIRE(prr3->data[4] ==  5.1);
		REQUIRE(prr3->length  ==  5);
		REQUIRE(prr3->size	  == 40);
		REQUIRE(prr3->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr4);
		REQUIRE(prr4->data[0] ==  2.0);
		REQUIRE(prr4->data[1] ==  2.3);
		REQUIRE(prr4->data[4] ==  3.2);
		REQUIRE(prr4->data[5] ==  3.5);
		REQUIRE(prr4->length  ==  6);
		REQUIRE(prr4->size	  == 48);
		REQUIRE(prr4->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr5);
		REQUIRE(prr5->data[0] ==  2.0);
		REQUIRE(prr5->data[1] ==  2.3);
		REQUIRE(prr5->data[3] ==  2.9);
		REQUIRE(prr5->data[4] ==  3.2);
		REQUIRE(prr5->length  ==  5);
		REQUIRE(prr5->size	  == 40);
		REQUIRE(prr5->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr6);
		REQUIRE(prr6->data[0] ==  2.0);
		REQUIRE(prr6->data[1] ==  2.3);
		REQUIRE(prr6->data[3] ==  2.9);
		REQUIRE(prr6->data[4] ==  3.2);
		REQUIRE(prr6->length  ==  5);
		REQUIRE(prr6->size	  == 40);
		REQUIRE(prr6->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr7);
		REQUIRE(prr7->data[0] ==  2.0);
		REQUIRE(prr7->data[1] ==  2.3);
		REQUIRE(prr7->data[3] ==  2.9);
		REQUIRE(prr7->data[4] ==  3.2);
		REQUIRE(prr7->length  ==  5);
		REQUIRE(prr7->size	  == 40);
		REQUIRE(prr7->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr8);
		REQUIRE(prr8->data[0] == 17.0);
		REQUIRE(prr8->data[1] == 16.7);
		REQUIRE(prr8->data[3] == 16.1);
		REQUIRE(prr8->data[4] == 15.8);
		REQUIRE(prr8->length  ==  5);
		REQUIRE(prr8->size	  == 40);
		REQUIRE(prr8->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prr9);
		REQUIRE(prr9->data[0] == 17.0);
		REQUIRE(prr9->data[1] == 16.7);
		REQUIRE(prr9->data[2] == 16.4);
		REQUIRE(prr9->data[3] == 16.1);
		REQUIRE(prr9->length  ==  4);
		REQUIRE(prr9->size	  == 32);
		REQUIRE(prr9->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prrA);
		REQUIRE(prrA->data[0] == -3.0);
		REQUIRE(prrA->data[1] == -3.3);
		REQUIRE(prrA->data[2] == -3.6);
		REQUIRE(prrA->data[3] == -3.9);
		REQUIRE(prrA->length  ==  4);
		REQUIRE(prrA->size	  == 32);
		REQUIRE(prrA->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prrB);
		REQUIRE(prrB->data[0] == -3.0);
		REQUIRE(prrB->data[1] == -3.3);
		REQUIRE(prrB->data[2] == -3.6);
		REQUIRE(prrB->data[3] == -3.9);
		REQUIRE(prrB->length  ==  4);
		REQUIRE(prrB->size	  == 32);
		REQUIRE(prrB->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prrC);
		REQUIRE(prrC->data[0] ==  5);
		REQUIRE(prrC->length  ==  1);
		REQUIRE(prrC->size	  ==  8);
		REQUIRE(prrC->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(ok_prrD);
		REQUIRE(prrD->data[0] == 17);
		REQUIRE(prrD->length  ==  1);
		REQUIRE(prrD->size	  ==  8);
		REQUIRE(prrD->type	  == BLOCKTYPE_C_REAL);

		REQUIRE(!ok_prrE);
		REQUIRE(!ok_prrF);

		REQUIRE(ok_pss1);

		REQUIRE(pss1->type	 == BLOCKTYPE_C_OFFS_CHARS);
		REQUIRE(pss1->length == 20);
		REQUIRE(pss1->size	 == 8*((80 + 8 + 1 + 7)/8));

		REQUIRE(pss1->data[ 0] == JAZZC_NA_STRING);
		REQUIRE(pss1->data[ 1] == JAZZC_NA_STRING);
		REQUIRE(pss1->data[18] == JAZZC_NA_STRING);
		REQUIRE(pss1->data[19] == JAZZC_NA_STRING);

		REQUIRE(strlen(PCHAR(pss1,	0)) == 0);
		REQUIRE(strlen(PCHAR(pss1, 19)) == 0);

		pStringBuff psb1 = STRING_BUFFER(pss1);
		REQUIRE( psb1->NA		 == 0);
		REQUIRE(!psb1->isBig);
		REQUIRE( psb1->last_idx	 == 8);
		REQUIRE( psb1->buffer[0] == 0);

		REQUIRE(ok_pss2);

		REQUIRE(pss2->type	 == BLOCKTYPE_C_OFFS_CHARS);
		REQUIRE(pss2->length == 20);
		REQUIRE(pss2->size	 == 8*((80 + 8 + 1 + 7)/8));

		REQUIRE(pss2->data[ 0] == JAZZC_EMPTY_STRING);
		REQUIRE(pss2->data[ 1] == JAZZC_EMPTY_STRING);
		REQUIRE(pss2->data[18] == JAZZC_EMPTY_STRING);
		REQUIRE(pss2->data[19] == JAZZC_EMPTY_STRING);

		REQUIRE(strlen(PCHAR(pss2,	0)) == 0);
		REQUIRE(strlen(PCHAR(pss2, 19)) == 0);

		pStringBuff psb2 = STRING_BUFFER(pss2);
		REQUIRE( psb2->NA		 == 0);
		REQUIRE(!psb2->isBig);
		REQUIRE( psb2->last_idx	 == 8);
		REQUIRE( psb2->buffer[0] == 0);

		REQUIRE(ok_pss3);

		int len = strlen(sss[1]);
		REQUIRE(pss3->type	 == BLOCKTYPE_C_OFFS_CHARS);
		REQUIRE(pss3->length == 20);
		REQUIRE(pss3->size	 == 8*((80 + 8 + 1 + len + 7)/8));

		REQUIRE(pss3->data[ 0] == 8);
		REQUIRE(pss3->data[ 1] == 8);
		REQUIRE(pss3->data[18] == 8);
		REQUIRE(pss3->data[19] == 8);

		REQUIRE(!strcmp(PCHAR(pss3,	 0), sss[1]));
		REQUIRE(!strcmp(PCHAR(pss3, 19), sss[1]));

		pStringBuff psb3 = STRING_BUFFER(pss3);
		REQUIRE( psb3->NA		 == 0);
		REQUIRE(!psb3->isBig);
		REQUIRE( psb3->last_idx	 == 8 + len + 1);
		REQUIRE( psb3->buffer[len + 1] == 0);

		REQUIRE(ok_praw);

		REQUIRE(praw->type	 == BLOCKTYPE_RAW_ANYTHING);
		REQUIRE(praw->length == 1);
		REQUIRE(praw->size	 == 13);

		REQUIRE(praw->data[ 0] == 'H');
		REQUIRE(praw->data[ 1] == 'e');
		REQUIRE(praw->data[11] == 'd');
		REQUIRE(praw->data[12] == '!');

		REQUIRE(jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_TXT));

		REQUIRE(praw->type == BLOCKTYPE_RAW_MIME_TXT);

		pboo->data[0]  = false;
		pboo->data[49] = false;
		REQUIRE(!pboo->data[0]);
		REQUIRE(!pboo->data[49]);

		pii1->data[0]  = 6789;
		pii1->data[49] = 4466;
		REQUIRE(pii1->data[0]  == 6789);
		REQUIRE(pii1->data[49] == 4466);

		pii2->data[0] = 777;
		pii2->data[4] = 999;
		REQUIRE(pii2->data[0] == 777);
		REQUIRE(pii2->data[4] == 999);

		pii3->data[0] = 777;
		pii3->data[4] = 999;
		REQUIRE(pii3->data[0] == 777);
		REQUIRE(pii3->data[4] == 999);

		pii4->data[0] = 777;
		pii4->data[3] = 999;
		REQUIRE(pii4->data[0] == 777);
		REQUIRE(pii4->data[3] == 999);

		pii5->data[0] = 777;
		pii5->data[4] = 999;
		REQUIRE(pii5->data[0] == 777);
		REQUIRE(pii5->data[4] == 999);

		pii6->data[0] = 777;
		pii6->data[4] = 999;
		REQUIRE(pii6->data[0] == 777);
		REQUIRE(pii6->data[4] == 999);

		pii7->data[0] = 777;
		pii7->data[5] = 999;
		REQUIRE(pii7->data[0] == 777);
		REQUIRE(pii7->data[5] == 999);

		pii8->data[0] = 777;
		pii8->data[5] = 999;
		REQUIRE(pii8->data[0] == 777);
		REQUIRE(pii8->data[5] == 999);

		pii9->data[0] = 777;
		pii9->data[4] = 999;
		REQUIRE(pii9->data[0] == 777);
		REQUIRE(pii9->data[4] == 999);

		piiA->data[0] = 777;
		piiA->data[4] = 999;
		REQUIRE(piiA->data[0] == 777);
		REQUIRE(piiA->data[4] == 999);

		piiB->data[0] = 777;
		piiB->data[4] = 999;
		REQUIRE(piiB->data[0] == 777);
		REQUIRE(piiB->data[4] == 999);

		piiC->data[0] = 888;
		REQUIRE(piiC->data[0] == 888);

		piiD->data[0] = 888;
		REQUIRE(piiD->data[0] == 888);

		prr1->data[0]  = 67.89;
		prr1->data[49] = 98.76;
		REQUIRE(prr1->data[0]  == 67.89);
		REQUIRE(prr1->data[49] == 98.76);

		prr2->data[0] = 67.89;
		prr2->data[4] = 98.76;
		REQUIRE(prr2->data[0] == 67.89);
		REQUIRE(prr2->data[4] == 98.76);

		prr3->data[0] = 67.89;
		prr3->data[4] = 98.76;
		REQUIRE(prr3->data[0] == 67.89);
		REQUIRE(prr3->data[4] == 98.76);

		prr4->data[0] = 67.89;
		prr4->data[5] = 98.76;
		REQUIRE(prr4->data[0] == 67.89);
		REQUIRE(prr4->data[5] == 98.76);

		prr5->data[0] = 67.89;
		prr5->data[4] = 98.76;
		REQUIRE(prr5->data[0] == 67.89);
		REQUIRE(prr5->data[4] == 98.76);

		prr6->data[0] = 67.89;
		prr6->data[4] = 98.76;
		REQUIRE(prr6->data[0] == 67.89);
		REQUIRE(prr6->data[4] == 98.76);

		prr7->data[0] = 67.89;
		prr7->data[4] = 98.76;
		REQUIRE(prr7->data[0] == 67.89);
		REQUIRE(prr7->data[4] == 98.76);

		prr8->data[0] = 67.89;
		prr8->data[4] = 98.76;
		REQUIRE(prr8->data[0] == 67.89);
		REQUIRE(prr8->data[4] == 98.76);

		prr9->data[0] = 67.89;
		prr9->data[3] = 98.76;
		REQUIRE(prr9->data[0] == 67.89);
		REQUIRE(prr9->data[3] == 98.76);

		prrA->data[0] = 67.89;
		prrA->data[3] = 98.76;
		REQUIRE(prrA->data[0] == 67.89);
		REQUIRE(prrA->data[3] == 98.76);

		prrB->data[0] = 67.89;
		prrB->data[3] = 98.76;
		REQUIRE(prrB->data[0] == 67.89);
		REQUIRE(prrB->data[3] == 98.76);

		prrC->data[0] = 88.88;
		REQUIRE(prrC->data[0] == 88.88);

		prrD->data[0] = 88.88;
		REQUIRE(prrD->data[0] == 88.88);

		pss3->data[ 0] = jb.get_string_idx_C_OFFS_CHARS(pss3, NULL, 1);
		pss3->data[ 1] = jb.get_string_idx_C_OFFS_CHARS(pss3, sss[0], 0);
		pss3->data[18] = jb.get_string_idx_C_OFFS_CHARS(pss3, sss[0], strlen(sss[0]));
		pss3->data[19] = jb.get_string_idx_C_OFFS_CHARS(pss3, sss[1], strlen(sss[1]));
		REQUIRE(pss3->data[ 0] == JAZZC_NA_STRING);
		REQUIRE(pss3->data[ 1] == JAZZC_EMPTY_STRING);
		REQUIRE(pss3->data[18] == JAZZC_EMPTY_STRING);
		REQUIRE(pss3->data[19] == 8);

		REQUIRE( jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_BOOL));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_OFFS_CHARS));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_FACTOR));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_GRADE));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_INTEGER));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_TIMESEC));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_C_REAL));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_ANYTHING));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_STRINGS));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_R_RAW));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_APK));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_CSS));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_CSV));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_GIF));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_HTML));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_IPA));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_JPG));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_JS));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_JSON));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_MP4));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_PDF));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_PNG));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_TSV));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_TXT));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_XAP));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_XML));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_RAW_MIME_ICO));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_FUN_R));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_FUN_DFQL));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_FUN_DFQO));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_ALL_ROLES));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_ALL_KEYSPACE));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_ALL_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_MY_LOG_EVENTS));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_MY_PROFILING));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_MY_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_MY_URL_DICT));
		REQUIRE(!jb.reinterpret_cast_block(pboo, BLOCKTYPE_SOURCE_ATTRIB));

		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_BOOL));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_OFFS_CHARS));
		REQUIRE( jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_FACTOR));
		REQUIRE( jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_GRADE));
		REQUIRE( jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_INTEGER));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_TIMESEC));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_C_REAL));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_ANYTHING));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_STRINGS));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_R_RAW));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_APK));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_CSS));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_CSV));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_GIF));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_HTML));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_IPA));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_JPG));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_JS));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_JSON));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_MP4));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_PDF));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_PNG));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_TSV));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_TXT));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_XAP));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_XML));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_RAW_MIME_ICO));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_FUN_R));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_FUN_DFQL));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_FUN_DFQO));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_ALL_ROLES));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_ALL_KEYSPACE));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_ALL_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_MY_LOG_EVENTS));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_MY_PROFILING));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_MY_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_MY_URL_DICT));
		REQUIRE(!jb.reinterpret_cast_block(pii1, BLOCKTYPE_SOURCE_ATTRIB));

		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_BOOL));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_OFFS_CHARS));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_FACTOR));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_GRADE));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_INTEGER));
		REQUIRE( jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_TIMESEC));
		REQUIRE( jb.reinterpret_cast_block(prr1, BLOCKTYPE_C_REAL));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_ANYTHING));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_STRINGS));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_R_RAW));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_APK));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_CSS));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_CSV));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_GIF));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_HTML));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_IPA));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_JPG));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_JS));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_JSON));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_MP4));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_PDF));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_PNG));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_TSV));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_TXT));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_XAP));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_XML));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_RAW_MIME_ICO));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_FUN_R));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_FUN_DFQL));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_FUN_DFQO));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_ALL_ROLES));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_ALL_KEYSPACE));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_ALL_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_MY_LOG_EVENTS));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_MY_PROFILING));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_MY_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_MY_URL_DICT));
		REQUIRE(!jb.reinterpret_cast_block(prr1, BLOCKTYPE_SOURCE_ATTRIB));

		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_BOOL));
		REQUIRE( jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_OFFS_CHARS));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_FACTOR));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_GRADE));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_INTEGER));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_TIMESEC));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_C_REAL));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_ANYTHING));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_STRINGS));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_R_RAW));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_APK));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_CSS));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_CSV));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_GIF));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_HTML));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_IPA));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_JPG));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_JS));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_JSON));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_MP4));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_PDF));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_PNG));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_TSV));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_TXT));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_XAP));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_XML));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_RAW_MIME_ICO));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_FUN_R));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_FUN_DFQL));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_FUN_DFQO));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_ALL_ROLES));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_ALL_KEYSPACE));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_ALL_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_MY_LOG_EVENTS));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_MY_PROFILING));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_MY_SECURITY));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_MY_URL_DICT));
		REQUIRE(!jb.reinterpret_cast_block(pss1, BLOCKTYPE_SOURCE_ATTRIB));

		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_BOOL));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_OFFS_CHARS));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_FACTOR));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_GRADE));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_INTEGER));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_TIMESEC));
		REQUIRE(!jb.reinterpret_cast_block(praw, BLOCKTYPE_C_REAL));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_ANYTHING));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_STRINGS));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_R_RAW));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_APK));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_CSS));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_CSV));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_GIF));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_HTML));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_IPA));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_JPG));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_JS));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_JSON));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_MP4));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_PDF));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_PNG));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_TSV));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_TXT));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_XAP));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_XML));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_RAW_MIME_ICO));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_FUN_R));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_FUN_DFQL));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_FUN_DFQO));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_ALL_ROLES));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_ALL_KEYSPACE));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_ALL_SECURITY));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_MY_LOG_EVENTS));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_MY_PROFILING));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_MY_SECURITY));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_MY_URL_DICT));
		REQUIRE( jb.reinterpret_cast_block(praw, BLOCKTYPE_SOURCE_ATTRIB));

		JAZZFREE(pboo, RAM_ALLOC_C_BOOL);

		JAZZFREE(pii1, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii2, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii3, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii4, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii5, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii6, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii7, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii8, RAM_ALLOC_C_INTEGER);
		JAZZFREE(pii9, RAM_ALLOC_C_INTEGER);
		JAZZFREE(piiA, RAM_ALLOC_C_INTEGER);
		JAZZFREE(piiB, RAM_ALLOC_C_INTEGER);
		JAZZFREE(piiC, RAM_ALLOC_C_INTEGER);
		JAZZFREE(piiD, RAM_ALLOC_C_INTEGER);

		JAZZFREE(prr1, RAM_ALLOC_C_REAL);
		JAZZFREE(prr2, RAM_ALLOC_C_REAL);
		JAZZFREE(prr3, RAM_ALLOC_C_REAL);
		JAZZFREE(prr4, RAM_ALLOC_C_REAL);
		JAZZFREE(prr5, RAM_ALLOC_C_REAL);
		JAZZFREE(prr6, RAM_ALLOC_C_REAL);
		JAZZFREE(prr7, RAM_ALLOC_C_REAL);
		JAZZFREE(prr8, RAM_ALLOC_C_REAL);
		JAZZFREE(prr9, RAM_ALLOC_C_REAL);
		JAZZFREE(prrA, RAM_ALLOC_C_REAL);
		JAZZFREE(prrB, RAM_ALLOC_C_REAL);
		JAZZFREE(prrC, RAM_ALLOC_C_REAL);
		JAZZFREE(prrD, RAM_ALLOC_C_REAL);

		JAZZFREE(pss1, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pss2, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pss3, RAM_ALLOC_C_OFFS_CHARS);

		JAZZFREE(praw, RAM_ALLOC_C_RAW);

		REQUIRE(pboo == NULL);

		REQUIRE(pii1 == NULL);
		REQUIRE(pii2 == NULL);
		REQUIRE(pii3 == NULL);
		REQUIRE(pii4 == NULL);
		REQUIRE(pii5 == NULL);
		REQUIRE(pii6 == NULL);
		REQUIRE(pii7 == NULL);
		REQUIRE(pii8 == NULL);
		REQUIRE(pii9 == NULL);
		REQUIRE(piiA == NULL);
		REQUIRE(piiB == NULL);
		REQUIRE(piiC == NULL);
		REQUIRE(piiD == NULL);

		REQUIRE(prr1 == NULL);
		REQUIRE(prr2 == NULL);
		REQUIRE(prr3 == NULL);
		REQUIRE(prr4 == NULL);
		REQUIRE(prr5 == NULL);
		REQUIRE(prr6 == NULL);
		REQUIRE(prr7 == NULL);
		REQUIRE(prr8 == NULL);
		REQUIRE(prr9 == NULL);
		REQUIRE(prrA == NULL);
		REQUIRE(prrB == NULL);
		REQUIRE(prrC == NULL);
		REQUIRE(prrD == NULL);

		REQUIRE(pss1 == NULL);
		REQUIRE(pss2 == NULL);
		REQUIRE(pss3 == NULL);

		REQUIRE(praw == NULL);

		REQUIRE(jazzPtrTrackClose());
	}

	GIVEN("Some block_C_OFFS_CHARS")
	{
		pCharBlock pa1, pb1, pc2, pc3, pc4;

		bool ok_pa1 = JAZZALLOC(pa1, RAM_ALLOC_C_OFFS_CHARS, 132);
		bool ok_pb1 = JAZZALLOC(pb1, RAM_ALLOC_C_OFFS_CHARS, 133);
		bool ok_pc2 = JAZZALLOC(pc2, RAM_ALLOC_C_OFFS_CHARS, 136);
		bool ok_pc3 = JAZZALLOC(pc3, RAM_ALLOC_C_OFFS_CHARS, 131072);
		bool ok_pc4 = JAZZALLOC(pc4, RAM_ALLOC_C_OFFS_CHARS, 131072);

		REQUIRE(ok_pa1);
		REQUIRE(ok_pb1);
		REQUIRE(ok_pc2);
		REQUIRE(ok_pc3);
		REQUIRE(ok_pc4);

		REQUIRE(!jb.format_C_OFFS_CHARS(pa1, 31));		// 31*4 + 8 + 1 = 133 > 132
		REQUIRE( jb.format_C_OFFS_CHARS(pb1, 31));		// 31*4 + 8 + 1 = 133
		REQUIRE( jb.format_C_OFFS_CHARS(pc2, 24));
		REQUIRE( jb.format_C_OFFS_CHARS(pc3, 2048));
		REQUIRE( jb.format_C_OFFS_CHARS(pc4, 16384));

		pStringBuff psb1 = STRING_BUFFER(pb1);
		pStringBuff psb2 = STRING_BUFFER(pc2);
		pStringBuff psb3 = STRING_BUFFER(pc3);
		pStringBuff psb4 = STRING_BUFFER(pc4);

		REQUIRE(!psb1->isBig);
		REQUIRE( psb1->NA == 0);
		REQUIRE( psb1->last_idx == 8);
		REQUIRE( psb1->buffer[0] == 0);

		REQUIRE(pb1->data[ 0] == JAZZC_NA_STRING);
		REQUIRE(pb1->data[ 1] == JAZZC_NA_STRING);
		REQUIRE(pb1->data[29] == JAZZC_NA_STRING);
		REQUIRE(pb1->data[30] == JAZZC_NA_STRING);

		string st;
		int idx;
		uintptr_t siz = (uintptr_t)psb1 - (uintptr_t)&pb1->data;
		REQUIRE(siz == 124);

		st = "";
		idx = jb.get_string_idx_C_OFFS_CHARS(pb1, st.c_str(), st.length());
		REQUIRE(idx == JAZZC_EMPTY_STRING);

		idx = jb.get_string_idx_C_OFFS_CHARS(pb1, NULL, 1);
		REQUIRE(idx == JAZZC_NA_STRING);

		st = "a";
		idx = jb.get_string_idx_C_OFFS_CHARS(pb1, st.c_str(), st.length());
		REQUIRE(idx == -1);

		siz = (uintptr_t)psb2 - (uintptr_t)&pc2->data;
		REQUIRE(siz == 96);

		REQUIRE(pc2->data[ 0] == JAZZC_NA_STRING);
		REQUIRE(pc2->data[ 1] == JAZZC_NA_STRING);
		REQUIRE(pc2->data[22] == JAZZC_NA_STRING);
		REQUIRE(pc2->data[23] == JAZZC_NA_STRING);

		REQUIRE(!psb2->isBig);
		REQUIRE( psb2->NA == 0);
		REQUIRE( psb2->last_idx == 8);
		REQUIRE( psb2->buffer[0] == 0);

		st = "123 hello.";	// 10 chars
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 8);
		REQUIRE(psb2->last_idx == 19);
		pc2->data[0] = idx;
		st = "Bye.";		// 4 chars
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 19);
		REQUIRE(psb2->last_idx == 24);
		pc2->data[1] = idx;
		st = "More.";		// 5 chars
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 24);
		REQUIRE(psb2->last_idx == 30);
		pc2->data[2] = idx;
		st = "Bye.";		// Known
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 19);
		st = "123 hello.";	// Known
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 8);
		st = "Anothr";		// 6 chars
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 30);
		REQUIRE(psb2->last_idx == 37);
		pc2->data[3] = idx;
		st = "This is too big.";
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == -1);
		st = "123 hello.";	// Known
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 8);
		st = "Anothr";		// Known
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 30);
		st = "ZZ";			// 2 is still too much
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == -1);
		st = "Z";			// 1 is ok
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 37);
		REQUIRE(psb2->last_idx == 39);
		pc2->data[4] = idx;
		st = "X";
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == -1);
		st = "Z";
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 37);

		bool ok;
		st = "123 hello.";
		ok = !strcmp(PCHAR(pc2, 0), st.c_str());
		REQUIRE(ok);
		st = "Bye.";
		ok = !strcmp(PCHAR(pc2, 1), st.c_str());
		REQUIRE(ok);
		st = "More.";
		ok = !strcmp(PCHAR(pc2, 2), st.c_str());
		REQUIRE(ok);
		st = "Anothr";
		ok = !strcmp(PCHAR(pc2, 3), st.c_str());
		REQUIRE(ok);
		st = "Z";
		ok = !strcmp(PCHAR(pc2, 4), st.c_str());
		REQUIRE(ok);

		st = "This is too big.";		// 16
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == -1);

		ok = jb.realloc_C_OFFS_CHARS(pc2, 32);
		REQUIRE(ok);

		psb2 = STRING_BUFFER(pc2);
		siz = (uintptr_t)psb2 - (uintptr_t)&pc2->data;
		REQUIRE(siz == 96);
		REQUIRE(pc2->size	== 168);
		REQUIRE(pc2->length ==	24);

		st = "This is too big.";		// 16
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 39);
		pc2->data[5] = idx;

		st = "Wow.";					// 4
		idx = jb.get_string_idx_C_OFFS_CHARS(pc2, st.c_str(), st.length());
		REQUIRE(idx == 56);
		pc2->data[6] = idx;

		st = "123 hello.";
		ok = !strcmp(PCHAR(pc2, 0), st.c_str());
		REQUIRE(ok);
		st = "Bye.";
		ok = !strcmp(PCHAR(pc2, 1), st.c_str());
		REQUIRE(ok);
		st = "More.";
		ok = !strcmp(PCHAR(pc2, 2), st.c_str());
		REQUIRE(ok);
		st = "Anothr";
		ok = !strcmp(PCHAR(pc2, 3), st.c_str());
		REQUIRE(ok);
		st = "Z";
		ok = !strcmp(PCHAR(pc2, 4), st.c_str());
		REQUIRE(ok);
		st = "This is too big.";
		ok = !strcmp(PCHAR(pc2, 5), st.c_str());
		REQUIRE(ok);
		st = "Wow.";
		ok = !strcmp(PCHAR(pc2, 6), st.c_str());
		REQUIRE(ok);

		siz = (uintptr_t)psb3 - (uintptr_t)&pc3->data;
		REQUIRE(siz == 8192);
		REQUIRE(pc3->data[	 0] == JAZZC_NA_STRING);
		REQUIRE(pc3->data[	 1] == JAZZC_NA_STRING);
		REQUIRE(pc3->data[2046] == JAZZC_NA_STRING);
		REQUIRE(pc3->data[2047] == JAZZC_NA_STRING);

		static const char* const strs[] = {"One", "Two", "Thr", "Fou", "Fiv", "Six", "Sev", "Eig", "Nin", "Ten",
										   "2ne", "2wo", "2hr", "2ou", "2iv", "2ix", "2ev", "2ig", "2in", "2en",
										   "3ne", "3wo", "3hr", "3ou", "3iv", "3ix", "3ev", "3ig", "3in", "3en",
										   "4ne", "4wo", "4hr", "4ou", "4iv", "4ix", "4ev", "4ig", "4in", "4en",
										   "5ne", "5wo", "5hr", "5ou", "5iv", "5ix", "5ev", "5ig", "5in", "5en"};

		for (int i = 0; i < pc3->length; i++) pc3->data[i] = jb.get_string_idx_C_OFFS_CHARS(pc3, strs[i % 10], 3);

		ok = true;
		for (int i = 0; i < pc3->length; i++)
			if (pc3->data[i] != 8 + 4*(i % 10))
				ok = false;

		REQUIRE(ok);

		REQUIRE(!psb3->isBig);
		REQUIRE( psb3->NA == 0);
		REQUIRE( psb3->last_idx	  == 48);
		REQUIRE( psb3->buffer[40] == 0);

		REQUIRE(jb.get_string_idx_C_OFFS_CHARS(pc3, NULL, 1) == JAZZC_NA_STRING);

		siz = (uintptr_t)psb4 - (uintptr_t)&pc4->data;
		REQUIRE(siz == 65536);
		REQUIRE(pc4->data[	  0] == JAZZC_NA_STRING);
		REQUIRE(pc4->data[	  1] == JAZZC_NA_STRING);
		REQUIRE(pc4->data[16382] == JAZZC_NA_STRING);
		REQUIRE(pc4->data[16383] == JAZZC_NA_STRING);

		for (int i = 0; i < 25; i++) pc4->data[i] = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[i], 3);

		ok = true;
		for (int i = 0; i < 25; i++)
			if (pc4->data[i] != 8 + 4*i)
				ok = false;

		REQUIRE(ok);
		REQUIRE(!psb4->isBig);
		REQUIRE( psb4->NA == 0);
		REQUIRE( psb4->last_idx	  == 108);
		REQUIRE( psb4->buffer[100] == 0);

		idx = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[20], 3);
		REQUIRE(idx == 88);

		pc4->data[25] = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[25], 3);
		REQUIRE(psb4->isBig);
		REQUIRE(psb4->NA == 0);
		REQUIRE(psb4->last_idx	 == 112);
		REQUIRE(psb4->buffer[104] == 0);

		idx = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[20], 3);
		REQUIRE(idx == 112);
		idx = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[20], 3);
		REQUIRE(idx == 116);

		for (int i = 26; i < 16384; i++) pc4->data[i] = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[i % 50], 3);

		REQUIRE(psb4->last_idx == 65552 - 20);	// Counting the 5 failed to allocate

		ok = true;
		for (int i = 16379; i < 16384; i++)
			if (pc4->data[i] != -1)				// Must fail to allocate 5 strings.
				ok = false;

		REQUIRE(ok);

		REQUIRE(psb4->isBig);
		REQUIRE(psb4->NA == 0);
		REQUIRE(psb4->buffer[65544 - 20] == 0);

		int fail = 0;
		for (int i = 26; i < 16379; i++)
			if (pc4->data[i] != 120 + 4*(i - 26))
				fail = fail ? fail : i;

		REQUIRE(fail == 0);

		ok = jb.realloc_C_OFFS_CHARS(pc4, 20);
		REQUIRE(ok);

		for (int i = 16379; i < 16384; i++) pc4->data[i] = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[i % 50], 3);

		psb4 = STRING_BUFFER(pc4);
		REQUIRE(psb4->last_idx == 65552);
		REQUIRE(psb4->isBig);
		REQUIRE(psb4->NA == 0);
		REQUIRE(psb4->buffer[65544] == 0);

		ok = true;
		for (int i = 16379; i < 16384; i++)
			if (pc4->data[i] != 65532 + 4*(i - 16379))
				ok = false;

		REQUIRE(ok);

		idx = jb.get_string_idx_C_OFFS_CHARS(pc4, strs[0], 3);
		REQUIRE(idx == -1);

		idx = jb.get_string_idx_C_OFFS_CHARS(pc4, NULL, 1);
		REQUIRE(idx == JAZZC_NA_STRING);

		JAZZFREE(pa1, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pb1, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pc2, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pc3, RAM_ALLOC_C_OFFS_CHARS);
		JAZZFREE(pc4, RAM_ALLOC_C_OFFS_CHARS);

		REQUIRE(pa1 == NULL);
		REQUIRE(pb1 == NULL);
		REQUIRE(pc2 == NULL);
		REQUIRE(pc3 == NULL);
		REQUIRE(pc4 == NULL);

		REQUIRE(jazzPtrTrackClose());
	}

	jb.stop();
}


SCENARIO("Testing all LMDB functions.")
{
	jzzBLOCKS jb;

	jCommons.load_config_file("./config/jazz_config.ini");

	struct stat sb;

	if (stat(TEST_LMDB_PATH, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
		remove(TEST_LMDB_DBI);

		REQUIRE(remove(TEST_LMDB_PATH) == 0);
	}
	REQUIRE(stat(TEST_LMDB_PATH, &sb) != 0);

	jCommons.debug_config_put("JazzPERSISTENCE.MDB_PERSISTENCE_PATH", TEST_LMDB_PATH);
	jCommons.debug_config_put("JazzPERSISTENCE.MDB_ENV_SET_MAXDBS", "32");				// MAX_POSSIBLE_SOURCES

	persistedKey key1, key2 = {"0_aZ_xy__"};

	REQUIRE( jb.char_to_key ("09_AZ_az", key1));
	REQUIRE( jb.char_to_key ("123456789012345", key1));
	REQUIRE( jb.char_to_key ("Yes_alpha", key1));
	REQUIRE( jb.char_to_key ("0_aZ_xy__", key1));

	REQUIRE(!jb.char_to_key ("This_is_too_long_for_a_key", key1));
	REQUIRE(!jb.char_to_key ("1234567890123456", key1));
	REQUIRE(!jb.char_to_key (" abc", key1));
	REQUIRE(!jb.char_to_key ("/abc", key1));
	REQUIRE(!jb.char_to_key (":abc", key1));
	REQUIRE(!jb.char_to_key ("@abc", key1));
	REQUIRE(!jb.char_to_key ("[abc", key1));
	REQUIRE(!jb.char_to_key ("`abc", key1));
	REQUIRE(!jb.char_to_key ("{abc", key1));
	REQUIRE(!jb.char_to_key ("abc ", key1));
	REQUIRE(!jb.char_to_key ("abc/", key1));
	REQUIRE(!jb.char_to_key ("abc:", key1));
	REQUIRE(!jb.char_to_key ("abc@", key1));
	REQUIRE(!jb.char_to_key ("abc[", key1));
	REQUIRE(!jb.char_to_key ("abc`", key1));
	REQUIRE(!jb.char_to_key ("abc{", key1));
	REQUIRE(!jb.char_to_key ("", key1));

	REQUIRE(!jb.strcmp_keys(key1, key2));

	REQUIRE(jb.start());

	REQUIRE(jb.open_all_sources());
	REQUIRE(jb.num_sources() == 2);

	REQUIRE(jb.source_idx[0] == -1);
	REQUIRE(jb.source_idx[1] == -1);
	REQUIRE(jb.source_idx[tenbits("sys")] == 0);
	REQUIRE(jb.source_idx[tenbits("www")] == 1);
	REQUIRE(jb.source_idx[1023] == -1);

	sourceName name0, name1, name2, name3, nameI;

	REQUIRE( jb.source_name(0, name0));
	REQUIRE( jb.source_name(1, name1));
	REQUIRE(!jb.source_name(2, name2));

	REQUIRE(!strcmp(name0.key, "sys"));
	REQUIRE(!strcmp(name1.key, "www"));

	REQUIRE(!jb.new_source("This_name_is_way_too_long_for_a_source"));
	REQUIRE(!jb.new_source("Not@alpha"));
	REQUIRE( jb.new_source("Yes_alpha"));

	REQUIRE( jb.source_name(2, name2));
	REQUIRE(!strcmp(name2.key, "Yes_alpha"));

	REQUIRE(jb.num_sources() == 3);

	REQUIRE(!jb.new_source("Yes_alpha"));

	REQUIRE(jb.num_sources() == 3);

	REQUIRE( jb.source_name(2, name2));
	REQUIRE(!strcmp(name2.key, "Yes_alpha"));

	REQUIRE(jb.num_sources() == 3);

	REQUIRE(!jb.kill_source("unknown"));
	REQUIRE( jb.kill_source("Yes_alpha"));
	REQUIRE(!jb.kill_source("sys"));
	REQUIRE(!jb.kill_source("www"));

	REQUIRE(jb.num_sources() == 2);

	REQUIRE(!jb.new_source("1234567890123456"));
	REQUIRE( jb.new_source("123456789012345"));

	REQUIRE(jb.num_sources() == 3);

	REQUIRE( jb.kill_source("123456789012345"));

	REQUIRE(jb.num_sources() == 2);

	for (int i = 0; i < 30; i++)
	{
		nameI.key[0] = 'a' + (char) i / 16;
		nameI.key[1] = 'a' + (char) i % 16;
		nameI.key[2] = 0;

		REQUIRE( jb.new_source(nameI.key));
		REQUIRE( jb.num_sources() == i + 3);
		REQUIRE( jb.source_name(i + 2, name3));
		REQUIRE(!strcmp(name3.key, nameI.key));

		int fi = jb.get_source_idx(nameI.key);

		REQUIRE(fi == i + 2);
	}
	REQUIRE( jb.num_sources() == 32);
	REQUIRE(!jb.new_source("toomany"));
	REQUIRE( jb.num_sources() == 32);

	for (int i = 0; i < 30; i++)
	{
		nameI.key[0] = 'a' + (char) i / 16;
		nameI.key[1] = 'a' + (char) i % 16;
		nameI.key[2] = 0;

		REQUIRE(jb.get_source_idx(nameI.key) == 2);

		REQUIRE(jb.kill_source(nameI.key));

		REQUIRE(jb.num_sources() == 31 - i);
	}

	REQUIRE(jb.num_sources() == 2);

	REQUIRE(jb.new_source("test1"));
	REQUIRE(jb.new_source("test2"));

	REQUIRE(jb.num_sources() == 4);

	REQUIRE(jb.source_idx[0] == -1);
	REQUIRE(jb.source_idx[1] == -1);
	REQUIRE(jb.source_idx[tenbits("sys")]	== 0);
	REQUIRE(jb.source_idx[tenbits("www")]	== 1);
	REQUIRE(jb.source_idx[tenbits("test1")] == 3);
	REQUIRE(jb.source_idx[1023] == -1);

	jb.close_all_sources();

	REQUIRE(jb.num_sources() == 0);

	REQUIRE(jb.source_idx[0] == -1);
	REQUIRE(jb.source_idx[1] == -1);
	REQUIRE(jb.source_idx[tenbits("sys")]	== -1);
	REQUIRE(jb.source_idx[tenbits("www")]	== -1);
	REQUIRE(jb.source_idx[tenbits("test1")] == -1);
	REQUIRE(jb.source_idx[1023] == -1);

	REQUIRE(jb.get_source_idx("") < 0);
	REQUIRE(jb.get_source_idx("\0.") < 0);

	REQUIRE(jb.get_source_idx("sys") < 0);
	REQUIRE(jb.get_source_idx("www") < 0);

	REQUIRE(jb.open_all_sources());

	REQUIRE(jb.num_sources() == 4);

	REQUIRE(jb.get_source_idx("") < 0);
	REQUIRE(jb.get_source_idx("\0■") < 0);

	REQUIRE(jb.get_source_idx("sys") == 0);
	REQUIRE(jb.get_source_idx("www") == 1);
	REQUIRE(jb.get_source_idx("test1") == 2);
	REQUIRE(jb.get_source_idx("test2") == 3);

	REQUIRE(jb.get_source_idx("") < 0);
	REQUIRE(jb.get_source_idx("\0\1") < 0);

	pRawBlock  praw1, praw2;

	static const char* const sss1[] = {"", "Hello, world!\0"};
	static const char* const sss2[] = {"", "Hallo, world?\0"};

	bool ok_praw1 = jb.new_block_C_RAW_once(praw1, sss1[1], strlen(sss1[1]) + 1);
	bool ok_praw2 = jb.new_block_C_RAW_once(praw2, sss2[1], strlen(sss2[1]) + 1);

	REQUIRE(ok_praw1);
	REQUIRE(ok_praw2);

	REQUIRE(praw1->type	  == BLOCKTYPE_RAW_ANYTHING);
	REQUIRE(praw1->length == 1);
	REQUIRE(praw1->size	  == 14);

	REQUIRE(praw1->data[ 0] == 'H');
	REQUIRE(praw1->data[ 1] == 'e');
	REQUIRE(praw1->data[12] == '!');
	REQUIRE(praw1->data[13] == 0);

	REQUIRE(jb.reinterpret_cast_block(praw1, BLOCKTYPE_RAW_MIME_TXT));

	REQUIRE(praw1->type == BLOCKTYPE_RAW_MIME_TXT);

	REQUIRE(praw2->type	  == BLOCKTYPE_RAW_ANYTHING);
	REQUIRE(praw2->length == 1);
	REQUIRE(praw2->size	  == 14);

	REQUIRE(praw2->data[ 0] == 'H');
	REQUIRE(praw2->data[ 1] == 'a');
	REQUIRE(praw2->data[12] == '?');
	REQUIRE(praw2->data[13] == 0);

	persistedKey kk[] = {"0_zer0", "1_o1e", "2_t2o", "3_thr33", "987654321012345"};
	persistedKey KK[] = {"0_ZER0", "1_O1E", "2_T2O", "3_THR33"};
	persistedKey searchK;
	pJazzBlock	pLbl0 = NULL, pLbl1 = NULL, pLbl2 = NULL, pLbl3 = NULL, pLbl4 = NULL, pLblN = NULL;

	REQUIRE(!jb.block_get(2, kk[0], pLbl0));
	REQUIRE( jb.block_put(2, kk[0], praw1));
	REQUIRE( jb.block_get(2, kk[0], pLbl0));
	REQUIRE(!jb.block_get(2, KK[0], pLblN));

	REQUIRE(praw1 != pLbl0);
	REQUIRE(praw1->type	  == pLbl0->type);
	REQUIRE(praw1->length == pLbl0->length);
	REQUIRE(praw1->size	  == pLbl0->size);
	REQUIRE(praw1->hash64 == pLbl0->hash64);
	REQUIRE(!strcmp(praw1->data, reinterpret_cast<pRawBlock>(pLbl0)->data));

	REQUIRE( jb.block_unprotect(pLbl0));
	REQUIRE(!jb.block_unprotect(pLbl0));
	REQUIRE(!jb.block_unprotect(pLbl1));

	REQUIRE(!jb.block_get(2, kk[1], pLbl1));
	REQUIRE( jb.block_put(2, kk[1], praw1));
	REQUIRE( jb.block_get(2, kk[1], pLbl1));
	REQUIRE(!jb.block_get(2, KK[1], pLblN));

	REQUIRE(!jb.block_get(2, kk[2], pLbl2));
	REQUIRE( jb.block_put(2, kk[2], praw1));
	REQUIRE( jb.block_get(2, kk[2], pLbl2));
	REQUIRE(!jb.block_get(2, KK[2], pLblN));

	REQUIRE(!jb.block_get(2, kk[3], pLbl3));
	REQUIRE( jb.block_put(2, kk[3], praw1));
	REQUIRE( jb.block_get(2, kk[3], pLbl3));
	REQUIRE(!jb.block_get(2, KK[3], pLblN));

	REQUIRE(!jb.block_get(2, kk[4], pLbl4));
	REQUIRE( jb.block_put(2, kk[4], praw1));
	REQUIRE( jb.block_get(2, kk[4], pLbl4));

	REQUIRE(praw1 != pLbl4);
	REQUIRE(!strcmp(praw1->data, reinterpret_cast<pRawBlock>(pLbl4)->data));
	REQUIRE( strcmp(praw2->data, reinterpret_cast<pRawBlock>(pLbl4)->data));

	REQUIRE(jb.block_unprotect(pLbl4));

	REQUIRE( jb.block_put(2, kk[4], praw2));
	REQUIRE( jb.block_get(2, kk[4], pLbl4));

	REQUIRE(praw2 != pLbl4);
	REQUIRE( strcmp(praw1->data, reinterpret_cast<pRawBlock>(pLbl4)->data));
	REQUIRE(!strcmp(praw2->data, reinterpret_cast<pRawBlock>(pLbl4)->data));

	REQUIRE(jb.block_unprotect(pLbl4));

	REQUIRE( jb.block_put(2, kk[4], praw1));
	REQUIRE( jb.block_get(2, kk[4], pLbl4));

	REQUIRE(praw1 != pLbl4);
	REQUIRE(!strcmp(praw1->data, reinterpret_cast<pRawBlock>(pLbl4)->data));
	REQUIRE( strcmp(praw2->data, reinterpret_cast<pRawBlock>(pLbl4)->data));

	REQUIRE(jb.block_unprotect(pLbl4));

	REQUIRE( jb.block_unprotect(pLbl1));
	REQUIRE( jb.block_unprotect(pLbl2));
	REQUIRE( jb.block_unprotect(pLbl3));
	REQUIRE(!jb.block_unprotect(pLbl1));
	REQUIRE(!jb.block_unprotect(pLbl2));
	REQUIRE(!jb.block_unprotect(pLbl3));

	REQUIRE( jb.block_get(2, kk[0], pLbl0));
	REQUIRE( jb.block_get(2, kk[1], pLbl1));
	REQUIRE( jb.block_get(2, kk[2], pLbl2));
	REQUIRE( jb.block_get(2, kk[3], pLbl3));

	REQUIRE( jb.block_unprotect(pLbl0));
	REQUIRE( jb.block_unprotect(pLbl1));
	REQUIRE( jb.block_unprotect(pLbl2));
	REQUIRE( jb.block_unprotect(pLbl3));

	REQUIRE(!jb.block_get(3, kk[0], pLbl0));
	REQUIRE(!jb.block_get(3, kk[1], pLbl1));
	REQUIRE(!jb.block_get(3, kk[2], pLbl2));
	REQUIRE(!jb.block_get(3, kk[3], pLbl3));

	REQUIRE(!jb.block_get(3, KK[0], pLbl0));
	REQUIRE(!jb.block_get(3, KK[1], pLbl1));
	REQUIRE(!jb.block_get(3, KK[2], pLbl2));
	REQUIRE(!jb.block_get(3, KK[3], pLbl3));

	REQUIRE( jb.block_put(3, KK[0], praw1));
	REQUIRE( jb.block_put(3, KK[1], praw1));
	REQUIRE( jb.block_put(3, KK[2], praw1));
	REQUIRE( jb.block_put(3, KK[3], praw1));

	REQUIRE( jb.block_get(3, KK[0], pLbl0));
	REQUIRE( jb.block_get(3, KK[1], pLbl1));
	REQUIRE( jb.block_get(3, KK[2], pLbl2));
	REQUIRE( jb.block_get(3, KK[3], pLbl3));

	REQUIRE( jb.block_unprotect(pLbl0));
	REQUIRE( jb.block_unprotect(pLbl1));
	REQUIRE( jb.block_unprotect(pLbl2));
	REQUIRE( jb.block_unprotect(pLbl3));

	REQUIRE(!jb.block_unprotect(pLbl0));
	REQUIRE(!jb.block_unprotect(pLbl1));
	REQUIRE(!jb.block_unprotect(pLbl2));
	REQUIRE(!jb.block_unprotect(pLbl3));

	REQUIRE(!jb.char_to_key ("This_is_too_long_for_a_key", searchK));
	REQUIRE(!jb.char_to_key ("1234567890123456", searchK));
	REQUIRE( jb.char_to_key ("123456789012345", searchK));
	REQUIRE(!jb.char_to_key ("Not@alpha", searchK));
	REQUIRE( jb.char_to_key ("Yes_alpha", searchK));
	REQUIRE( jb.char_to_key ("0_zer0", searchK));
	REQUIRE(!jb.char_to_key ("", searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[0]));

	jb.set_first_key(searchK);

	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_START));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[0]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[1]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[2]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[3]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[4]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_END));
	REQUIRE( jb.is_last_key(searchK));

	REQUIRE(!jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_END));
	REQUIRE( jb.is_last_key(searchK));

	REQUIRE( jb.block_kill(2, kk[0]));
	REQUIRE( jb.block_kill(2, kk[1]));
	REQUIRE( jb.block_kill(2, kk[2]));
	REQUIRE( jb.block_kill(2, kk[3]));

	REQUIRE(!jb.block_kill(2, kk[0]));
	REQUIRE(!jb.block_kill(2, kk[1]));
	REQUIRE(!jb.block_kill(2, kk[2]));
	REQUIRE(!jb.block_kill(2, kk[3]));

	jb.set_first_key(searchK);

	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_START));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, kk[4]));
	REQUIRE(!jb.is_last_key(searchK));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_END));
	REQUIRE( jb.is_last_key(searchK));

	REQUIRE(!jb.next_key(2, searchK));

	REQUIRE( jb.block_kill(2, kk[4]));

	jb.set_first_key(searchK);

	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_START));

	REQUIRE( jb.next_key(2, searchK));
	REQUIRE(!jb.strcmp_keys(searchK, KEY_SPACE_END));
	REQUIRE( jb.is_last_key(searchK));

	REQUIRE(!jb.next_key(2, searchK));

	REQUIRE(!jb.reinterpret_cast_block(praw1, BLOCKTYPE_C_BOOL));
	REQUIRE( jb.reinterpret_cast_block(praw1, BLOCKTYPE_SOURCE_ATTRIB));

	JAZZFREE(praw1, RAM_ALLOC_C_RAW);
	JAZZFREE(praw2, RAM_ALLOC_C_RAW);

	REQUIRE(praw1 == NULL);
	REQUIRE(praw2 == NULL);

	REQUIRE(jazzPtrTrackClose());

	jb.stop();
}

#endif

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

/*! \brief Root class providing all the necessary web resource functionality.

This class keeps a dictionary of known urls as keys, the values include: block name Content-Length, Contenct_Language, Content_Encoding and
Last-Modified. The source name is always "www". These dictionaries are stored in persistence.

The urls and all their attributes are stored in URLattrib structures. An abstract entity known as a websource organizes the links in the persistence
in a way that a whole websource can be queried, created, deleted. websources are identified by name (websourceName) or index in a in internal vector
queried via find_websource(). websources do no exist for the API, they are just a clean way to manage sets of urls that are created/removed together.
*/

#include "src/jazz01_misc/jazz_websource.h"

/*~ end of automatic header ~*/

/*	-----------------------------------------------
	  C o n s t r u c t o r / d e s t r u c t o r
--------------------------------------------------- */

jazzWebSource::jazzWebSource()
{
};

jazzWebSource::~jazzWebSource()
{
};

/*	-----------------------------------------------
	 Methods inherited from	 j a z z S e r v i c e
--------------------------------------------------- */

/** Start of jazzWebSource

	see jazzService::start()
*/
bool jazzWebSource::start()
{
	bool ok = super::start();

	if (!ok) return false;

	ok = open_all_websources();
	if (!ok)
	{
		jCommons.log(LOG_MISS, "jazzWebSource::start(): open_all_websources() failed.");

		return false;
	}

	jCommons.log(LOG_INFO, "jazzWebSource started.");

	return true;
}


/** Stop of jazzWebSource

	see jazzService::stop()
*/
bool jazzWebSource::stop()
{
	bool ok = close_all_websources();

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jazzWebSource::stop(): close_all_websources() failed.");

		super::stop();

		return false;
	}

	jCommons.log(LOG_INFO, "jazzWebSource stopped.");

	return super::stop();
}


/** Reload of jazzWebSource

	see jazzService::reload()
*/
bool jazzWebSource::reload()
{
	jCommons.log(LOG_INFO, "jazzWebSource::reload(): Nothing to do.");

	return super::reload();
}

/*	-----------------------------------------------
		Functions to manage	  w e b s o u r c e s
--------------------------------------------------- */

/** Initialize the whole object storage from block stored in persistence.

	\return	true if there are no websources or all websources were updated successfully, false and log(LOG_MISS, "further details") if not.

	The block is stored in sys.MY_URL_DICT as specified in the API.
*/
bool jazzWebSource::open_all_websources()
{
	sources		 = {};
	url_block	 = {};
	block_mime	 = {};
	block_lang	 = {};
	block_source = {};

	persistedKey block;
	pJazzBlock dict;
	jBLOCKC.char_to_key("MY_URL_DICT", block);
	if (!jBLOCKC.block_get (SYSTEM_SOURCE_SYS, block, dict))
	{
		jCommons.log(LOG_INFO, "Opening all websources at jazzWebSource::open_all_websources(): No websources found.");

		return true;
	}

	for (int i = 0; i < dict->length; i++)
	{
		URLattrib * puatt = &reinterpret_cast<block_URLdictionary *>(dict)->attr[i];

		new_websource(puatt->websource.key);

		if (!set_url_to_block (puatt->url, puatt->block.key, puatt->websource.key))
			goto exit_fail;

		if (puatt->blocktype != BLOCKTYPE_RAW_MIME_HTML && !set_mime_to_block(puatt->blocktype, puatt->block.key, puatt->websource.key))
			goto exit_fail;

		if (puatt->language != LANG_DONT_CARE && !set_lang_to_block(HTTP_LANGUAGE_STRING[puatt->language], puatt->block.key, puatt->websource.key))
			goto exit_fail;
	}

	jCommons.log_printf(LOG_INFO, "Opening all websources at jazzWebSource::open_all_websources(): %d websources found.", dict->length);

	jBLOCKC.block_unprotect (dict);

	return true;


exit_fail:

	jBLOCKC.block_unprotect (dict);

	return false;
}


/** Create a new websource.

	\param websource The name of the new websource.
	\return			 true if the websource does not already exist.
*/
bool jazzWebSource::new_websource(const char * websource)
{
	persistedKey key;
	if (!jBLOCKC.char_to_key(websource, key))
		return false;

	try
	{
		if (sources[websource] == 1)
			return false;
	}
	catch (...)
	{
	}

	if (sources.size() >= MAX_NUM_WEBSOURCES)
		return false;

	sources[websource] = 1;

	return true;
}


/** Kill a websource removing all its URLattrib from persistence.

	\param websource The name of the websource to be deleted.
	\return			 true if successful, false and log(LOG_MISS, "further details") if not.

This function returns true even if some blocks did not exist, as the interface allow to create the metadata without actually creating the blocks.
*/
bool jazzWebSource::kill_websource(const char * websource)
{
	try
	{
		if (sources[websource] != 1)
			return false;
	}
	catch (...)
	{
		return false;
	}

	for (map<string, string>::iterator it = block_source.begin(); it != block_source.end(); ++it)
	{
		if (!strcmp(it->second.c_str(), websource))
		{
			persistedKey key;
			if (!jBLOCKC.char_to_key(it->first.c_str(), key))
			{
				jCommons.log(LOG_MISS, "jazzWebSource::kill_websource(): block name fails char_to_key().");

				return false;
			}
			if (!jBLOCKC.block_kill(SYSTEM_SOURCE_WWW, key))
			{
				jCommons.log_printf(LOG_MISS, "jazzWebSource::kill_websource(): block_kill() failed bloc = %s.", key.key);
			}
		}
	}

	sources[websource] = 0;

	return close_all_websources() && open_all_websources();
}


/** Add a new (url, block) pair to an existing websource.

	\param url		 The url assigned to the block.
	\param block	 The block whose properties are assigned.
	\param websource The websource to which the block belongs.
	\return			 true if successful, false if the source does not already exist.
*/
bool jazzWebSource::set_url_to_block(const char * url, const char * block, const char * websource)
{
	persistedKey key;
	if (!url[0] || strlen(url) > MAX_URL_LENGTH || !jBLOCKC.char_to_key(block, key))
		return false;

	try
	{
		if (sources[websource] != 1)
			return false;
	}
	catch (...)
	{
		return false;
	}

	url_block[url]		= block;
	block_source[block] = websource;
	block_mime[block]	= BLOCKTYPE_RAW_MIME_HTML;
	block_lang[block]	= LANG_DONT_CARE;

	return true;
}


/** Add a mime type to an existing block defined in an existing websource.

	\param type		 The type assigned to the block.
	\param block	 The block whose properties are assigned.
	\param websource The websource to which the block belongs.
	\return			 true if successful, false if the source does not exist or is not already assigned to the block via set_url_to_block().

This function requires a previous call to set_url_to_block() to assign the block to the url. The set_url_to_block() already assigns the
mime type as BLOCKTYPE_RAW_MIME_HTML.
*/
bool jazzWebSource::set_mime_to_block(int type, const char * block, const char * websource)
{
	if (type < 0 || type > BLOCKTYPE_LAST_)
		return false;

	try
	{
		if (sources[websource] != 1)
			return false;

		string ss (block_source[block]);

		if (strcmp(websource, ss.c_str()))
			return false;
	}
	catch (...)
	{
		return false;
	}

	block_mime[block] = type;

	return true;
}


/** Add a language to an existing block defined in an existing websource.

	\param lang		 The language assigned to the block.
	\param block	 The block whose properties are assigned.
	\param websource The websource to which the block belongs.
	\return			 true if successful, false if the source does not exist or is not already assigned to the block via set_url_to_block().

This function requires a previous call to set_url_to_block() to assign the block to the url. The set_url_to_block() already assigns the
language as LANG_DONT_CARE == don't send a language header.
*/
bool jazzWebSource::set_lang_to_block(const char * lang, const char * block, const char * websource)
{
	if (!lang || !lang[0])
		return false;

	try
	{
		if (sources[websource] != 1)
			return false;

		string ss (block_source[block]);

		if (strcmp(websource, ss.c_str()))
			return false;
	}
	catch (...)
	{
		return false;
	}

	int ilang;

	for (ilang = 1; ilang < LANG_LAST_; ilang++)
	{
		if (!strcmp(lang, HTTP_LANGUAGE_STRING[ilang]))
		{
			block_lang[block] = ilang;

			return true;
		}
	}

	return false;
}


/** Write the whole object storage to the persistence and clear it.

	\return	true if there are no websources or all websources were written successfully, false and log(LOG_MISS, "further details") if not.

	The block is stored in sys.MY_URL_DICT as specified in the API.
*/
bool jazzWebSource::close_all_websources()
{
	persistedKey dict_block_key;

	jBLOCKC.char_to_key("MY_URL_DICT", dict_block_key);
	jBLOCKC.block_kill(SYSTEM_SOURCE_SYS, dict_block_key);

	int numitems = 0;

	for (map<string, string>::iterator it = url_block.begin(); it != url_block.end(); ++it)
	{
		string block (it->second);
		if (sources[block_source[block]] == 1)
			numitems++;
	}

	if (!numitems)
	{
		jCommons.log(LOG_INFO, "Closing all websources at jazzWebSource::close_all_websources(): No websources found.");

		return true;
	}

	int size = numitems*sizeof(URLattrib);
	block_URLdictionary * pdict;
	bool ok = JAZZALLOC(pdict, RAM_ALLOC_C_RAW, size);

	if (!ok)
	{
		jCommons.log(LOG_MISS, "jazzWebSource::close_all_websources(): alloc failed.");

		return false;
	}

	int i = 0;
	for (map<string, string>::iterator it = url_block.begin(); it != url_block.end(); ++it)
	{
		string block (it->second);
		if (sources[block_source[block]] == 1)
		{
			URLattrib * puat = &pdict->attr[i++];

			strcpy(puat->url, it->first.c_str());
			strcpy(puat->block.key, block.c_str());
			strcpy(puat->websource.key, block_source[block].c_str());
			puat->blocktype = block_mime[block];
			puat->language	= block_lang[block];
		}
	}

	pdict->type	  = BLOCKTYPE_MY_URL_DICT;
	pdict->length = numitems;
	if (!jBLOCKC.block_put(SYSTEM_SOURCE_SYS, dict_block_key, pdict))
	{
		jCommons.log(LOG_MISS, "jazzWebSource::close_all_websources(): jBLOCKC.block_put failed.");

		JAZZFREE(pdict, RAM_ALLOC_C_RAW);

		return false;
	}

	sources		 = {};
	url_block	 = {};
	block_mime	 = {};
	block_lang	 = {};
	block_source = {};

	JAZZFREE(pdict, RAM_ALLOC_C_RAW);

	jCommons.log_printf(LOG_INFO, "Closing all websources at jazzWebSource::close_all_websources(): %d websources saved.", numitems);

	return true;
}

/*	-----------------------------------------------
		Function for the  A P I
--------------------------------------------------- */

/** Get the following http attributes for a url: block, type and language.

	\param url	 The url to be tried.
	\param uattr A URLattrib record with all the http attributes.
	\return		 true if found. No log() on miss.
*/
bool jazzWebSource::get_url(const char * url, URLattrib &uattr)
{
	try
	{
		if (!jBLOCKC.char_to_key(url_block[url].c_str(), uattr.block))
			return false;

		uattr.blocktype = block_mime[uattr.block.key];
		uattr.language	= block_lang[uattr.block.key];

		return true;
	}
	catch (...)
	{
		return false;
	}
}

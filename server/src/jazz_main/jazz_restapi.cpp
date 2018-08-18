/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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

#include <signal.h>

#include "src/jazz_main/jazz_restapi.h"

namespace jazz_restapi
{

using namespace std;

//TODO: Implement module jazz_restapi.

/**
//TODO: Document JazzCallbackAnswerHTTP
*/
int JazzCallbackAnswerHTTP(void *cls,
						   struct MHD_Connection *connection,
						   const char *url,
						   const char *method,
						   const char *version,
						   const char *upload_data,
						   size_t *upload_data_size,
						   void **con_cls)
{
/*
	// Step 1: First opportunity to end the connection before uploading or getting. Not used. We initialize con_cls for the next call.

	if (*con_cls == NULL)
	{
		*con_cls = &state_new_call;

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int imethod;
	imethod = imethods[jazz_utils::TenBitsAtAddress(method)];

	parsedURL pars;

	if (*con_cls == &state_upload_in_progress)
	{
		if (*upload_data_size == 0)
			goto create_response_answer_put_ok;

		if (imethod != HTTP_PUT || !jAPI.parse_url(url, HTTP_PUT, pars) || pars.isInstrumental || pars.hasAFunction)
		{
			jCommons.log(LOG_MISS, "jazz_answer_to_connection(): Trying to continue state_upload_in_progress, but not exec_block_put()");

			return MHD_NO;
		}

		if (int tid = jCommons.enter_persistence(ACMODE_READWRITE) >= 0)
		{
			if (jAPI.exec_block_put(pars, upload_data, *upload_data_size, false))
			{
				jCommons.leave_persistence(tid);

				goto continue_in_put_ok;
			}
			jCommons.leave_persistence(tid);

			goto continue_in_put_notacceptable;
		}

		jCommons.log(LOG_MISS, "jazz_answer_to_connection(): Trying to continue state_upload_in_progress, but failed enter_persistence()");

		goto continue_in_put_unavailable;
	}

	// Step 3 : Get rid of failed uploads without doing anything.

	if (*con_cls == &state_upload_notacceptable)
	{
		if (*upload_data_size == 0)
			goto create_response_answer_PUT_NOTACCEPTABLE;

		return MHD_YES;
	}

	if (*con_cls == &state_upload_unavailable)
	{
		if (*upload_data_size == 0)
			goto create_response_answer_PUT_UNAVAILABLE;

		return MHD_YES;
	}

	if (*con_cls == &state_upload_badrequest)
	{
		if (*upload_data_size == 0)
			goto create_response_answer_PUT_BADREQUEST;

		return MHD_YES;
	}

	// Step 4 : This point is reached just once per http petition. Parse the query, returns errors and web pages, continue to API.

	struct MHD_Response * response;

#ifdef DEBUG
	jCommons.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - cls \x20 \x20 \x20 \x20 \x20 \x20 \x20: %p", cls);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - connection \x20 \x20 \x20 : %p", connection);
	MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - url \x20 \x20 \x20 \x20 \x20 \x20 \x20: %s", url);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - method \x20 \x20 \x20 \x20 \x20 : %s", method);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - version \x20 \x20 \x20 \x20 \x20: %s", version);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - upload_data \x20 \x20 \x20: %p", upload_data);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - upload_data_size : %d", *upload_data_size);
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - *con_cls \x20 \x20 \x20 \x20 : %p", *con_cls);
	jCommons.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");

	response = NULL;	// clang warning
#endif

	switch (imethod)
	{
		case HTTP_NOTUSED:
			return jAPI.return_error_message(connection, MHD_HTTP_METHOD_NOT_ALLOWED);


		case HTTP_OPTIONS:
			{
				string allow;
				if (jazz_utils::TenBitsAtAddress(url) != tenbitDS)
				{
					if (no_webpages)
						return jAPI.return_error_message(connection, MHD_HTTP_FORBIDDEN);

					URLattrib uattr;

					if (jAPI.get_url(url, uattr))
						allow = "HEAD,GET,";

					allow = allow + "OPTIONS";
				}
				else
				{
					if (jAPI.parse_url(url, HTTP_GET, pars))
						allow = "HEAD,GET,";
					if (jAPI.parse_url(url, HTTP_PUT, pars))
						allow = allow + "PUT,";
					if (jAPI.parse_url(url, HTTP_DELETE, pars))
						allow = allow + "DELETE,";

					allow = allow + "OPTIONS";
				}

				response = MHD_create_response_from_buffer (1, response_put_ok, MHD_RESPMEM_PERSISTENT);

				MHD_add_response_header (response, MHD_HTTP_HEADER_SERVER, "Jazz " JAZZ_VERSION " - " LINUX_PLATFORM);
				MHD_add_response_header (response, MHD_HTTP_HEADER_ALLOW, allow.c_str());
			}

			goto answer_no_content;


		case HTTP_HEAD:
		case HTTP_GET:
			if (jazz_utils::TenBitsAtAddress(url) != tenbitDS)
			{
				if (no_webpages)
					return jAPI.return_error_message(connection, MHD_HTTP_FORBIDDEN);

				bool ok;

				if (int tid = jCommons.enter_persistence(ACMODE_READONLY) >= 0)
				{
					ok = jAPI.exec_www_get(url, response);

					jCommons.leave_persistence(tid);
				}
				else
				{
					jCommons.log(LOG_MISS, "jazz_answer_to_connection(): HTTP_HEAD or HTTP_GET failed enter_persistence()");

					return jAPI.return_error_message(connection, MHD_HTTP_SERVICE_UNAVAILABLE);
				}

				if (ok)
					goto answer_http_ok;

				return jAPI.return_error_message(connection, MHD_HTTP_NOT_FOUND);
			}
			else
			{
				if (!jAPI.parse_url(url, HTTP_GET, pars))
					return jAPI.return_error_message(connection, MHD_HTTP_BAD_REQUEST);
			}
			break;


		default:
			if (jazz_utils::TenBitsAtAddress(url) != tenbitDS)
			{
				if (no_webpages)
					return jAPI.return_error_message(connection, MHD_HTTP_FORBIDDEN);

				return jAPI.return_error_message(connection, MHD_HTTP_METHOD_NOT_ALLOWED);
			}
			else
			{
				if (!jAPI.parse_url(url, imethod, pars))
				{
					if (imethod == HTTP_PUT)
						goto continue_in_put_badrequest;
					return jAPI.return_error_message(connection, MHD_HTTP_BAD_REQUEST);
				}
			}
	}

	// Step 5 : This is the core. This point is only reached by correct API queries for the first (or only) time.

	if (no_storage && pars.source > SYSTEM_SOURCE_WWW)
		return jAPI.return_error_message(connection, MHD_HTTP_FORBIDDEN);

	bool sample;
	int tid, status;

	sample = jAPI.sample_api_call();

	if (sample)
		jCommons.enter_api_call(connection, url, imethod);

	switch (imethod)
	{
		case HTTP_PUT:

			tid = pars.deleteSource ? jCommons.enter_persistence(ACMODE_SOURCECTL) : jCommons.enter_persistence(ACMODE_READWRITE);
			if (tid >= 0)
			{
				if (pars.isInstrumental)
				{
					status = pars.hasAFunction ? jAPI.exec_instr_put_function(pars, upload_data, *upload_data_size)
											   : jAPI.exec_instr_put(pars, upload_data, *upload_data_size);
				}
				else
				{
					status = pars.hasAFunction ? jAPI.exec_block_put_function(pars, upload_data, *upload_data_size)
											   : jAPI.exec_block_put(pars, upload_data, *upload_data_size, true);
				}
			}

			break;

		case HTTP_DELETE:

			tid = pars.deleteSource ? jCommons.enter_persistence(ACMODE_SOURCECTL) : jCommons.enter_persistence(ACMODE_READWRITE);
			if (tid >= 0)
			{
				if (pars.isInstrumental)
				{
					status = jAPI.exec_instr_kill(pars);
				}
				else
				{
					status = jAPI.exec_block_kill(pars);
				}
			}

			break;

		default:

			tid = jCommons.enter_persistence(ACMODE_READONLY);
			if (tid >= 0)
			{
				if (pars.isInstrumental)
				{
					status = pars.hasAFunction ? jAPI.exec_instr_get_function(pars, response) : jAPI.exec_instr_get(pars, response);
				}
				else
				{
					status = pars.hasAFunction ? jAPI.exec_block_get_function(pars, response) : jAPI.exec_block_get(pars, response);
				}
			}
	}

	if (tid >= 0) jCommons.leave_persistence(tid);
	else
	{
		jCommons.log(LOG_MISS, "jazz_answer_to_connection(): CORE failed enter_persistence()");

		if (imethod == HTTP_PUT) goto continue_in_put_unavailable;

		status = MHD_HTTP_SERVICE_UNAVAILABLE;
	}

	if (sample)
		jCommons.leave_api_call(status);

	// Step 6 : The core finished, just distribute the answer as appropriate.

	if (imethod == HTTP_PUT)
	{
		if (status)
		{
			if (*upload_data_size) goto continue_in_put_ok;
			else				   goto create_response_answer_put_ok;
		}
		else
		{
			if (*upload_data_size) goto continue_in_put_notacceptable;
			else				   goto create_response_answer_PUT_NOTACCEPTABLE;
		}
	}

	if (status >= MHD_HTTP_ANYERROR)
		return jAPI.return_error_message(connection, status);

	if (imethod == HTTP_DELETE)
		response = MHD_create_response_from_buffer (1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	int ret;

answer_status:

	ret = MHD_queue_response (connection, status, response);

	MHD_destroy_response (response);

	return ret;


answer_http_ok:

	status = MHD_HTTP_OK;

	goto answer_status;


answer_no_content:

	status = MHD_HTTP_NO_CONTENT;

	goto answer_status;


create_response_answer_put_ok:

	response = MHD_create_response_from_buffer (1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response (response);

	return ret;


create_response_answer_PUT_NOTACCEPTABLE:

	response = MHD_create_response_from_buffer (1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_NOT_ACCEPTABLE, response);

	MHD_destroy_response (response);

	return ret;


create_response_answer_PUT_UNAVAILABLE:

	response = MHD_create_response_from_buffer (1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_SERVICE_UNAVAILABLE, response);

	MHD_destroy_response (response);

	return ret;


create_response_answer_PUT_BADREQUEST:

	response = MHD_create_response_from_buffer (1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);

	MHD_destroy_response (response);

	return ret;


continue_in_put_notacceptable:

	if (*upload_data_size)
	{
		*upload_data_size = 0;
		*con_cls		  = &state_upload_notacceptable;

		return MHD_YES;
	}
	return MHD_NO;


continue_in_put_unavailable:

	if (*upload_data_size)
	{
		*upload_data_size = 0;
		*con_cls		  = &state_upload_unavailable;

		return MHD_YES;
	}
	return MHD_NO;


continue_in_put_badrequest:

	if (*upload_data_size)
	{
		*upload_data_size = 0;
		*con_cls		  = &state_upload_badrequest;

		return MHD_YES;
	}
	return MHD_NO;


continue_in_put_ok:

	if (*upload_data_size)
	{
		*upload_data_size = 0;
		*con_cls		  = &state_upload_in_progress;

		return MHD_YES;
	}
*/
	return MHD_NO;
}

#if defined LEGACY_MHD_CODE

/** Struct sizes
*/
#define MAX_URL_LENGTH		215		///< Maximum length of a URL in the www API. (This makes the URLattrib size = 256
#define MAX_NUM_WEBSOURCES	100		///< The maximum number of web sources. Limited to avoid buffer overflow in GET function.

typedef persistedKey websourceName;


/** A row in the url dictionary
*/
struct URLattrib
{
	char		 url[MAX_URL_LENGTH + 1];	///< The url
	persistedKey block;						///< The key identifying the resource. The resource is persisted as source="www", key=key.
	persistedKey websource;					///< The key identifying the resource. The resource is persisted as source="www", key=key.
	int			 blocktype;					///< The block type of the resource. BLOCKTYPE_RAW_MIME_*
	int			 language;					///< The language as an index to HTTP_LANGUAGE_STRING (consts like LANG_EN_US are valid too.)
};


/** A persistence block storing dictionary entries. The length parameter in the header defines the number of rows.
*/
struct block_URLdictionary : jzzBlockHeader
{
	URLattrib attr[];		///< The array of block_URLdictionary
};


class jazzWebSource: public jazzService {

	public:

		 jazzWebSource();
		~jazzWebSource();

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();

// Functions to manage websources.

		bool open_all_websources  ();
		bool new_websource		  (const char * websource);
		bool kill_websource		  (const char * websource);
		bool set_url_to_block	  (const char * url,  const char * block, const char * websource);
		bool set_mime_to_block	  (int type,		  const char * block, const char * websource);
		bool set_lang_to_block	  (const char * lang, const char * block, const char * websource);
		bool close_all_websources ();

// Function for the API.

		bool get_url (const char * url, URLattrib &uattr);

	protected:

		map<string, int> sources;			// websource -> 1 (0 when deleted)

	private:

		typedef jazzService super;

		map<string, string> url_block;		// url	  -> block
		map<string, int>	block_mime;		// block  -> mime type
		map<string, int>	block_lang;		// block  -> language
		map<string, string> block_source;	// block  -> websource
};

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


#endif


/**
//TODO: Document JazzHttpServer()
*/
JazzHttpServer::JazzHttpServer(jazz_utils::pJazzLogger	   a_logger,
							   jazz_utils::pJazzConfigFile a_config) : JazzAPI(a_logger, a_config)
{
#ifdef DEBUG
	log(LOG_DEBUG, "Entering JazzHttpServer::JazzHttpServer");
#endif

//TODO: Implement JazzHttpServer
}


/**
//TODO: Document ~JazzHttpServer()
*/
JazzHttpServer::~JazzHttpServer()
{
//TODO: Implement ~JazzHttpServer

#ifdef DEBUG
	log(LOG_DEBUG, "Leaving JazzHttpServer::~JazzHttpServer");
#endif
}


/** Start the Jazz server.

\param p_sig_handler, a function (of type pSignalHandler) that will be called when the process receives a SIGTERM signal.
\param p_daemon, returns by reference the pointer that will be used to control the MHD_Daemon.

\return			On failure, EXIT_FAILURE. On success, the thread forks and only the parent process returns EXIT_SUCCESS, the child does not return.
The application is stopped when callback signalHandler_SIGTERM exits with EXIT_SUCCESS if shutting all services was successful or with EXIT_FAILURE
if not. On failure, the caller is responsible of stopping all started services (see jazz_main.cpp).

Starting logic:

 1. Get all the MHD server config settings from p_config

	The default config file is JAZZ_DEFAULT_CONFIG_PATH but that can be changed via command line argument (see jazz_main.cpp).

 2. Registers the signal handlers for SIGTERM. (See argument p_sig_handler)

 3. Forks (== The parent exits with EXIT_SUCCESS, the child continues to call MHD_start_daemon().)

 4. Calls MHD_start_daemon()

	Then calls setsid() This creates a new session if the calling process is not a process group leader. The calling process is the leader of the
	new session, the process group leader of the new process group, and has no controlling terminal.

	And sleeps forever! (Remember, it is the child of the original caller who exited with EXIT_SUCCESS.)
*/
int JazzHttpServer::StartServer (pSignalHandler	 p_sig_handler,
								 pMHD_Daemon	&p_daemon)
{
// 1. Get all the MHD server config settings from p_config

	int http_port;

	if (!p_config->get_key("HTTP_PORT", http_port)) {
		cout << "Failed to find server port in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed to find server port in configuration.");

		return EXIT_FAILURE;
	}

	int ok, debug, ssl, ipv6, pedantic, supp_date, tcp_fastopen;

	ok =   p_config->get_key("MHD_DEBUG", debug)
		 & p_config->get_key("MHD_SSL", ssl)
		 & p_config->get_key("MHD_IPv6", ipv6)
		 & p_config->get_key("MHD_PEDANTIC_CHECKS", pedantic)
		 & p_config->get_key("MHD_SUPPRESS_DATE", supp_date)
		 & p_config->get_key("MHD_USE_TCP_FASTOPEN", tcp_fastopen);

	if ((!ok ) | ((debug | ssl | ipv6 | pedantic | supp_date | tcp_fastopen) & 0xfffffffe)) {
		cout << "Failed parsing flags block in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed config in flags block.");

		return EXIT_FAILURE;
	}
	unsigned int server_flags =	  debug*MHD_USE_DEBUG | ssl*MHD_USE_SSL | ipv6*MHD_USE_IPv6
								| pedantic*MHD_USE_PEDANTIC_CHECKS | supp_date*MHD_SUPPRESS_DATE_NO_CLOCK | tcp_fastopen*MHD_USE_TCP_FASTOPEN
								| MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;		// Only threading model for Jazz.

	MHD_OptionItem server_options[9];		// The variadic parameter MHD_OPTION_ARRAY, server_options, MHD_OPTION_END in MHD_start_daemon()

	int cml, cmi, ocl, oct, pic, tps, tss, lar;

	ok =   p_config->get_key("MHD_CONN_MEMORY_LIMIT", cml)
		 & p_config->get_key("MHD_CONN_MEMORY_INCR", cmi)
		 & p_config->get_key("MHD_CONN_LIMIT", ocl)
		 & p_config->get_key("MHD_CONN_TIMEOUT", oct)
		 & p_config->get_key("MHD_PER_IP_CONN_LIMIT", pic)
		 & p_config->get_key("MHD_THREAD_POOL_SIZE", tps)
		 & p_config->get_key("MHD_THREAD_STACK_SIZE", tss)
		 & p_config->get_key("MHD_LISTEN_ADDR_REUSE", lar);

	if (!ok) {
		cout << "Failed parsing integers block in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed config in integers block.");

		return EXIT_FAILURE;
	}

	MHD_OptionItem *pop = server_options;

	if (cml) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_LIMIT,	   cml, NULL}; pop++;
	if (cmi) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_INCREMENT, cmi, NULL}; pop++;
	if (ocl) pop[0] = {MHD_OPTION_CONNECTION_LIMIT,			   ocl, NULL}; pop++;
	if (oct) pop[0] = {MHD_OPTION_CONNECTION_TIMEOUT,		   oct, NULL}; pop++;
	if (pic) pop[0] = {MHD_OPTION_PER_IP_CONNECTION_LIMIT,	   pic, NULL}; pop++;
	if (tps) pop[0] = {MHD_OPTION_THREAD_POOL_SIZE,			   tps, NULL}; pop++;
	if (tss) pop[0] = {MHD_OPTION_THREAD_STACK_SIZE,		   tss, NULL}; pop++;
	if (lar) pop[0] = {MHD_OPTION_LISTENING_ADDRESS_REUSE,	   lar, NULL}; pop++;

	pop[0] = {MHD_OPTION_END, 0, NULL};

// 2. Register the signal handlers for SIGTERM

	int sig_ok = signal(SIGTERM, p_sig_handler) != SIG_ERR;

	if (!sig_ok) {
		cout << "Failed to register signal handlers." << endl;

		log(LOG_ERROR, "Failed to register signal handlers.");

		return EXIT_FAILURE;
	}

// 3. Forks

	pid_t pid = fork();
	if (pid < 0) {
		cout << "Failed to fork." << endl;

		log(LOG_ERROR, "Failed to fork.");

		return EXIT_FAILURE;
	}
	if (pid > 0) return EXIT_SUCCESS; // This is parent process, exit now.

// 4. Calls MHD_start_daemon()

	cout << "Starting server on port : " << http_port << endl;

//TODO: Implement an MHD_AcceptPolicyCallback when security is taken in consideration

	p_daemon = MHD_start_daemon (server_flags, http_port, NULL, NULL, JazzCallbackAnswerHTTP, NULL, MHD_OPTION_ARRAY, server_options, MHD_OPTION_END);

	if (p_daemon == NULL)
	{
//TODO: See what could replace "jServices.stop_all()" when MHD_start_daemon fails

		cout << "Failed to start the server." << endl;

		log(LOG_ERROR, "Failed to start the server.");
	}

// Creates a new session if the calling process is not a process group leader. The calling process is the leader of the new session,
// the process group leader of the new process group, and has no controlling terminal.

	setsid();

#ifdef DEBUG
	cout << endl << "DEBUG MODE: -- Press any key to stop the server. ---" << endl;
	getchar();
	cout << endl << "Stopping ..." << endl;
	kill(getpid(), SIGTERM);
	sleep(1);
	cout << endl << "Failed :-(" << endl;
#endif

	while(true) sleep(60);
}

} // namespace jazz_restapi


#if defined CATCH_TEST
#include "src/jazz_main/tests/test_restapi.ctest"
#endif

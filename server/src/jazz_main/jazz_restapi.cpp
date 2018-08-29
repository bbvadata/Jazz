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

#ifdef DEBUG
jazz_utils::JazzLogger	httpLog("http_log");

int print_out_key (void *cls, enum MHD_ValueKind kind,
				   const char *key, const char *value)
{
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);

	return MHD_YES;
}
#endif



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
#ifdef DEBUG
	httpLog.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - cls \x20 \x20 \x20 \x20 \x20 \x20 \x20: %p", cls);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - connection \x20 \x20 \x20 : %p", connection);
	MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - url \x20 \x20 \x20 \x20 \x20 \x20 \x20: %s", url);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - method \x20 \x20 \x20 \x20 \x20 : %s", method);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - version \x20 \x20 \x20 \x20 \x20: %s", version);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - upload_data \x20 \x20 \x20: %p", upload_data);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - upload_data_size : %d", *upload_data_size);
	httpLog.log_printf(LOG_DEBUG, "| HTTP callback - *con_cls \x20 \x20 \x20 \x20 : %p", *con_cls);
	httpLog.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
#endif

/*	// Step 1: First opportunity to end the connection before uploading or getting. Not used. We initialize con_cls for the next call.

	if (*con_cls == NULL)
	{
		*con_cls = &state_new_call;

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int i_method = i_methods[jazz_utils::TenBitsAtAddress(method)];

	parsedURL pars;

	if (*con_cls == &state_upload_in_progress)
	{
		if (*upload_data_size == 0)
			goto create_response_answer_put_ok;

		if (i_method != HTTP_PUT || !jAPI.parse_url(url, HTTP_PUT, pars) || pars.isInstrumental || pars.hasAFunction)
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

	switch (i_method)
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

					URLattrib u_attr;

					if (jAPI.get_url(url, u_attr))
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
				if (!jAPI.parse_url(url, i_method, pars))
				{
					if (i_method == HTTP_PUT)
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
		jCommons.enter_api_call(connection, url, i_method);

	switch (i_method)
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

		if (i_method == HTTP_PUT) goto continue_in_put_unavailable;

		status = MHD_HTTP_SERVICE_UNAVAILABLE;
	}

	if (sample)
		jCommons.leave_api_call(status);

	// Step 6 : The core finished, just distribute the answer as appropriate.

	if (i_method == HTTP_PUT)
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

	if (i_method == HTTP_DELETE)
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
struct block_URL_dictionary : jzzBlockHeader
{
	URLattrib attr[];		///< The array of block_URL_dictionary
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

		bool get_url (const char * url, URLattrib &u_attr);

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
	jBlockC.char_to_key("MY_URL_DICT", block);
	if (!jBlockC.block_get (SYSTEM_SOURCE_SYS, block, dict))
	{
		jCommons.log(LOG_INFO, "Opening all websources at jazzWebSource::open_all_websources(): No websources found.");

		return true;
	}

	for (int i = 0; i < dict->length; i++)
	{
		URLattrib * p_u_att = &reinterpret_cast<block_URL_dictionary *>(dict)->attr[i];

		new_websource(p_u_att->websource.key);

		if (!set_url_to_block (p_u_att->url, p_u_att->block.key, p_u_att->websource.key))
			goto exit_fail;

		if (p_u_att->blocktype != BLOCKTYPE_RAW_MIME_HTML && !set_mime_to_block(p_u_att->blocktype, p_u_att->block.key, p_u_att->websource.key))
			goto exit_fail;

		if (p_u_att->language != LANG_DONT_CARE && !set_lang_to_block(HTTP_LANGUAGE_STRING[p_u_att->language], p_u_att->block.key, p_u_att->websource.key))
			goto exit_fail;
	}

	jCommons.log_printf(LOG_INFO, "Opening all websources at jazzWebSource::open_all_websources(): %d websources found.", dict->length);

	jBlockC.block_unprotect (dict);

	return true;


exit_fail:

	jBlockC.block_unprotect (dict);

	return false;
}


/** Create a new websource.

	\param websource The name of the new websource.
	\return			 true if the websource does not already exist.
*/
bool jazzWebSource::new_websource(const char * websource)
{
	persistedKey key;
	if (!jBlockC.char_to_key(websource, key))
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
			if (!jBlockC.char_to_key(it->first.c_str(), key))
			{
				jCommons.log(LOG_MISS, "jazzWebSource::kill_websource(): block name fails char_to_key().");

				return false;
			}
			if (!jBlockC.block_kill(SYSTEM_SOURCE_WWW, key))
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
	if (!url[0] || strlen(url) > MAX_URL_LENGTH || !jBlockC.char_to_key(block, key))
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

	int i_lang;

	for (i_lang = 1; i_lang < LANG_LAST_; i_lang++)
	{
		if (!strcmp(lang, HTTP_LANGUAGE_STRING[i_lang]))
		{
			block_lang[block] = i_lang;

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

	jBlockC.char_to_key("MY_URL_DICT", dict_block_key);
	jBlockC.block_kill(SYSTEM_SOURCE_SYS, dict_block_key);

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
	block_URL_dictionary * p_dict;
	bool ok = JAZZALLOC(p_dict, RAM_ALLOC_C_RAW, size);

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
			URLattrib * p_u_att = &p_dict->attr[i++];

			strcpy(p_u_att->url, it->first.c_str());
			strcpy(p_u_att->block.key, block.c_str());
			strcpy(p_u_att->websource.key, block_source[block].c_str());
			p_u_att->blocktype = block_mime[block];
			p_u_att->language	= block_lang[block];
		}
	}

	p_dict->type	  = BLOCKTYPE_MY_URL_DICT;
	p_dict->length = numitems;
	if (!jBlockC.block_put(SYSTEM_SOURCE_SYS, dict_block_key, p_dict))
	{
		jCommons.log(LOG_MISS, "jazzWebSource::close_all_websources(): jBlockC.block_put failed.");

		JAZZFREE(p_dict, RAM_ALLOC_C_RAW);

		return false;
	}

	sources		 = {};
	url_block	 = {};
	block_mime	 = {};
	block_lang	 = {};
	block_source = {};

	JAZZFREE(p_dict, RAM_ALLOC_C_RAW);

	jCommons.log_printf(LOG_INFO, "Closing all websources at jazzWebSource::close_all_websources(): %d websources saved.", numitems);

	return true;
}

/*	-----------------------------------------------
		Function for the  A P I
--------------------------------------------------- */

/** Get the following http attributes for a url: block, type and language.

	\param url	 The url to be tried.
	\param u_attr A URLattrib record with all the http attributes.
	\return		 true if found. No log() on miss.
*/
bool jazzWebSource::get_url(const char * url, URLattrib &u_attr)
{
	try
	{
		if (!jBlockC.char_to_key(url_block[url].c_str(), u_attr.block))
			return false;

		u_attr.blocktype = block_mime[u_attr.block.key];
		u_attr.language	= block_lang[u_attr.block.key];

		return true;
	}
	catch (...)
	{
		return false;
	}
}


/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

jzzAPI jAPI;
int	   i_methods[1024];


/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

#define MHD_HTTP_ANYERROR 400


/// Pointers to these global variables control the state of a PUT call.
int	state_new_call				= 0;	///< Default state: connection open for any call
int	state_upload_in_progress	= 1;	///< Data was uploaded, the function was executed and it returned true.
int	state_upload_notacceptable	= 2;	///< Data uploaded failed, the function was executed and it failed. Will return MHD_HTTP_NOT_ACCEPTABLE
int	state_upload_unavailable	= 3;	///< Data uploaded failed, call to enter_persistence() failed. Must end with MHD_HTTP_SERVICE_UNAVAILABLE.
int	state_upload_badrequest		= 4;	///< PUT call malformed, Must end with MHD_HTTP_BAD_REQUEST.

char response_put_ok[]			= "0";
char response_put_fail[]		= "1";

bool no_storage, no_webpages;

int tenbitDS;							///< The binary ten bits of "//" double-slash to identify web source interface.


/*	-----------------------------------------------
		p a r s i n g  method
--------------------------------------------------- */

/** Parse an API url.

	\param	url	   The http url
	\param	method The http method in [HTTP_NOTUSED..HTTP_DELETE]
	\param	pars   A structure with the parts of the url successfully parsed.

	\return		   true if successful, does not log.
*/
bool jzzAPI::parse_url(const char * url, int method, parsedURL &pars)
{
	switch (method)
	{
		case HTTP_HEAD:
			method = HTTP_GET;
			break;

		case HTTP_GET:
		case HTTP_PUT:
		case HTTP_DELETE:

			break;

		default:

			return false;
	}

	memset(&pars, 0, sizeof(ParsedUrlHea));
	int i;

	// url is already verified to start with "//"

	switch (url[2])	// syntactic sugar
	{
		case 0:
			goto list_all_sources;

		case '/':
			goto full_server_id;
	}

	persistedKey source;

	if (!char_to_key_relaxed_end(&url[2], source))
		return false;

	pars.source = jBlockC.get_source_idx(source.key);
	if (pars.source < 0)
		return false;

	i = 2 + strlen(source.key);

	switch (url[i])
	{
		case '.':
			i++;
			goto source_key;

		case '/':
			i++;
			switch (url[i])
			{
				case '/':
					i++;
					goto anything_function_parameters;

				case 0:	// - List all meshes from a source.
					if (method != HTTP_GET)
						return false;

					pars.hasAFunction	= true;
					pars.isInstrumental	= true;

					strcpy(pars.function.name, "ls");
					pars.parameters.param[0] = 0;
					pars.instrument.name[0]	 = 0;

					return true;

				default:
					goto source_mesh;
			}

		case 0:	// - Delete a source.
			pars.deleteSource = true;
			return method == HTTP_DELETE;
	}

	return false;


list_all_sources:	// - List all sources.

	if (method != HTTP_GET)
		return false;

	pars.hasAFunction = true;

	pars.source = 0;

	strcpy(pars.function.name, "ls");
	pars.parameters.param[0] = 0;

	return true;


full_server_id:		// - //sys//server_vers/full

	if (method != HTTP_GET || url[3] != 0)
		return false;

	pars.hasAFunction = true;

	pars.source = 0;

	strcpy(pars.function.name, "server_vers");
	strcpy(pars.parameters.param, "full");

	return true;


source_key:

	if (!char_to_key_relaxed_end(&url[i], pars.key))
		return false;

	i += strlen(pars.key.key);

	pars.hasAKey = true;

	switch (url[i])
	{
		case '.':
			i++;
			goto anything_function_parameters;

		case 0:	// - Get a block, Write a block & Delete a block.
			return true;
	}

	return false;


anything_function_parameters:

	if (method == HTTP_DELETE)
		return false;

	if (!char_to_instrum_relaxed_end(&url[i], pars.function))
		return false;

	pars.hasAFunction = true;

	i += strlen(pars.function.name);

	switch (url[i])
	{
		case '/':
			return char_to_param_strict_end(&url[++i], pars.parameters);

		case 0:
			pars.parameters.param[0] = 0;

			return true;
	}

	return false;


source_mesh:

	if (!char_to_key_relaxed_end(&url[i], pars.mesh))
		return false;

	i += strlen(pars.mesh.key);

	pars.isInstrumental = true;
	pars.instrument.name[0] = 0;

	switch (url[i])
	{
		case '.':
			i++;
			goto anything_function_parameters;

		case '/':
			i++;
			switch (url[i])
			{
				case 0:	// - List all instruments in a mesh.
					if (method != HTTP_GET)
						return false;

					pars.hasAFunction = true;
					strcpy(pars.function.name, "ls");
					pars.parameters.param[0] = 0;

					return true;

				default:
					goto source_mesh_instrument;
			}

		case 0:	// - Delete a mesh.
			return method == HTTP_DELETE;
	}

	return false;


source_mesh_instrument:

	if (!char_to_instrum_relaxed_end(&url[i], pars.instrument))
		return false;

	i += strlen(pars.instrument.name);

	switch (url[i])
	{
		case '.':
			i++;
			goto anything_function_parameters;

		case 0:	// - Delete a mesh.
			return true;
	}

	return false;
}

/*	-----------------------------------------------
		deliver http  e r r o r	 pages
--------------------------------------------------- */

/** Finish a query by delivering the appropriate message page.

	\param connection  The MHD connection passed to the callback function. (Needed for MHD_queue_response()ing the response.)
	\param http_status The http status error (e.g., MHD_HTTP_NOT_FOUND)

	\return			   A valid answer for an MHD callback. The callback returns: return jAPI.return_error_message(MHD_HTTP_METHOD_NOT_ALLOWED);

	This function searches for a persistence block named ("www", "httpERR_%d") where %d is the code in decimal and serves it as an answer.
*/
int jzzAPI::return_error_message(struct MHD_Connection *connection, int http_status)
{
	persistedKey key;

	sprintf(key.key, "httpERR_%d", http_status);

	pJazzBlock block;

	struct MHD_Response * response;

	if (jBlockC.get_source_idx("www") == 1 && jBlockC.block_get(1, key, block))
	{
		response = MHD_create_response_from_buffer (block->size, &reinterpret_cast<pRawBlock>(block)->data, MHD_RESPMEM_MUST_COPY);

		jBlockC.block_unprotect (block);
	}
	else
	{
		char buff[256];

		sprintf(buff, "<html><body><h1><br/><br/>Http error : %d.</h1></body></html>", http_status);

		response = MHD_create_response_from_buffer (strlen(buff), buff, MHD_RESPMEM_MUST_COPY);
	}

	int ret = MHD_queue_response (connection, http_status, response);

	MHD_destroy_response (response);

	return ret;
}

/*	---------------------------------------------------
		execution method for the  w w w	 API
------------------------------------------------------- */

/** Execute a get www resource.

	\param url		 The unparsed url as seen by the callback.
	\param response	 A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
bool jzzAPI::exec_www_get(const char * url, struct MHD_Response * &response)
{
	URLattrib u_attr;

	if (!get_url(url, u_attr))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_www_get(): get_url() failed.");

		return false;
	}

	pJazzBlock pb;

	if (!jBlockC.block_get(1, u_attr.block, pb))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_www_get(): block_get() failed.");

		return false;
	}

	response = MHD_create_response_from_buffer (pb->size, &reinterpret_cast<pRawBlock>(pb)->data, MHD_RESPMEM_MUST_COPY);

	jBlockC.block_unprotect(pb);

	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_STRING[u_attr.blocktype]);

	if (u_attr.language != LANG_DONT_CARE)
		MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_LANGUAGE, HTTP_LANGUAGE_STRING[u_attr.language]);

	return true;
}

/*	---------------------------------------------------
		execution methods for the  b l o c k  API
------------------------------------------------------- */

/** Execute a get block using the block API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	response A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return	http status.
*/
int jzzAPI::exec_block_get(parsedURL &pars, struct MHD_Response * &response)
{
	if (!pars.source)
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_block_get(): direct block get to sys.");

		return MHD_HTTP_FORBIDDEN;
	}

	pJazzBlock pb;

	if (!jBlockC.block_get(pars.source, pars.key, pb))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_block_get(): block_get() failed.");

		return MHD_HTTP_NOT_FOUND;
	}

	response = MHD_create_response_from_buffer (pb->size, &reinterpret_cast<pRawBlock>(pb)->data, MHD_RESPMEM_MUST_COPY);

	jBlockC.block_unprotect(pb);

	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_STRING[BLOCKTYPE_RAW_ANYTHING]);

	return MHD_HTTP_OK;
}


/** Execute a get function using the block API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	response A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_block_get_function(parsedURL &pars, struct MHD_Response * &response)
{
	char page_2k[GET_FUN_BUFFER_SIZE];	// The size of this is checked by REQUIRE below.
	pJazzBlock pb_dest;

	switch (pars.source)
	{
		case 0:	// "sys" function
			if (!strcmp(pars.function.name, "server_vers"))
				goto return_server_version;

			if (!strcmp(pars.function.name, "ls"))
				goto return_list_sources;

			break;

		case 1:	// "www" function
			if (!strcmp(pars.function.name, "ls"))
				goto return_list_websources;

			break;

		default:
			pJazzBlock pb;

			if (pars.hasAKey)
			{
				if (!jBlockC.block_get(pars.source, pars.key, pb))
				{
					jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): block_get() failed.");

					return MHD_HTTP_NOT_FOUND;
				}

				if (!strcmp(pars.function.name, "header"))
				{
					sprintf(page_2k, "type:%d\nlength:%d\nsize:%d\nflags:%d\nhash64:%lx\n",
							pb->type, pb->length, pb->size, pb->flags, (long unsigned int) pb->hash64);

					jBlockC.block_unprotect(pb);

					goto return_page2k;
				}

				if (!strcmp(pars.function.name, "as_text"))
				{
					if (!jBlockC.translate_block_TO_TEXT(pb, pb_dest, pars.parameters.param))
					{
						jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): translate_block_TO_TEXT() failed.");

						jBlockC.block_unprotect(pb);

						return MHD_HTTP_NOT_ACCEPTABLE;
					}

					jBlockC.block_unprotect(pb);

					goto return_pb_dest;
				}

				if (!strcmp(pars.function.name, "as_R"))
				{
					if (!jBlockC.translate_block_TO_R(pb, pb_dest))
					{
						jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): translate_block_TO_R() failed.");

						jBlockC.block_unprotect(pb);

						return MHD_HTTP_NOT_ACCEPTABLE;
					}

					jBlockC.block_unprotect(pb);

					goto return_pb_dest;
				}

				jBlockC.block_unprotect(pb);
			}

			break;

	}

	jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function() failed: Unknown function.");

	return MHD_HTTP_NOT_ACCEPTABLE;


return_list_websources:

	char * pt;
	pt = (char *) &page_2k;
	for(map<string, int>::iterator it = sources.begin(); it != sources.end(); ++it)
	{
		if (it->second == 1)
		{
			strcpy(pt, it->first.c_str());
		}
		pt += it->first.length();

		*pt++ = '\n';
	}
	*pt++ = 0;
	goto return_page2k;


return_list_sources:

	pt = (char *) &page_2k;
	for (int i = 0; i < jBlockC.num_sources(); i++)
	{
		sourceName nam;
		jBlockC.source_name(i, nam);

		strcpy(pt, nam.key);
		pt += strlen(nam.key);

		*pt++ = '\n';
	}
	*pt++ = 0;
	goto return_page2k;


return_server_version:

	if (!strcmp(pars.parameters.param, "full"))
	{
#ifdef DEBUG
		string st ("DEBUG");
#else
		string st ("RELEASE");
#endif

		struct utsname unn;
		uname(&unn);
		string me;
		jCommons.get_config_key("JazzCLUSTER.JAZZ_NODE_WHO_AM_I", me);

		sprintf(page_2k, "Jazz\n\n version : %s\n build \x20 : %s\n artifact: %s\n my name : %s\n "
				"sysname : %s\n hostname: %s\n kernel\x20 : %s\n sysvers : %s\n machine : %s",
				JAZZ_VERSION, st.c_str(), LINUX_PLATFORM, me.c_str(),
				unn.sysname, unn.nodename, unn.release, unn.version, unn.machine);
	}
	else
		sprintf(page_2k, JAZZ_VERSION);


return_page2k:

	response = MHD_create_response_from_buffer (strlen (page_2k), (void *) &page_2k, MHD_RESPMEM_MUST_COPY);

	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_STRING[BLOCKTYPE_RAW_STRINGS]);

	return MHD_HTTP_OK;


return_pb_dest:

	response = MHD_create_response_from_buffer (pb_dest->size, &reinterpret_cast<pRawBlock>(pb_dest)->data, MHD_RESPMEM_MUST_COPY);

	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_STRING[pb_dest->type]);

	JAZZFREE(pb_dest, AUTOTYPEBLOCK(pb_dest));

	return MHD_HTTP_OK;
}


/**	 Execute a put block using the block API.

	\param	pars   The structure containing the parts of the url successfully parsed by parse_url().
	\param	upload A pointer to the data uploaded with the http PUT call.
	\param	size   The size of the data uploaded with the http PUT call.
	\param	reset  Start a sequence resetting the block from scratch. Otherwise, the upload is added at the end of the already existing block.

	\return		   true if successful, false and log(LOG_MISS, "further details") if not.

	This function performs block PUT incrementally. The first time, reset == true and the block is created, all other times, the block is appended at
the end on the existing block.
*/
bool jzzAPI::exec_block_put(parsedURL &pars, const char * upload, size_t size, bool reset)
{
	if (!pars.source)
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): direct block put to sys.");

		return false;
	}

	pRawBlock pb;

	if (reset)
	{
		if (!jBlockC.new_block_C_RAW_once(pb, upload, size))
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): new_block_C_RAW_once() failed.");

			return false;
		}
	}
	else
	{
		pJazzBlock pb2;

		if (!jBlockC.block_get(pars.source, pars.key, pb2))
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): block_get() failed.");

			return false;
		}

		bool ok = JAZZALLOC(pb, RAM_ALLOC_C_RAW, pb2->size + size);
		if (!ok)
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): JAZZALLOC() failed.");

			jBlockC.block_unprotect (pb2);

			return false;
		}

		uint8_t * pt = (uint8_t *) &pb->data;
		memcpy(pt, &reinterpret_cast<pRawBlock>(pb2)->data, pb2->size);

		jBlockC.block_unprotect (pb2);

		pt += pb2->size;
		memcpy(pt, upload, size);
	}

	if (!jBlockC.block_put(pars.source, pars.key, pb))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): block_put() failed.");

		JAZZFREE(pb, RAM_ALLOC_C_RAW);

		return false;
	}

	JAZZFREE(pb, RAM_ALLOC_C_RAW);

	return true;
}


/**	 Execute a put function using the block API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	upload	 A pointer to the data uploaded with the http PUT call.
	\param	size	 The size of the data uploaded with the http PUT call.

	\return			 true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzAPI::exec_block_put_function(parsedURL &pars, const char * upload, size_t size)
{
	if (pars.hasAKey)
	{
		switch (pars.source)
		{
			case 0:
				jCommons.log(LOG_MISS, "exec_block_put_function() tried sys.block.");

				return false;

			case 1:
				websourceName websource;

				if (!jBlockC.char_to_key((const char *) &pars.parameters, websource))
				{
					jCommons.log(LOG_MISS, "exec_block_put_function() char_to_key() failed.");

					return false;
				}

				if (!strcmp(pars.function.name, "assign_url"))
				{
					if (size > MAX_URL_LENGTH)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() assign_url, wrong size.");

						return false;
					}
					char buff[MAX_URL_LENGTH + 1];
					strncpy(buff, upload, size);
					buff[size] = 0;
					return set_url_to_block(buff, pars.key.key, websource.key);
				}
				if (!strcmp(pars.function.name, "assign_mime_type"))
				{
					if (size != sizeof(int))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() assign_mime_type, wrong size.");

						return false;
					}

					return set_mime_to_block(* reinterpret_cast<const int *>(upload), pars.key.key, websource.key);
				}
				if (!strcmp(pars.function.name, "assign_language"))
				{
					if (size > MAX_URL_LENGTH)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() assign_language, wrong size.");

						return false;
					}
					char buff[MAX_URL_LENGTH + 1];
					strncpy(buff, upload, size);
					buff[size] = 0;
					return set_lang_to_block(buff, pars.key.key, websource.key);
				}

				break;

			default:
				if (!strcmp(pars.function.name, "header"))
				{
					if (!strcmp(pars.parameters.param, "type"))
					{
						if (size != sizeof(int))
						{
							jCommons.log(LOG_MISS, "exec_block_put_function() size != sizeof(int).");

							return false;
						}

						int type = reinterpret_cast<const int *>(upload)[0];

						return jBlockC.cast_lmdb_block(pars.source, pars.key, type);
					}
					if (!strcmp(pars.parameters.param, "flags"))
					{
						if (size != sizeof(int))
						{
							jCommons.log(LOG_MISS, "exec_block_put_function() flags, size != sizeof(int).");

							return false;
						}

						int flags = reinterpret_cast<const int *>(upload)[0];

						return jBlockC.set_lmdb_blockflags(pars.source, pars.key, flags);
					}
				}
				if (!strcmp(pars.function.name, "from_text"))		// parameters: type,source_key	upload: -nothing-
				{
					int type;
					char source_key[MAX_PARAM_LENGTH];
					char fmt[MAX_PARAM_LENGTH];
					fmt[0] = 0;
					persistedKey key;
					if (	sscanf(pars.parameters.param, "%d,%15[a-zA-Z0-9_],%15s", &type, source_key, fmt) < 2	// empty fmt is valid
						|| !jBlockC.char_to_key(source_key, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() from_text, parse params failed.");

						return false;
					}

					return jBlockC.set_lmdb_fromtext(pars.source, pars.key, type, key, fmt);
				}
				if (!strcmp(pars.function.name, "from_R"))			// parameters: source_key		upload: -nothing-
				{
					persistedKey key;

					if(!jBlockC.char_to_key(pars.parameters.param, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() from_R, char_to_key() failed.");

						return false;
					}
					return jBlockC.set_lmdb_fromR(pars.source, pars.key, key);
				}
				if (!strcmp(pars.function.name, "C_bool_rep"))		// parameters: x,times			upload: -nothing-
				{
					unsigned int x = JAZZ_BOOLEAN_NA, times;
					bool ok = sscanf(pars.parameters.param, "NA,%u", &times) == 1;
					if (!ok)
						ok = sscanf(pars.parameters.param, "%u,%u", &x, &times) == 2 && x < 2;
					if (!ok)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_bool_rep, parse params failed.");

						return false;
					}
					pBoolBlock pb;
					if (!jBlockC.new_block_C_BOOL_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_bool_rep, new_block_C_BOOL_rep() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_bool_rep, block_put() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_BOOL);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_BOOL);

					return true;
				}
				if (!strcmp(pars.function.name, "C_integer_rep"))	// parameters: x,times			upload: -nothing-
				{
					int x = JAZZ_INTEGER_NA;
					unsigned int times;
					bool ok = sscanf(pars.parameters.param, "NA,%u", &times) == 1;
					if (!ok)
						ok = sscanf(pars.parameters.param, "%d,%u", &x, &times) == 2;
					if (!ok)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_rep, parse params failed.");

						return false;
					}
					pIntBlock pb;
					if (!jBlockC.new_block_C_INTEGER_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_rep, new_block_C_INTEGER_rep() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_rep, block_put() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_INTEGER);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_INTEGER);

					return true;
				}
				if (!strcmp(pars.function.name, "C_integer_seq"))	// parameters: from,to,by		upload: -nothing-
				{
					int from, to, by;
					if (sscanf(pars.parameters.param, "%d,%d,%d", &from, &to, &by) != 3)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_seq, parse params failed.");

						return false;
					}
					pIntBlock pb;
					if (!jBlockC.new_block_C_INTEGER_seq(pb, from, to, by))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_seq, new_block_C_INTEGER_seq() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_seq, block_put() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_INTEGER);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_INTEGER);

					return true;
				}
				if (!strcmp(pars.function.name, "C_real_rep"))		// parameters: x,times			upload: -nothing-
				{
					double x = JAZZ_DOUBLE_NA;
					unsigned int times;
					bool ok = sscanf(pars.parameters.param, "NA,%u", &times) == 1;
					if (!ok)
						ok = sscanf(pars.parameters.param, "%lf,%u", &x, &times) == 2;
					if (!ok)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_rep, parse params failed.");

						return false;
					}
					pRealBlock pb;
					if (!jBlockC.new_block_C_REAL_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_rep, new_block_C_REAL_rep() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_rep, block_put() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_REAL);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_REAL);

					return true;
				}
				if (!strcmp(pars.function.name, "C_real_seq"))		// parameters: from,to,by		upload: -nothing-
				{
					double from, to, by;
					if (sscanf(pars.parameters.param, "%lf,%lf,%lf", &from, &to, &by) != 3)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_seq, parse params failed.");

						return false;
					}
					pRealBlock pb;
					if (!jBlockC.new_block_C_REAL_seq(pb, from, to, by))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_seq, new_block_C_REAL_seq() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_seq, block_put() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_REAL);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_REAL);

					return true;
				}
				if (!strcmp(pars.function.name, "C_chars_rep"))		//	  parameters: str,times		upload: -nothing-
				{													// or parameters: times			upload: string
					char pt [MAX_PARAM_LENGTH];
					unsigned int times;
					if (size > MAX_PARAM_LENGTH - 1)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_chars_rep, wrong uploaded size.");

						return false;
					}
					if (size) strncpy(pt, upload, size);
					pt[size] = 0;
					if (sscanf(pars.parameters.param, "%u", &times) != 1)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_chars_rep, parse params failed.");

						return false;
					}
					pCharBlock pb;
					if (!jBlockC.new_block_C_CHARS_rep(pb, pt, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_chars_rep, new_block_C_CHARS_rep() failed.");

						return false;
					}
					jBlockC.hash_block((pJazzBlock) pb);
					if (!jBlockC.block_put(pars.source, pars.key, (pJazzBlock) pb))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_rep, C_chars_rep() failed.");

						JAZZFREE(pb, RAM_ALLOC_C_OFFS_CHARS);

						return false;
					}
					JAZZFREE(pb, RAM_ALLOC_C_OFFS_CHARS);

					return true;
				}

				break;
		}
	}
	else
	{
		char buff[MAX_KEY_LENGTH];
		if (size < 1 || size >= MAX_KEY_LENGTH)
		{
			jCommons.log(LOG_MISS, "exec_block_put_function() non-block functions expect an uploaded key.");

			return false;
		}
		strncpy(buff, upload, size);
		buff[size] = 0;
		switch (pars.source)
		{
			case 0:

				if (!strcmp(pars.function.name, "new_source"))
				{
					persistedKey key;

					if (!jBlockC.char_to_key(buff, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() new_source, char_to_key() failed.");

						return false;
					}

					return jBlockC.new_source(key.key);
				}
				break;

			case 1:

				if (!strcmp(pars.function.name, "new_websource"))
				{
					websourceName name;

					if (!jBlockC.char_to_key(buff, name))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() new_websource, char_to_key() failed.");

						return false;
					}

					return new_websource(name.key);
				}
				if (!strcmp(pars.function.name, "delete_websource"))
				{
					return kill_websource(buff);
				}
				break;
		}
	}

	jCommons.log(LOG_MISS, "exec_block_put_function() Unknown function.");

	return false;
}


/**	 Execute a kill block using the block API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_block_kill(parsedURL &pars)
{
	if (pars.hasAKey)									// Delete block
		if (jBlockC.block_kill(pars.source, pars.key))
			return MHD_HTTP_NO_CONTENT;
		else
		{
			jCommons.log(LOG_MISS, "exec_block_kill() block_kill() failed.");

			return MHD_HTTP_NOT_FOUND;
		}

	sourceName name;									// Delete source

	if (pars.source <= SYSTEM_SOURCE_WWW)
	{
		jCommons.log(LOG_MISS, "exec_block_kill() pars.source <= SYSTEM_SOURCE_WWW.");

		return MHD_HTTP_FORBIDDEN;
	}

	if (!jBlockC.source_name(pars.source, name))
	{
		jCommons.log(LOG_MISS, "exec_block_kill() source_name() failed.");

		return MHD_HTTP_NOT_ACCEPTABLE;
	}

	if (jBlockC.kill_source(name.key))
		return MHD_HTTP_NO_CONTENT;
	else
	{
		jCommons.log(LOG_MISS, "exec_block_kill() kill_source() failed.");

		return MHD_HTTP_NOT_FOUND;
	}
}

/*	---------------------------------------------------------------
		execution methods for the  i n s t r u m e n t a l	API
------------------------------------------------------------------- */

/** Execute a get block using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	response A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_instr_get(parsedURL &pars, struct MHD_Response * &response)
{
	jCommons.log(LOG_MISS, "exec_instr_get()");

//_in_deprecated_code_TODO: Implement exec_instr_get().

	return MHD_HTTP_NOT_IMPLEMENTED;
}


/** Execute a get function using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	response A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_instr_get_function(parsedURL &pars, struct MHD_Response * &response)
{
	jCommons.log(LOG_MISS, "exec_instr_get_function()");

//_in_deprecated_code_TODO: Implement exec_instr_get_function().

	return MHD_HTTP_NOT_IMPLEMENTED;
}


/**	 Execute a put block using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	upload	 A pointer to the data uploaded with the http PUT call.
	\param	size	 The size of the data uploaded with the http PUT call.

	\return			 true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzAPI::exec_instr_put(parsedURL &pars, const char * upload, size_t size)
{
	jCommons.log(LOG_MISS, "exec_instr_put()");

//_in_deprecated_code_TODO: Implement exec_instr_put().

	return false;
}


/**	 Execute a put function using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().
	\param	upload	 A pointer to the data uploaded with the http PUT call.
	\param	size	 The size of the data uploaded with the http PUT call.

	\return			 true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool jzzAPI::exec_instr_put_function(parsedURL &pars, const char * upload, size_t size)
{
	jCommons.log(LOG_MISS, "exec_instr_put_function()");

//_in_deprecated_code_TODO: Implement exec_instr_put_function().

	return false;
}


/**	 Execute a kill block using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_instr_kill(parsedURL &pars)
{
	jCommons.log(LOG_MISS, "exec_instr_kill()");

//_in_deprecated_code_TODO: Implement exec_instr_kill().

	return MHD_HTTP_NOT_IMPLEMENTED;
}

#endif
//TODO: Remove section LEGACY_MHD_CODE when refactoring is complete.

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

\param p_sig_handler	A function (of type pSignalHandler) that will be called when the process receives a SIGTERM signal.
\param p_daemon			Returns by reference the pointer that will be used to control the MHD_Daemon.

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

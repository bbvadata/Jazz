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

/*! \brief API class implementing the non-security related functionality of the Jazz server.

This module implements both the callback function (which can be a direct callback or called through jzzPERIMETRAL) and a class that parses API calls.

The callback function calls the appropriate instanced services and handles the http responses on successful url parsing or tries a jazzWebsource
call with the url if not. If that call is successful it provides the resource, if not it delivers a 404 (with a possible 404 resource).
*/

#include "src/jazz01_main/jazz_api.h"

/*~ end of automatic header ~*/

#include <sys/utsname.h>
#include <math.h>

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

jzzAPI jAPI;
int	   imethods[1024];


/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

#define MHD_HTTP_ANYERROR 400

#ifdef DEBUG
int print_out_key (void *cls, enum MHD_ValueKind kind,
				   const char *key, const char *value)
{
	jCommons.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);
	return MHD_YES;
}
#endif


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


/** Callback function for MHD. The details explaining the interface can be found at: .../da_jazz_JAZZ/doc/MHD_TUT/mhdTutorial.html

	Jazz does not use post processor callbacks linked with MHD_create_post_processor().
	Jazz does not use request completed callbacks linked with MHD_start_daemon().

	The only callback functions are:

		1. This (The full operational blocks and instrumental API)
		2. perimetral_answer_to_connection() - A security wrapper that decides which calls to this are acceptable.
		3. jazz_apc_callback()				 - An IP based (firewall like) that can filter based on called IPs.

	This function is multithreaded with the default configuration settings. (Other settings are untested for the moment.)

	The only reference of the functionality of the API is: .../da_jazz_JAZZ/doc/RFC/rest_api.html

	The internal operation of the callback function is subject to change and the remarks in the source code are the description of it.
*/
int jazz_answer_to_connection(void *cls,
							  struct MHD_Connection *connection,
							  const char *url,
							  const char *method,
							  const char *version,
							  const char *upload_data,
							  size_t *upload_data_size,
							  void **con_cls)
{
	// Step 1: First opportunity to end the connection before uploading or getting. Not used. We initialize con_cls for the next call.

	if (*con_cls == NULL)
	{
		*con_cls = &state_new_call;

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int imethod;
	imethod = imethods[tenbits(method)];

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
				if (tenbits(url) != tenbitDS)
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
			if (tenbits(url) != tenbitDS)
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
			if (tenbits(url) != tenbitDS)
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
	return MHD_NO;
}

/*	-----------------------------------------------
	  C o n s t r u c t o r / d e s t r u c t o r
--------------------------------------------------- */

jzzAPI::jzzAPI()
{
	for (int i = 0; i < 1024; i++) imethods[i] = HTTP_NOTUSED;

	imethods[tenbits("OPTIONS")] = HTTP_OPTIONS;
	imethods[tenbits("HEAD")]	 = HTTP_HEAD;
	imethods[tenbits("GET")]	 = HTTP_GET;
	imethods[tenbits("PUT")]	 = HTTP_PUT;
	imethods[tenbits("DELETE")]	 = HTTP_DELETE;

	tenbitDS = tenbits("//");
};


jzzAPI::~jzzAPI()
{
};

/*	-----------------------------------------------
	 Methods inherited from	 j a z z S e r v i c e
--------------------------------------------------- */

/** Start of jzzAPI

	see jazzService::start()
*/
bool jzzAPI::start()
{
	bool ok = super::start();

	if (!ok) return false;

	double sr;
	int ino_storage, ino_webpages;

	ok =   jCommons.get_config_key("JazzHTTPSERVER.REST_SAMPLE_PROPORTION", sr)
		&& jCommons.get_config_key("JazzHTTPSERVER.MHD_DISABLE_STORAGE", ino_storage)
		&& jCommons.get_config_key("JazzHTTPSERVER.MHD_DISABLE_WEBPAGES", ino_webpages);

	if (!ok || sr < 0 || sr > 1 || (ino_storage & 1) || (ino_webpages & 1))
	{
		jCommons.log(LOG_MISS, "jzzAPI::start() failed. Key JazzHTTPSERVER.REST_SAMPLE_PROPORTION not valid.");

		return false;
	}
	no_storage	= ino_storage;
	no_webpages = ino_webpages;

	sample_n = sr == 0 ? 0 : round(1/sr);
	sample_i = 0;

	jCommons.log(LOG_INFO, "jzzAPI started.");

	return true;
}


/** Stop of jzzAPI

	see jazzService::stop()
*/
bool jzzAPI::stop()
{
	jCommons.log(LOG_INFO, "jzzAPI stopped.");

	return super::stop();
}


/** Reload of jzzAPI

	see jazzService::reload()
*/
bool jzzAPI::reload()
{
	bool ok = super::reload();

	if (!ok) return false;

	jCommons.log(LOG_INFO, "jzzAPI reload did nothing.");

	return true;
}

/*	-----------------------------------------------
		s a m p l i n g	 method
--------------------------------------------------- */

/** Sample this API call for statistics with an enter_api_call()/leave_api_call() pair
*/
bool jzzAPI::sample_api_call()
{
	return sample_n == 0 ? false : (sample_i++ % sample_n) == 0;
}

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

	memset(&pars, 0, sizeof(parsedURLhea));
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

	pars.source = jBLOCKC.get_source_idx(source.key);
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

	if (jBLOCKC.get_source_idx("www") == 1 && jBLOCKC.block_get(1, key, block))
	{
		response = MHD_create_response_from_buffer (block->size, &reinterpret_cast<pRawBlock>(block)->data, MHD_RESPMEM_MUST_COPY);

		jBLOCKC.block_unprotect (block);
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
	URLattrib uattr;

	if (!get_url(url, uattr))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_www_get(): get_url() failed.");

		return false;
	}

	pJazzBlock pb;

	if (!jBLOCKC.block_get(1, uattr.block, pb))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_www_get(): block_get() failed.");

		return false;
	}

	response = MHD_create_response_from_buffer (pb->size, &reinterpret_cast<pRawBlock>(pb)->data, MHD_RESPMEM_MUST_COPY);

	jBLOCKC.block_unprotect(pb);

	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_STRING[uattr.blocktype]);

	if (uattr.language != LANG_DONT_CARE)
		MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_LANGUAGE, HTTP_LANGUAGE_STRING[uattr.language]);

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

	if (!jBLOCKC.block_get(pars.source, pars.key, pb))
	{
		jCommons.log(LOG_MISS, "jzzAPI::exec_block_get(): block_get() failed.");

		return MHD_HTTP_NOT_FOUND;
	}

	response = MHD_create_response_from_buffer (pb->size, &reinterpret_cast<pRawBlock>(pb)->data, MHD_RESPMEM_MUST_COPY);

	jBLOCKC.block_unprotect(pb);

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
				if (!jBLOCKC.block_get(pars.source, pars.key, pb))
				{
					jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): block_get() failed.");

					return MHD_HTTP_NOT_FOUND;
				}

				if (!strcmp(pars.function.name, "header"))
				{
					sprintf(page_2k, "type:%d\nlength:%d\nsize:%d\nflags:%d\nhash64:%lx\n",
							pb->type, pb->length, pb->size, pb->flags, (long unsigned int) pb->hash64);

					jBLOCKC.block_unprotect(pb);

					goto return_page2k;
				}

				if (!strcmp(pars.function.name, "as_text"))
				{
					if (!jBLOCKC.translate_block_TO_TEXT(pb, pb_dest, pars.parameters.param))
					{
						jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): translate_block_TO_TEXT() failed.");

						jBLOCKC.block_unprotect(pb);

						return MHD_HTTP_NOT_ACCEPTABLE;
					}

					jBLOCKC.block_unprotect(pb);

					goto return_pb_dest;
				}

				if (!strcmp(pars.function.name, "as_R"))
				{
					if (!jBLOCKC.translate_block_TO_R(pb, pb_dest))
					{
						jCommons.log(LOG_MISS, "jzzAPI::exec_block_get_function(): translate_block_TO_R() failed.");

						jBLOCKC.block_unprotect(pb);

						return MHD_HTTP_NOT_ACCEPTABLE;
					}

					jBLOCKC.block_unprotect(pb);

					goto return_pb_dest;
				}

				jBLOCKC.block_unprotect(pb);
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
	for (int i = 0; i < jBLOCKC.num_sources(); i++)
	{
		sourceName nam;
		jBLOCKC.source_name(i, nam);

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

		sprintf(page_2k, "Jazz\n\n version : %s\n build \x20 : %s\n artifact: %s\n myname\x20 : %s\n "
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
		if (!jBLOCKC.new_block_C_RAW_once(pb, upload, size))
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): new_block_C_RAW_once() failed.");

			return false;
		}
	}
	else
	{
		pJazzBlock pb2;

		if (!jBLOCKC.block_get(pars.source, pars.key, pb2))
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): block_get() failed.");

			return false;
		}

		bool ok = JAZZALLOC(pb, RAM_ALLOC_C_RAW, pb2->size + size);
		if (!ok)
		{
			jCommons.log(LOG_MISS, "jzzAPI::exec_block_put(): JAZZALLOC() failed.");

			jBLOCKC.block_unprotect (pb2);

			return false;
		}

		uint8_t * pt = (uint8_t *) &pb->data;
		memcpy(pt, &reinterpret_cast<pRawBlock>(pb2)->data, pb2->size);

		jBLOCKC.block_unprotect (pb2);

		pt += pb2->size;
		memcpy(pt, upload, size);
	}

	if (!jBLOCKC.block_put(pars.source, pars.key, pb))
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

				if (!jBLOCKC.char_to_key((const char *) &pars.parameters, websource))
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

						return jBLOCKC.cast_lmdb_block(pars.source, pars.key, type);
					}
					if (!strcmp(pars.parameters.param, "flags"))
					{
						if (size != sizeof(int))
						{
							jCommons.log(LOG_MISS, "exec_block_put_function() flags, size != sizeof(int).");

							return false;
						}

						int flags = reinterpret_cast<const int *>(upload)[0];

						return jBLOCKC.set_lmdb_blockflags(pars.source, pars.key, flags);
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
						|| !jBLOCKC.char_to_key(source_key, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() from_text, parse params failed.");

						return false;
					}

					return jBLOCKC.set_lmdb_fromtext(pars.source, pars.key, type, key, fmt);
				}
				if (!strcmp(pars.function.name, "from_R"))			// parameters: source_key		upload: -nothing-
				{
					persistedKey key;

					if(!jBLOCKC.char_to_key(pars.parameters.param, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() from_R, char_to_key() failed.");

						return false;
					}
					return jBLOCKC.set_lmdb_fromR(pars.source, pars.key, key);
				}
				if (!strcmp(pars.function.name, "C_bool_rep"))		// parameters: x,times			upload: -nothing-
				{
					unsigned int x = JAZZC_NA_BOOL, times;
					bool ok = sscanf(pars.parameters.param, "NA,%u", &times) == 1;
					if (!ok)
						ok = sscanf(pars.parameters.param, "%u,%u", &x, &times) == 2 && x < 2;
					if (!ok)
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_bool_rep, parse params failed.");

						return false;
					}
					pBoolBlock pb;
					if (!jBLOCKC.new_block_C_BOOL_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_bool_rep, new_block_C_BOOL_rep() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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
					int x = JAZZC_NA_INTEGER;
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
					if (!jBLOCKC.new_block_C_INTEGER_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_rep, new_block_C_INTEGER_rep() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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
					if (!jBLOCKC.new_block_C_INTEGER_seq(pb, from, to, by))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_integer_seq, new_block_C_INTEGER_seq() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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
					double x = JAZZC_NA_DOUBLE;
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
					if (!jBLOCKC.new_block_C_REAL_rep(pb, x, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_rep, new_block_C_REAL_rep() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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
					if (!jBLOCKC.new_block_C_REAL_seq(pb, from, to, by))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_real_seq, new_block_C_REAL_seq() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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
					if (!jBLOCKC.new_block_C_CHARS_rep(pb, pt, times))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() C_chars_rep, new_block_C_CHARS_rep() failed.");

						return false;
					}
					jBLOCKC.hash_block((pJazzBlock) pb);
					if (!jBLOCKC.block_put(pars.source, pars.key, (pJazzBlock) pb))
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

					if (!jBLOCKC.char_to_key(buff, key))
					{
						jCommons.log(LOG_MISS, "exec_block_put_function() new_source, char_to_key() failed.");

						return false;
					}

					return jBLOCKC.new_source(key.key);
				}
				break;

			case 1:

				if (!strcmp(pars.function.name, "new_websource"))
				{
					websourceName name;

					if (!jBLOCKC.char_to_key(buff, name))
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
		if (jBLOCKC.block_kill(pars.source, pars.key))
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

	if (!jBLOCKC.source_name(pars.source, name))
	{
		jCommons.log(LOG_MISS, "exec_block_kill() source_name() failed.");

		return MHD_HTTP_NOT_ACCEPTABLE;
	}

	if (jBLOCKC.kill_source(name.key))
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

//TODO: Implement exec_instr_get().

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

//TODO: Implement exec_instr_get_function().

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

//TODO: Implement exec_instr_put().

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

//TODO: Implement exec_instr_put_function().

	return false;
}


/**	 Execute a kill block using the instrumental API.

	\param	pars	 The structure containing the parts of the url successfully parsed by parse_url().

	\return			 http status and log(LOG_MISS, "further details") for errors.
*/
int jzzAPI::exec_instr_kill(parsedURL &pars)
{
	jCommons.log(LOG_MISS, "exec_instr_kill()");

//TODO: Implement exec_instr_kill().

	return MHD_HTTP_NOT_IMPLEMENTED;
}

/*	--------------------------------------------------
	  T y p e	u t i l s
--------------------------------------------------- */

/** Convert a char * into a persistedKey (internal). String accepts MAX_KEY_LENGTH-1 chars in [a-zA-Z0-9_]+
	\param pch	The source key as char *
	\param key	The destination persistedKey

	\return		true if successful, does not log.

	Unlike the same function in jzzBLOCKS (char_to_key()), this stops accepting strings finished by '.', '/' or '\0'.
*/
bool jzzAPI::char_to_key_relaxed_end (const char * pch, persistedKey &key)
{
	if (!pch[0])
		return false;

	char c;
	int i = 0;
	char * pd = (char *) &key.key;

	while (c = pch[i++])	// ... 0..9:;<=>?@A..Z[\]^_`a..z ...
	{
		if (i == MAX_KEY_LENGTH) break;

		if (c <	 '0' || c >	 'z') break;
		if (c <= '9' || c >= 'a' || c == '_')
		{
			*pd++ = c;
			continue;
		}
		if (c <	 'A' || c > 'Z') break;
		*pd++ = c;
	}

	*pd++ = 0;

	return c == 0 || c == '.' || c == '/';
}


/** Convert a char * into an instumentName (internal). String accepts MAX_INSTRUMENT_LENGTH-1 chars in [a-zA-Z0-9_]+
	\param pch	The source key as char *
	\param inst	The destination instumentName

	\return		true if successful, does not log.

	This stops accepting strings finished by '.', '/' or '\0'.
*/
bool jzzAPI::char_to_instrum_relaxed_end (const char * pch, instumentName &inst)
{
	if (!pch[0])
		return false;

	char c;
	int i = 0;
	char * pd = (char *) &inst.name;

	while (c = pch[i++])	// ... 0..9:;<=>?@A..Z[\]^_`a..z ...
	{
		if (i == MAX_INSTRUMENT_LENGTH) break;

		if (c <	 '0' || c >	 'z') break;
		if (c <= '9' || c >= 'a' || c == '_')
		{
			*pd++ = c;
			continue;
		}
		if (c <	 'A' || c > 'Z') break;
		*pd++ = c;
	}

	*pd++ = 0;

	return c == 0 || c == '.' || c == '/';
}


/** Convert a char * into an apifunctionParam (internal). String accepts MAX_PARAM_LENGTH-1 chars in [a-zA-Z0-9_,;&%\-\.\\]+
	\param pch	 The source key as char *
	\param param The destination apifunctionParam

	\return		true if successful, does not log.

	This accepts no additional ending characters.
*/
bool jzzAPI::char_to_param_strict_end (const char * pch, apifunctionParam &param)
{
	if (!pch[0])
		return false;

	int i = 0;
	char * pd = (char *) &param.param;

	while (char c = pch[i++])	// ... %&,. 0..9:;<=>?@A..Z[\]^_`a..z ...
	{
		if (i == MAX_PARAM_LENGTH || c < '%' || c > 'z') return false;
		if (c < '0')
		{
			switch (c)
			{
				case '%':
				case '&':
				case ',':
				case '.':
				case '-':
					*pd++ = c;
					break;

				default:
					return false;
			}
			continue;
		}
		if (c <= '9' || c >= 'a' || c == '_' || c == ';')
		{
			*pd++ = c;
			continue;
		}
		if (c <	 'A' || (c > 'Z' && c != '\\')) return false;
		*pd++ = c;
	}

	*pd++ = 0;

	expand_escaped(param.param);

	return true;
}

/*	--------------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST

TEST_CASE("Assertions on some jzzWebSource and API sizes")
{
	REQUIRE(sizeof(URLattrib) == 256);

	REQUIRE(MAX_NUM_WEBSOURCES*(sizeof(websourceName) + 1) < GET_FUN_BUFFER_SIZE);
	REQUIRE(MAX_POSSIBLE_SOURCES*(sizeof(persistedKey) + 1) < GET_FUN_BUFFER_SIZE);
}


TEST_CASE("Some simple type conversion utils.")
{
	jzzAPI jAPI;

	persistedKey key;

	REQUIRE(!jAPI.char_to_key_relaxed_end("", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("1234567890123456", key));
	REQUIRE( jAPI.char_to_key_relaxed_end("123456789012345", key));
	REQUIRE(!strcmp(key.key, "123456789012345"));
	REQUIRE( jAPI.char_to_key_relaxed_end("a", key));
	REQUIRE(!strcmp(key.key, "a"));
	REQUIRE( jAPI.char_to_key_relaxed_end("123456789012345.abcdefg", key));
	REQUIRE(!strcmp(key.key, "123456789012345"));
	REQUIRE( jAPI.char_to_key_relaxed_end("a", key));
	REQUIRE(!strcmp(key.key, "a"));
	REQUIRE( jAPI.char_to_key_relaxed_end("123456789012345/abcdefg", key));
	REQUIRE(!strcmp(key.key, "123456789012345"));
	REQUIRE( jAPI.char_to_key_relaxed_end("a", key));
	REQUIRE(!strcmp(key.key, "a"));
	REQUIRE(!jAPI.char_to_key_relaxed_end("123456789012345-abcdefg", key));
	REQUIRE( jAPI.char_to_key_relaxed_end("1aA_2bB_9cC_0zZ", key));
	REQUIRE(!strcmp(key.key, "1aA_2bB_9cC_0zZ"));

	REQUIRE(!jAPI.char_to_key_relaxed_end("a-a", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a:a", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a@a", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a[a", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a`a", key));

	REQUIRE( jAPI.char_to_key_relaxed_end("a_B9.123456789012345", key));
	REQUIRE(!strcmp(key.key, "a_B9"));
	REQUIRE( jAPI.char_to_key_relaxed_end("a_B9/123456789012345", key));
	REQUIRE(!strcmp(key.key, "a_B9"));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a_B9-123456789012345", key));

	REQUIRE(!jAPI.char_to_key_relaxed_end("a_B9*12345", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a_B9,12345", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a_B9 12345", key));
	REQUIRE(!jAPI.char_to_key_relaxed_end("a_B9;12345", key));

	instumentName inst;

	REQUIRE(!jAPI.char_to_instrum_relaxed_end("", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("12345678901234567890123456789012", inst));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("1234567890123456789012345678901", inst));
	REQUIRE(!strcmp(inst.name, "1234567890123456789012345678901"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("a", inst));
	REQUIRE(!strcmp(inst.name, "a"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("1234567890123456789012345678901.abcdefg", inst));
	REQUIRE(!strcmp(inst.name, "1234567890123456789012345678901"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("a", inst));
	REQUIRE(!strcmp(inst.name, "a"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("1234567890123456789012345678901/abcdefg", inst));
	REQUIRE(!strcmp(inst.name, "1234567890123456789012345678901"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("a", inst));
	REQUIRE(!strcmp(inst.name, "a"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("1aA_2bB_9cC_0zZ_789012345678901", inst));
	REQUIRE(!strcmp(inst.name, "1aA_2bB_9cC_0zZ_789012345678901"));

	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a-a", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a:a", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a@a", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a[a", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a`a", inst));

	REQUIRE( jAPI.char_to_instrum_relaxed_end("a_B9.123456789012345", inst));
	REQUIRE(!strcmp(inst.name, "a_B9"));
	REQUIRE( jAPI.char_to_instrum_relaxed_end("a_B9/123456789012345", inst));
	REQUIRE(!strcmp(inst.name, "a_B9"));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a_B9-123456789012345", inst));

	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a_B9*12345", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a_B9,12345", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a_B9 12345", inst));
	REQUIRE(!jAPI.char_to_instrum_relaxed_end("a_B9;12345", inst));

	apifunctionParam param;

	REQUIRE(!jAPI.char_to_param_strict_end("", param));
	REQUIRE(!jAPI.char_to_param_strict_end("1234567890123456789012345678901234567890123456789012345678901234", param));
	REQUIRE( jAPI.char_to_param_strict_end("123456789012345678901234567890123456789012345678901234567890123", param));
	REQUIRE(!strcmp(param.param, "123456789012345678901234567890123456789012345678901234567890123"));
	REQUIRE( jAPI.char_to_param_strict_end("a", param));
	REQUIRE(!strcmp(param.param, "a"));
	REQUIRE(!jAPI.char_to_param_strict_end("123456789012345678901234567890123456789012345678901234567890123.abcdefg", param));
	REQUIRE(!jAPI.char_to_param_strict_end("123456789012345678901234567890123456789012345678901234567890123/abcdefg", param));
	REQUIRE(!jAPI.char_to_param_strict_end("123456789012345678901234567890123456789012345678901234567890123-abcdefg", param));
	REQUIRE( jAPI.char_to_param_strict_end("1aA_2bB_9cC_.,;&-8901234567890123456789012345678901234567890123", param));
	REQUIRE(!strcmp(param.param, "1aA_2bB_9cC_.,;&-8901234567890123456789012345678901234567890123"));

	REQUIRE( jAPI.char_to_param_strict_end("a-a", param));
	REQUIRE(!strcmp(param.param, "a-a"));
	REQUIRE( jAPI.char_to_param_strict_end("b.a", param));
	REQUIRE(!strcmp(param.param, "b.a"));
	REQUIRE( jAPI.char_to_param_strict_end("c,a", param));
	REQUIRE(!strcmp(param.param, "c,a"));
	REQUIRE( jAPI.char_to_param_strict_end("d;a", param));
	REQUIRE(!strcmp(param.param, "d;a"));
	REQUIRE( jAPI.char_to_param_strict_end("e&a", param));
	REQUIRE(!strcmp(param.param, "e&a"));
	REQUIRE( jAPI.char_to_param_strict_end("f%a", param));
	REQUIRE(!strcmp(param.param, "f%a"));
	REQUIRE( jAPI.char_to_param_strict_end("g\\a", param));
	REQUIRE(!strcmp(param.param, "g\a"));

	REQUIRE(!jAPI.char_to_param_strict_end("a a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a!a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a$a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a'a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a+a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a/a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a:a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a<a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a@a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a[a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a]a", param));
	REQUIRE(!jAPI.char_to_param_strict_end("a`a", param));

	REQUIRE( jAPI.char_to_param_strict_end("a_B9.123456789012345", param));
	REQUIRE(!strcmp(param.param, "a_B9.123456789012345"));
	REQUIRE(!jAPI.char_to_param_strict_end("a_B9/123456789012345", param));
	REQUIRE( jAPI.char_to_param_strict_end("a_B9-123456789012345", param));
	REQUIRE(!strcmp(param.param, "a_B9-123456789012345"));

	REQUIRE(!jAPI.char_to_param_strict_end("a_B9*12345", param));
	REQUIRE( jAPI.char_to_param_strict_end("a_B9,12345", param));
	REQUIRE(!strcmp(param.param, "a_B9,12345"));
	REQUIRE(!jAPI.char_to_param_strict_end("a_B9 12345", param));
	REQUIRE( jAPI.char_to_param_strict_end("a_B9;12345", param));
	REQUIRE(!strcmp(param.param, "a_B9;12345"));
}


SCENARIO("Scenario jzzAPI ...")
{
	jCommons.load_config_file("./serverconf/jazz_config.ini");

	REQUIRE(jBLOCKC.start());

	jzzAPI jAPI;

	jCommons.debug_config_put("JazzHTTPSERVER.REST_SAMPLE_PROPORTION", "1");

	REQUIRE(jAPI.start());

	for (int i = 0; i < 50; i++)
	{
		REQUIRE(jAPI.sample_api_call());
	}

	REQUIRE(jAPI.stop());

	jCommons.debug_config_put("JazzHTTPSERVER.REST_SAMPLE_PROPORTION", "0");

	REQUIRE(jAPI.start());

	for (int i = 0; i < 50; i++)
	{
		REQUIRE(!jAPI.sample_api_call());
	}

	REQUIRE(jAPI.stop());

	jCommons.debug_config_put("JazzHTTPSERVER.REST_SAMPLE_PROPORTION", "0.25");

	REQUIRE(jAPI.start());

	for (int i = 0; i < 50; i++)
	{
		REQUIRE( jAPI.sample_api_call());
		REQUIRE(!jAPI.sample_api_call());
		REQUIRE(!jAPI.sample_api_call());
		REQUIRE(!jAPI.sample_api_call());
	}

	GIVEN("A working persistence")
	{
		REQUIRE(jBLOCKC.num_sources() >= 2);

		REQUIRE(jBLOCKC.get_source_idx("sys") == 0);
		REQUIRE(jBLOCKC.get_source_idx("www") == 1);

		WHEN("I parse some valid urls ...")
		{
			for (int t = 0; t < 2; t++)
			{
				parsedURL pars;

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 0);
					REQUIRE(!strcmp(pars.function.name, "ls"));
					REQUIRE(pars.parameters.param[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("///", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 0);
					REQUIRE(!strcmp(pars.function.name, "server_vers"));
					REQUIRE(!strcmp(pars.parameters.param, "full"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www//My_func4", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(pars.parameters.param[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",	HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X.my_Fun", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(pars.parameters.param[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www//My_func4", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(pars.parameters.param[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",	HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X.my_Fun", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(pars.parameters.param[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "kY666_X"));
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www.my_blk", HTTP_DELETE, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE( pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source == 1);
					REQUIRE(!strcmp(pars.key.key, "my_blk"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www", HTTP_DELETE, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE(!pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE( pars.deleteSource);

					REQUIRE(pars.source == 1);
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "ls"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(pars.mesh.key[0] == 0);
					REQUIRE(pars.instrument.name[0]	 == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "ls"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0]	 == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92.My_func4", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0]	 == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enable-gps", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0]	 == 0);
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.my_Fun", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/1,2,,0,5&a;enable_gps", HTTP_GET, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0,5&a;enable_gps"));
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92.My_func4", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0]	 == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enable-gps", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0.5&a;enable-gps"));
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0]	 == 0);
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.my_Fun", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "my_Fun"));
					REQUIRE(pars.parameters.param[0] == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/1,2,,0,5&a;enable_gps", HTTP_PUT, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE( pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.function.name, "My_func4"));
					REQUIRE(!strcmp(pars.parameters.param, "1,2,,0,5&a;enable_gps"));
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}

				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha", HTTP_DELETE, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(!strcmp(pars.instrument.name, "instrument_0123456789_alpha"));
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//www/Mesh_Z92", HTTP_DELETE, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 1);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0] == 0);
				}
				memset(&pars, 33*t, sizeof(pars));
				REQUIRE(jAPI.parse_url("//sys/Mesh_Z92", HTTP_DELETE, pars));
				THEN("... and check the resulting record.")
				{
					REQUIRE( pars.isInstrumental);
					REQUIRE(!pars.hasAKey);
					REQUIRE(!pars.hasAFunction);
					REQUIRE(!pars.deleteSource);

					REQUIRE(pars.source	 == 0);
					REQUIRE(!strcmp(pars.mesh.key, "Mesh_Z92"));
					REQUIRE(pars.instrument.name[0] == 0);
				}
			}
		}

		WHEN("I parse some invalid urls ...")
		{
			parsedURL pars;

			REQUIRE(!jAPI.parse_url("//.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("///.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//ww//My_func4", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//wwww.71436136218254", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable:gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X1234567890", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun.2", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun//1,2,,0.5&a;enable-gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4.", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/My_func4/1,2,,0.5&a;enable-gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X$", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun.aaa", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps/a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//wow.my_blk", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www&", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www//", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.?", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_fun c4/1,2,,0.5&a;enable-gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha_too_too_too", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha..my_Fun", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/1,2,,0,5&a$enable_gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92//My_func4", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4.1,2,,0.5&a;enable-gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92//instrument_0123456789_alpha", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.my_Fun/", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/?1,2,,0,5&a;enable_gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92%", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//sys/Mesh_Z92.", HTTP_DELETE, pars));
		}

		WHEN("I parse all urls with trailing junk ...")
		{
			parsedURL pars;

			REQUIRE(!jAPI.parse_url("// ", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("////", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps/",	HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps ", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4.a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps/a",	HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X/a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun.a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps/a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.my_blk.a", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/&a", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www//", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4.a", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enable-gps/a", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.my_Fun.a", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/1,2,,0,5&a;enable_gps/b", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4//", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enable-gps/a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha/", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.my_Fun.a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.My_func4/1,2,,0,5&a;enable_gps/a", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92//", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//sys/Mesh_Z92//a", HTTP_DELETE, pars));
		}

		WHEN("I parse all urls with wrong characters ...")
		{
			parsedURL pars;

			REQUIRE(!jAPI.parse_url("//%", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("///%", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//w%w//My_func4", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_f%nc4/1,2,,0.5&a;enable-gps",	HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY66?_X", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_F!un", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps!", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func&4", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www//My_fun*c4/1,2,,0.5&a;enable-gps",	HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666^X", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666?X.my_Fun", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.m%y_Fun/1,2,,0.5&a;enable-gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www.my_blk=", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www=", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www=/", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//w!w/Mesh_Z92/", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Me$h_Z92.My_func4", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_&unc4/1,2,,0.5&a;enable-gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/ins%rument_0123456789_alpha", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_!92/instrument_0123456789_alpha.my_Fun", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instr?ument_0123456789_alpha.My_func4/1,2,,0,5&a;enable_gps", HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_fun(c4", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;en)able-gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_a{lpha", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0}123456789_alpha.my_Fun", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_01[23456789_alpha.My_func4/1,2,,0,5&a;enable_gps", HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123]456789_alpha", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh<_Z92", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//sys/Mes>h_Z92", HTTP_DELETE, pars));
		}

		WHEN("I parse all urls with wrong methods ...")
		{
			parsedURL pars;

			REQUIRE( jAPI.parse_url("//",												HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("///",												HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4",									HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",			HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X",									HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun",								HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps",		HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www",											HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/",											HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/",									HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4",							HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enablegps",		HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.myFun", HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_012pha.My_func4/12;gps", HTTP_GET, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha",		HTTP_GET, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92",									HTTP_GET, pars));

			REQUIRE(!jAPI.parse_url("//",												HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("///",												HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4",									HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",			HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X",									HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun",								HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps",		HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www",											HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/",											HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/",									HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4",							HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enablegps",		HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.myFun", HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_012pha.My_func4/12;gps", HTTP_PUT, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha",		HTTP_PUT, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92",									HTTP_PUT, pars));

			REQUIRE(!jAPI.parse_url("//",												HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("///",												HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4",									HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",			HTTP_DELETE, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X",									HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun",								HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps",		HTTP_DELETE, pars));
			REQUIRE( jAPI.parse_url("//www",											HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/",											HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/",									HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4",							HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enablegps",		HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.myFun", HTTP_DELETE, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_012pha.My_func4/12;gps", HTTP_DELETE, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha",		HTTP_DELETE, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92",									HTTP_DELETE, pars));

			REQUIRE( jAPI.parse_url("//",												HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("///",												HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4",									HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",			HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X",									HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun",								HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps",		HTTP_HEAD, pars));
			REQUIRE(!jAPI.parse_url("//www",											HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/",											HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/",									HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4",							HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enablegps",		HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.myFun", HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_012pha.My_func4/12;gps", HTTP_HEAD, pars));
			REQUIRE( jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha",		HTTP_HEAD, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92",									HTTP_HEAD, pars));

			REQUIRE(!jAPI.parse_url("//",												HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("///",												HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4",									HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www//My_func4/1,2,,0.5&a;enable-gps",			HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X",									HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun",								HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www.kY666_X.my_Fun/1,2,,0.5&a;enable-gps",		HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www",											HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/",											HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/",									HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4",							HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92.My_func4/1,2,,0.5&a;enablegps",		HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha.myFun", HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_012pha.My_func4/12;gps", HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92/instrument_0123456789_alpha",		HTTP_NOTUSED, pars));
			REQUIRE(!jAPI.parse_url("//www/Mesh_Z92",									HTTP_NOTUSED, pars));
		}
	}

	REQUIRE(jAPI.stop());
	REQUIRE(jBLOCKC.stop());
}
#endif

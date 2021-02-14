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


#include "src/jazz_main/api.h"


namespace jazz_main
{

/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

/*

On cookies:
-----------

1. The idea is explained in https://developer.mozilla.org/en-US/docs/Web/HTTP/Cookies

	The server, has to check if the request comes with a cookie, if not, it has to send:

	Set-Cookie: jazz_user=a3fWa; Expires=Thu, 31 Oct 2021 07:28:00 GMT;

	After that, all requests will come with:

	GET /sample_page.html HTTP/2.0
	Host: www.example.org
	Cookie: jazz_user=a3fWa;

	The server uses the cookie to keep the conversation context persisted.

2. The use via MHD is explained in https://www.gnu.org/software/libmicrohttpd/tutorial.html#Session-management

3. The disclaimer has been added to config/static/cookies.htm

*/


#define MHD_HTTP_ANYERROR 400

#ifdef DEBUG
int print_out_key (void *cls, enum MHD_ValueKind kind,
				   const char *key, const char *value)
{
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);
	return MHD_YES;
}
#endif


/// Pointers to these global variables control the state of a PUT call.
int	state_new_call				= 0;	///< Default state: connection open for any call
int	state_upload_in_progress	= 1;	///< Data was uploaded, the function was executed and it returned true.
int	state_upload_notacceptable	= 2;	///< Data upload failed, the function executed and failed. Will return MHD_HTTP_NOT_ACCEPTABLE
int	state_upload_unavailable	= 3;	///< Data upload failed, enter_persistence() failed. Must end with MHD_HTTP_SERVICE_UNAVAILABLE.
int	state_upload_badrequest		= 4;	///< PUT call malformed, Must end with MHD_HTTP_BAD_REQUEST.

char response_put_ok[]			= "0";
char response_put_fail[]		= "1";

bool no_storage, no_webpages;

int tenbitDS;							///< The binary ten bits of "//" double-slash to identify web source interface.


/** Callback function for MHD. See: https://www.gnu.org/software/libmicrohttpd/tutorial.html

	Jazz does not use post processor callbacks linked with MHD_create_post_processor().
	Jazz does not use request completed callbacks linked with MHD_start_daemon().

	The only callback functions are:

		1. This (http_request_callback): The full operational blocks and instrumental API.
		2. http_apc_callback():			 An IP based (firewall like) that can filter based on called IPs.

	This function is multithreaded with the default configuration settings. (Other settings are untested for the moment.)

	The internal operation of the callback function is subject to change and the remarks in the source code are the description of it.
*/
int http_request_callback(void *cls,
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
/*
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
*/
#ifdef DEBUG
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - cls \x20 \x20 \x20 \x20 \x20 \x20 \x20: %p", cls);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - connection \x20 \x20 \x20 : %p", connection);
	MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - url \x20 \x20 \x20 \x20 \x20 \x20 \x20: %s", url);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - method \x20 \x20 \x20 \x20 \x20 : %s", method);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - version \x20 \x20 \x20 \x20 \x20: %s", version);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data \x20 \x20 \x20: %p", upload_data);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data_size : %d", *upload_data_size);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - *con_cls \x20 \x20 \x20 \x20 : %p", *con_cls);
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");

//	response = NULL;	// clang warning
#endif
/*
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
*/
	return MHD_NO;
}

/*	-----------------------------------------------
	 Api : I m p l e m e n t a t i o n
--------------------------------------------------- */

Api::Api(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}

/**
//TODO: Document Api::start()
*/
Service_ErrorCode Api::start()
{
//TODO: Implement Api::start()

	return SERVICE_NO_ERROR;
}

/**
//TODO: Document Api::shut_down()
*/
Service_ErrorCode Api::shut_down(bool restarting_service)
{
//TODO: Implement Api::shut_down()

	return SERVICE_NO_ERROR;
}

} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

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

#define MHD_HTTP_ANYERROR 400

#ifdef DEBUG
int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);

	return MHD_YES;
}
#endif


/// Pointers to these global variables control the state of a PUT call.
int	state_new_call				= 0;	///< Default state: connection open for any call
int	state_upload_in_progress	= 1;	///< Data was uploaded, the query executed successfully.
int	state_upload_notacceptable	= 2;	///< Data upload failed, query execution failed locating tagets. Returns MHD_HTTP_NOT_ACCEPTABLE
int	state_upload_badrequest		= 3;	///< PUT query is call malformed. Returns MHD_HTTP_BAD_REQUEST.

char response_put_ok[]			= "0";
char response_put_fail[]		= "1";

int tenbit_double_slash;				///< The binary ten bits of "//" double-slash to identify web source interface.
TenBitsLUT http_methods;				///< A LUT to convert argument const char *method int an integer code.


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

	if (*con_cls == NULL) {
		*con_cls = &state_new_call;

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int http_method = http_methods[TenBitsAtAddress(method)];

	APIParseBuffer parse_buffer;
	parse_buffer.stack_size = 0;

	struct MHD_Response *response = nullptr;
	pBlockKeeper p_response_block_keeper = nullptr;

	if (*con_cls == &state_upload_in_progress) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_ok;

		if (http_method != HTTP_PUT || API.parse(url, HTTP_PUT, parse_buffer) != PARSE_OK) {
			LOGGER.log(LOG_MISS, "http_request_callback(): Trying to continue state_upload_in_progress, but API.parse() failed.");

			return MHD_NO;
		}

		if (API.upload(parse_buffer, upload_data, *upload_data_size, true))
			goto continue_in_put_ok;

		goto continue_in_put_notacceptable;
	}

	// Step 3 : Get rid of failed uploads without doing anything.

	if (*con_cls == &state_upload_notacceptable) {
		if (*upload_data_size == 0)
			goto create_response_answer_PUT_NOTACCEPTABLE;

		return MHD_YES;
	}

	if (*con_cls == &state_upload_badrequest) {
		if (*upload_data_size == 0)
			goto create_response_answer_PUT_BADREQUEST;

		return MHD_YES;
	}

	// Step 4 : This point is reached just once per http petition. Parse the query, returns errors and web pages, continue to API.

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
#endif

	switch (http_method) {
	case HTTP_NOTUSED:
		return API.return_error_message(connection, MHD_HTTP_METHOD_NOT_ALLOWED);

	case HTTP_OPTIONS:
		{ // Tricky: Opens a scope to make "std::string allow" out of scope in the rest to support the goto logic.
			std::string allow;
			if (TenBitsAtAddress(url) != tenbit_double_slash) {
				pBlockKeeper p_not_executed;

				if (API.get_static(url, &p_not_executed, false) == GET_OK)
					allow = "HEAD,GET,";

				allow = allow + "OPTIONS";
			} else {
				if (API.parse(url, HTTP_GET, parse_buffer, false) == PARSE_OK)
					allow = "HEAD,GET,";
				if (API.parse(url, HTTP_PUT, parse_buffer, false) == PARSE_OK)
					allow = allow + "PUT,";
				if (API.parse(url, HTTP_DELETE, parse_buffer, false) == PARSE_OK)
					allow = allow + "DELETE,";

				allow = allow + "OPTIONS";
			}

			response = MHD_create_response_from_buffer (1, response_put_ok, MHD_RESPMEM_PERSISTENT);

			MHD_add_response_header (response, MHD_HTTP_HEADER_SERVER, "Jazz " JAZZ_VERSION " - " LINUX_PLATFORM);
			MHD_add_response_header (response, MHD_HTTP_HEADER_ALLOW, allow.c_str());
		} // Here std::string allow is out of scope.

		goto answer_no_content;

	case HTTP_HEAD:
	case HTTP_GET:
		if (TenBitsAtAddress(url) != tenbit_double_slash) {

			if (API.get_static(url, &p_response_block_keeper) == GET_OK)
				goto answer_http_ok;

			return API.return_error_message(connection, MHD_HTTP_NOT_FOUND);
		} else {
			if (API.parse(url, HTTP_GET, parse_buffer) != PARSE_OK)
				return API.return_error_message(connection, MHD_HTTP_BAD_REQUEST);
		}
		break;

	default:
		if (TenBitsAtAddress(url) != tenbit_double_slash) {
			return API.return_error_message(connection, MHD_HTTP_METHOD_NOT_ALLOWED);
		} else {
			if (API.parse(url, http_method, parse_buffer) != PARSE_OK) {
				if (http_method == HTTP_PUT)
					goto continue_in_put_badrequest;

				return API.return_error_message(connection, MHD_HTTP_BAD_REQUEST);
			}
		}
	}

	// Step 5 : This is the core. This point is only reached by correct API queries for the first (or only) time.

	bool status;

	switch (http_method) {
	case HTTP_PUT:
		status = API.upload(parse_buffer, upload_data, *upload_data_size, false);

		break;

	case HTTP_DELETE:
		status = API.remove(parse_buffer);

		break;

	default:

		status = API.http_get(parse_buffer, response);
	}

	// Step 6 : The core finished, just distribute the answer as appropriate.

	if (http_method == HTTP_PUT) {
		if (status) {
			if (*upload_data_size) goto continue_in_put_ok;
			else				   goto create_response_answer_put_ok;
		} else {
			if (*upload_data_size) goto continue_in_put_notacceptable;
			else				   goto create_response_answer_PUT_NOTACCEPTABLE;
		}
	}

	if (status >= MHD_HTTP_ANYERROR)
		return API.return_error_message(connection, status);

	if (http_method == HTTP_DELETE)
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


create_response_answer_PUT_BADREQUEST:

	response = MHD_create_response_from_buffer (1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);

	MHD_destroy_response (response);

	return ret;


continue_in_put_notacceptable:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &state_upload_notacceptable;

		return MHD_YES;
	}
	return MHD_NO;


continue_in_put_badrequest:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &state_upload_badrequest;

		return MHD_YES;
	}
	return MHD_NO;


continue_in_put_ok:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &state_upload_in_progress;

		return MHD_YES;
	}
	return MHD_NO;
}

/*	-----------------------------------------------
	 Api : I m p l e m e n t a t i o n
--------------------------------------------------- */

Api::Api(pLogger	 a_logger,
		 pConfigFile a_config,
		 pVolatile	 a_volatile,
		 pRemote	 a_remote,
		 pPersisted	 a_persisted,
		 pCluster	 a_cluster,
		 pBebop		 a_bebop) : Container(a_logger, a_config) {

	for (int i = 0; i < 1024; i++) http_methods[i] = HTTP_NOTUSED;

	http_methods[TenBitsAtAddress("OPTIONS")] = HTTP_OPTIONS;
	http_methods[TenBitsAtAddress("HEAD")]	  = HTTP_HEAD;
	http_methods[TenBitsAtAddress("GET")]	  = HTTP_GET;
	http_methods[TenBitsAtAddress("PUT")]	  = HTTP_PUT;
	http_methods[TenBitsAtAddress("DELETE")]  = HTTP_DELETE;

	tenbit_double_slash = TenBitsAtAddress("//");

	p_volatile	= a_volatile;
	p_remote	= a_remote;
	p_persisted	= a_persisted;
	p_cluster	= a_cluster;
	p_bebop		= a_bebop;

	base = {};
}


/**
//TODO: Document Api::start()
*/
StatusCode Api::start ()
{
//TODO: Implement Api::start()

	return SERVICE_NO_ERROR;
}


/**
//TODO: Document Api::shut_down()
*/
StatusCode Api::shut_down (bool restarting_service)
{
//TODO: Implement Api::shut_down()

	return SERVICE_NO_ERROR;
}


/**
//TODO: Document Api::parse()

Parse:

// https://en.wikipedia.org/wiki/Percent-encoding

*/
StatusCode Api::parse (const char * url, int method, APIParseBuffer &pars, bool no_execution)
{
//TODO: Implement Api::parse()


	return SERVICE_NOT_IMPLEMENTED;
}


/**
//TODO: Document Api::get_static()
*/
StatusCode Api::get_static (const char *url, pBlockKeeper *p_keeper, bool execution)
{
//TODO: Implement Api::get_static()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Finish a query by delivering the appropriate message page.

	\param connection  The MHD connection passed to the callback function. (Needed for MHD_queue_response()ing the response.)
	\param http_status The http status error (e.g., MHD_HTTP_NOT_FOUND)

	\return			   A valid answer for an MHD callback. It is an integer generated by MHD_queue_response() and returned by the callback.

	This function searches for a persistence block named ("www", "httpERR_%d") where %d is the code in decimal and serves it as an answer.
*/
int	Api::return_error_message (struct MHD_Connection * connection, int http_status)
{
	Answer answer;

	sprintf(answer.text, "<html><body><h1><br/><br/>Http error : %d.</h1></body></html>", http_status);

	struct MHD_Response *response = MHD_create_response_from_buffer (strlen(answer.text), answer.text, MHD_RESPMEM_MUST_COPY);

	int ret = MHD_queue_response (connection, http_status, response);

	MHD_destroy_response (response);

	return ret;
}


/**
//TODO: Document Api::upload()
*/
bool Api::upload (APIParseBuffer &pars, const char * upload, size_t size, bool continue_upload)
{
//TODO: Implement Api::upload()

	return false;
}


/**
//TODO: Document Api::remove()
*/
bool Api::remove (APIParseBuffer &parse_buff)
{
//TODO: Implement Api::remove()

	return false;
}


/**
//TODO: Document Api::http_get()
*/
bool Api::http_get (APIParseBuffer &parse_buff, pMHD_Response &response)
{
//TODO: Implement Api::http_get()

	return false;
}


/**
//TODO: Document Api::get()
*/
StatusCode Api::get(pBlockKeeper *p_keeper,
					pR_value	  p_rvalue,
					pContainer	  p_sender,
					BlockId64	  block_id)
{
//TODO: Implement Api::get()

	return false;
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Api::base_names (BaseNames &base_names)
{
	base_names[""] = this;
}


} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

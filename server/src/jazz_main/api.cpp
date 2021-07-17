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


#include "src/jazz_main/api.h"


namespace jazz_main
{

/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

#define MHD_HTTP_ANYERROR 400

#ifdef DEBUG
MHD_Result print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
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
MHD_Result http_request_callback(void *cls,
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

	if (*con_cls == &state_upload_in_progress) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_ok;

		if (http_method != HTTP_PUT || API.parse(url, HTTP_PUT, parse_buffer, false) != PARSE_OK) {
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
				if (API.get_static(url, response, false) == GET_OK)
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

			if (API.get_static(url, response) == GET_OK)
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

	MHD_Result ret;

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
	 Parser grammar definition
--------------------------------------------------- */

#define REX_ALL_SPACES		"[ \\t]"
#define REX_COLON			"[:]"
#define REX_COMMA			"[,]"
#define REX_DOUBLEQUOTE		"[\"]"
#define REX_HEX_CHAR		"[0-9A-Fa-f]"
#define REX_LEFT_SQUARE_BR	"[\\[]"
#define REX_NAME_ANY		"[A-Za-z0-9_]"
#define REX_NAME_FIRST		"[A-Za-z]"
#define REX_NON_URLENCODED	"[ \tA-Za-z0-9_~!#$&',/:;=@\\[\\]\\?\\(\\)\\*\\+\\-\\.]"
#define REX_PERCENT			"[%]"
#define REX_RIGHT_SQUARE_BR	"[\\]]"
#define REX_SEMICOLON		"[;]"
#define REX_ZERO			"[\\0]"


StateTransitions state_tr = {
	{PSTATE_INITIAL,		PSTATE_INITIAL,			REX_ALL_SPACES},
	{PSTATE_INITIAL,		PSTATE_CONST_IN_UNK,	REX_LEFT_SQUARE_BR},
	{PSTATE_INITIAL,		PSTATE_CONST_INT,		"[0-9\\-]"},
	{PSTATE_INITIAL,		PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},
	{PSTATE_INITIAL,		PSTATE_BASE_NAME,		REX_NAME_FIRST},

	{PSTATE_CONST_IN_UNK,	PSTATE_CONST_IN_UNK,	"[ \\t\\[]"},
	{PSTATE_CONST_IN_UNK,	PSTATE_CONST_INT,		"[0-9\\-]"},
	{PSTATE_CONST_IN_UNK,	PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},

	{PSTATE_CONST_INT,		PSTATE_CONST_INT,		"[0-9]"},
	{PSTATE_CONST_INT,		PSTATE_CONST_SEP_INT,	REX_COMMA},
	{PSTATE_CONST_INT,		PSTATE_CONST_IN_INT,	REX_LEFT_SQUARE_BR},
	{PSTATE_CONST_INT,		PSTATE_CONST_OUT_INT,	REX_RIGHT_SQUARE_BR},
	{PSTATE_CONST_INT,		PSTATE_CONST_END_INT,	REX_ZERO},
	{PSTATE_CONST_INT,		PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},
	{PSTATE_CONST_INT,		PSTATE_CONST_REAL,		"[e\\.]"},

	{PSTATE_CONST_SEP_INT,	PSTATE_CONST_SEP_INT,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_INT,	PSTATE_CONST_INT,		"[0-9\\-]"},
	{PSTATE_CONST_SEP_INT,	PSTATE_CONST_IN_INT,	REX_LEFT_SQUARE_BR},

	{PSTATE_CONST_IN_INT,	PSTATE_CONST_IN_INT,	"[ \\t\\[]"},
	{PSTATE_CONST_IN_INT,	PSTATE_CONST_INT,		"[0-9\\-]"},

	{PSTATE_CONST_OUT_INT,	PSTATE_CONST_OUT_INT,	"[ \\t\\]]"},
	{PSTATE_CONST_OUT_INT,	PSTATE_CONST_INT,		"[0-9\\-]"},
	{PSTATE_CONST_OUT_INT,	PSTATE_CONST_SEP_INT,	REX_COMMA},
	{PSTATE_CONST_OUT_INT,	PSTATE_CONST_END_INT,	REX_ZERO},
	{PSTATE_CONST_OUT_INT,	PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},

	{PSTATE_CONST_REAL,		PSTATE_CONST_REAL,		"[e0-9\\-\\.]"},
	{PSTATE_CONST_REAL,		PSTATE_CONST_SEP_REAL,	REX_COMMA},
	{PSTATE_CONST_REAL,		PSTATE_CONST_IN_REAL,	REX_LEFT_SQUARE_BR},
	{PSTATE_CONST_REAL,		PSTATE_CONST_OUT_REAL,	REX_RIGHT_SQUARE_BR},
	{PSTATE_CONST_REAL,		PSTATE_CONST_END_REAL,	REX_ZERO},
	{PSTATE_CONST_REAL,		PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},

	{PSTATE_CONST_SEP_REAL,	PSTATE_CONST_SEP_REAL,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_REAL,	PSTATE_CONST_REAL,		"[0-9\\-]"},
	{PSTATE_CONST_SEP_REAL,	PSTATE_CONST_IN_REAL,	REX_LEFT_SQUARE_BR},

	{PSTATE_CONST_IN_REAL,	PSTATE_CONST_IN_REAL,	"[ \\t\\[]"},
	{PSTATE_CONST_IN_REAL,	PSTATE_CONST_REAL,		"[0-9\\-]"},

	{PSTATE_CONST_OUT_REAL,	PSTATE_CONST_OUT_REAL,	"[ \\t\\]]"},
	{PSTATE_CONST_OUT_REAL,	PSTATE_CONST_REAL,		"[0-9\\-]"},
	{PSTATE_CONST_OUT_REAL,	PSTATE_CONST_SEP_REAL,	REX_COMMA},
	{PSTATE_CONST_OUT_REAL,	PSTATE_CONST_END_REAL,	REX_ZERO},
	{PSTATE_CONST_OUT_REAL,	PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},

	{PSTATE_CONST_STR0,		PSTATE_CONST_STR,		REX_NON_URLENCODED},
	{PSTATE_CONST_STR0,		PSTATE_CONST_SEP_STR0,	REX_DOUBLEQUOTE},
	{PSTATE_CONST_STR0,		PSTATE_CONST_STR_ENC0,	REX_PERCENT},

	{PSTATE_CONST_STR,		PSTATE_CONST_STR,		REX_NON_URLENCODED},
	{PSTATE_CONST_STR,		PSTATE_CONST_SEP_STR0,	REX_DOUBLEQUOTE},
	{PSTATE_CONST_STR,		PSTATE_CONST_STR_ENC0,	REX_PERCENT},

	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_SEP_STR0,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_SEP_STR,	REX_COMMA},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_IN_STR,	REX_LEFT_SQUARE_BR},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_OUT_STR,	REX_RIGHT_SQUARE_BR},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_END_STR,	REX_ZERO},
	{PSTATE_CONST_SEP_STR0,	PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},

	{PSTATE_CONST_SEP_STR,	PSTATE_CONST_SEP_STR,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_STR,	PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},
	{PSTATE_CONST_SEP_STR,	PSTATE_CONST_IN_STR,	REX_LEFT_SQUARE_BR},

	{PSTATE_CONST_IN_STR,	PSTATE_CONST_IN_STR,	"[ \\t\\[]"},
	{PSTATE_CONST_IN_STR,	PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},

	{PSTATE_CONST_OUT_STR,	PSTATE_CONST_OUT_STR,	"[ \\t\\]]"},
	{PSTATE_CONST_OUT_STR,	PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},
	{PSTATE_CONST_OUT_STR,	PSTATE_CONST_SEP_STR,	REX_COMMA},
	{PSTATE_CONST_OUT_STR,	PSTATE_CONST_END_STR,	REX_ZERO},
	{PSTATE_CONST_OUT_STR,	PSTATE_TUPL_SEMICOLON,	REX_SEMICOLON},

	{PSTATE_CONST_STR_ENC0,	PSTATE_CONST_STR_ENC1,	REX_HEX_CHAR},

	{PSTATE_CONST_STR_ENC1,	PSTATE_CONST_STR_ENC2,	REX_HEX_CHAR},

	{PSTATE_CONST_STR_ENC2,	PSTATE_CONST_STR_ENC0,	REX_PERCENT},
	{PSTATE_CONST_STR_ENC2,	PSTATE_CONST_STR,		REX_NON_URLENCODED},
	{PSTATE_CONST_STR_ENC2,	PSTATE_CONST_SEP_STR0,	REX_DOUBLEQUOTE},

	{PSTATE_BASE_NAME,		PSTATE_BASE_NAME,		REX_NAME_ANY},
	{PSTATE_BASE_NAME,		PSTATE_TUPL_COLON,		REX_COLON},

	{PSTATE_TUPL_COLON,		PSTATE_TUPL_COLON,		REX_ALL_SPACES},
	{PSTATE_TUPL_COLON,		PSTATE_KIND_COLON0,		REX_COLON},
	{PSTATE_TUPL_COLON,		PSTATE_CONST_IN_UNK,	REX_LEFT_SQUARE_BR},
	{PSTATE_TUPL_COLON,		PSTATE_CONST_INT,		"[0-9\\-]"},
	{PSTATE_TUPL_COLON,		PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},

	{PSTATE_TUPL_SEMICOLON,	PSTATE_TUPL_SEMICOLON,	REX_ALL_SPACES},
	{PSTATE_TUPL_SEMICOLON,	PSTATE_TUPL_ITEM_NAME,	REX_NAME_FIRST},

	{PSTATE_TUPL_ITEM_NAME,	PSTATE_TUPL_ITEM_NAME,	REX_NAME_ANY},
	{PSTATE_TUPL_ITEM_NAME,	PSTATE_TUPL_COLON,		REX_COLON},

	{PSTATE_KIND_COLON0,	PSTATE_KIND_COLON0,		REX_ALL_SPACES},
	{PSTATE_KIND_COLON0,	PSTATE_KIND_ITEM_NAME,	REX_NAME_FIRST},

	{PSTATE_KIND_ITEM_NAME,	PSTATE_KIND_ITEM_NAME,	REX_NAME_ANY},
	{PSTATE_KIND_ITEM_NAME,	PSTATE_KIND_COLON,		REX_COLON},

	{PSTATE_KIND_COLON,		PSTATE_KIND_COLON,		REX_ALL_SPACES},
	{PSTATE_KIND_COLON,		PSTATE_TYPE_NAME,		REX_NAME_FIRST},

	{PSTATE_TYPE_NAME,		PSTATE_TYPE_NAME,		REX_NAME_ANY},
	{PSTATE_TYPE_NAME,		PSTATE_DIMENSION_IN,	REX_LEFT_SQUARE_BR},
	{PSTATE_TYPE_NAME,		PSTATE_KIND_SEMICOLON,	REX_SEMICOLON},
	{PSTATE_TYPE_NAME,		PSTATE_CONST_END_KIND,	REX_ZERO},

	{PSTATE_DIMENSION_IN,	PSTATE_DIMENSION_IN,	REX_ALL_SPACES},
	{PSTATE_DIMENSION_IN,	PSTATE_DIMENSION_NAME,	REX_NAME_FIRST},
	{PSTATE_DIMENSION_IN,	PSTATE_DIMENSION_INT,	"[0-9]"},

	{PSTATE_DIMENSION_NAME,	PSTATE_DIMENSION_NAME,	REX_NAME_ANY},
	{PSTATE_DIMENSION_NAME,	PSTATE_DIMENSION_SEP,	REX_COMMA},
	{PSTATE_DIMENSION_NAME,	PSTATE_DIMENSION_OUT,	REX_RIGHT_SQUARE_BR},

	{PSTATE_DIMENSION_INT,	PSTATE_DIMENSION_INT,	"[0-9]"},
	{PSTATE_DIMENSION_INT,	PSTATE_DIMENSION_SEP,	REX_COMMA},
	{PSTATE_DIMENSION_INT,	PSTATE_DIMENSION_OUT,	REX_RIGHT_SQUARE_BR},

	{PSTATE_DIMENSION_SEP,	PSTATE_DIMENSION_SEP,	REX_ALL_SPACES},
	{PSTATE_DIMENSION_SEP,	PSTATE_DIMENSION_NAME,	REX_NAME_FIRST},
	{PSTATE_DIMENSION_SEP,	PSTATE_DIMENSION_INT,	"[0-9]"},

	{PSTATE_DIMENSION_OUT,	PSTATE_DIMENSION_OUT,	REX_ALL_SPACES},
	{PSTATE_DIMENSION_OUT,	PSTATE_KIND_SEMICOLON,	REX_SEMICOLON},
	{PSTATE_DIMENSION_OUT,	PSTATE_CONST_END_KIND,	REX_ZERO},

	{PSTATE_KIND_SEMICOLON,	PSTATE_KIND_SEMICOLON,	REX_ALL_SPACES},
	{PSTATE_KIND_SEMICOLON,	PSTATE_KIND_ITEM_NAME,	REX_NAME_FIRST},

	{MAX_NUM_PSTATES}
};


StateSwitch  parser_state_switch;
NextStateLUT hex_hi_LUT, hex_lo_LUT;

/*	-----------------------------------------------
	 Api : I m p l e m e n t a t i o n
--------------------------------------------------- */

Api::Api(pLogger	 a_logger,
		 pConfigFile a_config,
		 pChannels	 a_channels,
		 pVolatile	 a_volatile,
		 pPersisted	 a_persisted,
		 pBebop		 a_bebop,
		 pAgency	 a_agency) : Container(a_logger, a_config) {

	for (int i = 0; i < 1024; i++) http_methods[i] = HTTP_NOTUSED;

	http_methods[TenBitsAtAddress("OPTIONS")] = HTTP_OPTIONS;
	http_methods[TenBitsAtAddress("HEAD")]	  = HTTP_HEAD;
	http_methods[TenBitsAtAddress("GET")]	  = HTTP_GET;
	http_methods[TenBitsAtAddress("PUT")]	  = HTTP_PUT;
	http_methods[TenBitsAtAddress("DELETE")]  = HTTP_DELETE;

	tenbit_double_slash = TenBitsAtAddress("//");

	memset(&parser_state_switch, -1, sizeof(parser_state_switch));

	StateTransition *p_trans = reinterpret_cast<StateTransition *>(&state_tr);
	while (true) {
		if (p_trans->from == MAX_NUM_PSTATES)
			break;

		NextStateLUT *p_next = &parser_state_switch.state[p_trans->from];

		std::regex  rex(p_trans->rex);
		std::string s("-");

		for (int i = 0; i < 256; i ++) {
			s[0] = i;
			if (std::regex_match(s, rex)) {
#ifdef DEBUG
				if (p_next->next[i] != PSTATE_INVALID_CHAR)
					throw 1;
#endif
				p_next->next[i] = p_trans->to;
			}
		};
		p_trans++;
	};

	memset(&hex_hi_LUT, 0, sizeof(NextStateLUT));
	memset(&hex_lo_LUT, 0, sizeof(NextStateLUT));

	int i = 0;
	for (unsigned char c = '0'; c <= '9'; c++) {
		hex_hi_LUT.next[c] = 0x10*i;
		hex_lo_LUT.next[c] = i++;
	};

	i = 0x0a;
	for (unsigned char c = 'A'; c <= 'F'; c++) {
		hex_hi_LUT.next[c + 0x20] = 0x10*i;
		hex_hi_LUT.next[c]		  = 0x10*i;
		hex_lo_LUT.next[c + 0x20] = i;
		hex_lo_LUT.next[c]		  = i++;
	};

	p_channels	= a_channels;
	p_volatile	= a_volatile;
	p_persisted	= a_persisted;
	p_bebop		= a_bebop;
	p_agency	= a_agency;

	base	 = {};
	url_name = {};
}


/** Starts the API service

Configuration-wise the API has just two keys:

- STATIC_HTML_AT_START: which defines a path to a tree of static objects that should be uploaded on start.
- REMOVE_STATICS_ON_CLOSE: removes the whole database Persisted //static when this service closes.

Besides that, this function initializes global (and object) variables used by the parser (mostly CharLUT).
*/
StatusCode Api::start ()
{
	p_channels->base_names(base);
	p_volatile->base_names(base);
	p_persisted->base_names(base);
	p_bebop->base_names(base);
	p_agency->base_names(base);

	base_names(base);

	std::string statics_path;

	if (get_conf_key("STATIC_HTML_AT_START", statics_path)) {
		int ret = _load_statics(statics_path.c_str());
		if (ret != SERVICE_NO_ERROR)
			return ret;
	}

	if (!get_conf_key("REMOVE_STATICS_ON_CLOSE", remove_statics))
		remove_statics = false;

	return Container::start();	// This initializes the one-shot functionality.
}


/** Shuts down the Persisted Service
*/
StatusCode Api::shut_down ()
{
	base.clear();

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** Parse an API url into an APIParseBuffer for later execution.

	\param url		 The http url (that has already been checked to start with //)
	\param method	 The http method in [HTTP_NOTUSED .. HTTP_DELETE]
	\param pars		 A structure with the parts the url successfully parsed ready to be executed.
	\param execution If true (default), locks the nested blocks and creates constants as blocks in the R_Value. Ready for execution.

	\return			 Some error code or SERVICE_NO_ERROR if successful.

	When parse() is successful, the content of the APIParseBuffer **must** be executed by a call (that depends on the method) and will
unlock() all the intermediate blocks.

method | call executed by
-------|-----------------
HTTP_GET, HTTP_HEAD | Api.http_get()
HTTP_PUT | Api.upload()
HTTP_DELETE | Api.remove()
HTTP_OPTIONS | Nothing: options calls must call with `execution = false`

*/
StatusCode Api::parse (const char *url, int method, APIParseBuffer &pars, bool execution)
{
//TODO: Implement Api::parse()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Check a non-API url into and return the static object related with it.

	\param url		 The http url (that has already been checked not to start with //)
	\param response	 A valid (or error) MHD_Response pointer with the resource, status, mime, etc.
	\param execution If true (default), locks the nested blocks and creates constants as blocks in the R_Value. Ready for execution.

	\return			 Some error code or SERVICE_NO_ERROR if successful.

*/
StatusCode Api::get_static (const char *url, pMHD_Response &response, bool execution)
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
MHD_Result Api::return_error_message (struct MHD_Connection *connection, int http_status)
{
	Answer answer;

	sprintf(answer.text, "<html><body><h1><br/><br/>Http error : %d.</h1></body></html>", http_status);

	struct MHD_Response *response = MHD_create_response_from_buffer (strlen(answer.text), answer.text, MHD_RESPMEM_MUST_COPY);

	MHD_Result ret = MHD_queue_response (connection, http_status, response);

	MHD_destroy_response (response);

	return ret;
}


/**	 Execute a put block using some one-shot block as an intermediate buffer.

	\param parse_buff		The structure containing the parts of the url successfully parsed.
	\param upload			A pointer to the data uploaded with the http PUT call.
	\param size				The size of the data uploaded with the http PUT call.
	\param continue_upload  If true, the upload is added at the end of the already existing block.

	\return					true if successful, log(LOG_MISS, "further details") if not.

	This function performs block PUT incrementally. The first time, continue_upload == false and the block is created, all other times,
the block is appended at the end on the existing block.

	This function is **only** called after a successfull parse() of an HTTP_PUT query. It is not private because it is called for the
callback, but it is not intended for any other context.

*/
bool Api::upload (APIParseBuffer &parse_buff, const char *upload, size_t size, bool continue_upload)
{
//TODO: Implement Api::upload()

	return false;
}


/**	 Execute an http DELETE of a block using the block API.

	\param parse_buff The structure containing the parts of the url successfully parsed.

	\return			  true if successful, log(LOG_MISS, "further details") for errors.

	This function is **only** called after a successfull parse() of an HTTP_DELETE query. It is not private because it is called for the
callback, but it is not intended for any other context.

*/
bool Api::remove (APIParseBuffer &parse_buff)
{

	return PARSE_NOT_IMPLEMENTED;
}


/** Execute a get block using the instrumental API.

	\param parse_buff The structure containing the parts of the url successfully parsed.
	\param response	  A valid (or error) MHD_Response pointer with the resource, status, mime, etc.

	\return			  true if successful, log(LOG_MISS, "further details") for errors.

	This function is **only** called after a successfull parse() of HTTP_GET and HTTP_HEAD queries. It is not private because it is called
for the callback, but it is not intended for any other context.

*/
bool Api::http_get (APIParseBuffer &parse_buff, pMHD_Response &response)
{

	return PARSE_NOT_IMPLEMENTED;
}


/** Implements .get() interface to the Api Container.

The Api is a Container (the root class) and includes all the one shot functionality (6 new block methods) and a deque to lock blocks.
It does not implement get(), just defines the interface to be inherited. Api needs this extra method for keeping temporary blocks locked
over possibly more than one http callback query (in uploads) and used by different threads.

This is an implementation of the interface using block names and locators for the only purpose of storing Api-owned Blocks and have some
mechanism to recall the same blocks across http PUT queries.

	\param p_keeper	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_what	An L_value (locator) of the block being uploaded, that will be moved to the appropriate Container when upload is done.

	\return			Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::get(pTransaction *p_keeper, pLocator p_what)
{

	return PARSE_NOT_IMPLEMENTED;
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Api::base_names (BaseNames &base_names)
{
	base_names[""] = this;
}


/** Push a copy of all the files in the path (searched recursively) to the Persisted database "static" and index their names
to be found by get_static().

It also assigns attributes:
- BLOCK_ATTRIB_URL == same relative path after the root path.
- BLOCK_ATTRIB_MIMETYPE guessed from the file extension (html, js, png, etc.)
- BLOCK_ATTRIB_LANGUAGE == en-us

	\param path	The path to the tree of webpage statics.

	\return		Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::_load_statics (const char *path)
{
//TODO: Implement Api::_load_statics()

	return 0;
}


/** This is a private submethod of parse() doing the outer level recursively. See the documentation of Api.parse() for reference.

	\param p_url	 The url as a pointer passed by reference (that updates and points to the conflicting char on error.)
	\param method	 The http method in [HTTP_NOTUSED .. HTTP_DELETE]
	\param l_value		 A structure with the parts the url successfully parsed ready to be executed.
	\param r_value		 A structure with the parts the url successfully parsed ready to be executed.
	\param execution If true (default), locks the nested blocks and creates constants as blocks in the R_Value. Ready for execution.
	\param rec_level The recursion level (starting with 0, breaking with MAX_RECURSION_DEPTH).

	\return			 Some error code or SERVICE_NO_ERROR if successful.


*/
StatusCode Api::_parse_recurse (pChar &p_url, int method, L_value &l_value, R_value &r_value, bool execution, int rec_level)
{

	return PARSE_NOT_IMPLEMENTED;
}


/** This is a private submethod of parse() parsing constants for syntax checking and retrieving the metadata.

It returns a BlockHeader that is necessary to allocate a block calling _parse_const_data() with it.

	\param p_url	A pointer to the first character of the constant.
	\param p_block	A pointer to a BlockHeader (A Block with no alloc) that will be filled on success.

	\return		 Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::_parse_const_meta(pChar &p_url, pBlock p_block)
{
	int state = PSTATE_INITIAL;
	unsigned char cursor, url_enc_byte;

	TensorDim shape	 = {-1, -1, -1, -1, -1, -1};
	TensorDim n_item = { 0,  0,  0,  0,  0,  0};

	int level = -1, tot_cells = 0, cell = 0, max_level = 0, utf8_len = 0, num_items = 0, num_dimensions = 0;
	bool no_brackets = true, is_kind = false;

	while (true) {
		cursor = p_url++[0];
		state  = parser_state_switch.state[state].next[cursor];

		switch (state) {
		case PSTATE_CONST_END_INT:
		case PSTATE_CONST_END_REAL:
		case PSTATE_CONST_END_STR:
		case PSTATE_CONST_END_KIND:
			if (level == 0 && no_brackets) {
				n_item.dim[0]++;
				tot_cells += cell;

				if (shape.dim[0] < 0)
					shape.dim[0] = n_item.dim[0];

				level--;
			}
			if (level != -1)
				return PARSE_BRACKET_MISMATCH;

			memset(p_block, 0, sizeof(Block));

			if (num_items) {
				if (state == PSTATE_CONST_END_KIND)
					p_block->cell_type = CELL_TYPE_KIND_ITEM;
				else
					p_block->cell_type = CELL_TYPE_TUPLE_ITEM;

				shape.dim[0] = num_items;
				shape.dim[1] = 0;

				p_block->set_dimensions(shape.dim);

			} else {
				switch (state) {
				case PSTATE_CONST_END_INT:
					p_block->cell_type = CELL_TYPE_INTEGER;
					break;
				case PSTATE_CONST_END_REAL:
					p_block->cell_type = CELL_TYPE_DOUBLE;
					break;
				case PSTATE_CONST_END_STR:
					p_block->cell_type = CELL_TYPE_STRING;
					break;
				}
				p_block->set_dimensions(shape.dim);

				if (p_block->size != tot_cells)
					return PARSE_ERROR_INVALID_SHAPE;
			}

			return PARSE_OK;

		case PSTATE_CONST_STR0:
			if (utf8_len)
				return PARSE_ERROR_ENCODING;

		case PSTATE_CONST_INT:
		case PSTATE_CONST_REAL:
			if (level < 0) {
				if (tot_cells > 0)
					return PARSE_BRACKET_MISMATCH;

				level = 0;
			}
			cell = 1;
			break;

		case PSTATE_CONST_IN_INT:
		case PSTATE_CONST_IN_REAL:
		case PSTATE_CONST_IN_STR:
		case PSTATE_CONST_IN_UNK:
			if (cursor == '[') {
				no_brackets = false;
				level++;
				if (tot_cells) {
					if (level > max_level)
						return PARSE_ERROR_INVALID_SHAPE;
				} else {
					max_level = level;
				}
				if (level >= MAX_TENSOR_RANK)
					return PARSE_ERROR_TOO_DEEP;

				n_item.dim[level] = 0;
			};
			break;

		case PSTATE_CONST_OUT_STR:
			if (utf8_len)
				return PARSE_ERROR_ENCODING;

		case PSTATE_CONST_OUT_INT:
		case PSTATE_CONST_OUT_REAL:
			if (cursor == ']') {
				if (level < 0)
					return PARSE_BRACKET_MISMATCH;

				n_item.dim[level]++;
				tot_cells += cell;
				cell = 0;

				if (shape.dim[level] < 0) {
					shape.dim[level] = n_item.dim[level];
				} else {
					if (shape.dim[level] != n_item.dim[level])
						return PARSE_ERROR_INVALID_SHAPE;
				};
				level--;
			};
			break;

		case PSTATE_CONST_SEP_STR:
			if (utf8_len)
				return PARSE_ERROR_ENCODING;

		case PSTATE_CONST_SEP_INT:
		case PSTATE_CONST_SEP_REAL:
			if (cursor == ',') {
				n_item.dim[level]++;
				tot_cells += cell;
				cell = 0;
			};
			break;

		case PSTATE_CONST_STR:
			if (utf8_len)
				return PARSE_ERROR_ENCODING;

			break;

		case PSTATE_CONST_STR_ENC1:
			url_enc_byte = hex_hi_LUT.next[cursor];

			break;

		case PSTATE_CONST_STR_ENC2:
			if (utf8_len) {
				utf8_len--;
			} else {
				url_enc_byte += hex_lo_LUT.next[cursor];

				if		((url_enc_byte & 0xE0) == 0xC0)	// 110x xxxx
					utf8_len = 1;
				else if ((url_enc_byte & 0xF0) == 0xE0)	// 1110 xxxx
					utf8_len = 2;
				else if ((url_enc_byte & 0xF8) == 0xF0)	// 1111 0xxx
					utf8_len = 3;
			}

			break;

		case PSTATE_KIND_COLON0:
			if (cursor == ':') {
				num_items--;
				if (num_items)
					return PARSE_ERROR_INVALID_CHAR;

				is_kind = true;
			}

			break;

		case PSTATE_TUPL_COLON:
		case PSTATE_KIND_COLON:
			if (cursor == ':')
				num_items++;

			break;

		case PSTATE_DIMENSION_IN:
			num_dimensions = 0;

			break;

		case PSTATE_DIMENSION_SEP:
			if (cursor == ',') {
				num_dimensions++;
				if (num_dimensions >= MAX_TENSOR_RANK)
					return PARSE_ERROR_INVALID_SHAPE;
			}

			break;

		case PSTATE_TUPL_SEMICOLON:
			if (cursor == ';') {
				if (level == 0 && no_brackets)
					level--;

				if (level != -1)
					return PARSE_BRACKET_MISMATCH;

				shape  = {-1, -1, -1, -1, -1, -1};
				n_item = { 0,  0,  0,  0,  0,  0};

				level = -1, tot_cells = 0, cell = 0, max_level = 0, utf8_len = 0;
				no_brackets = true;
			}

			break;

		case PSTATE_INITIAL:
		case PSTATE_CONST_SEP_STR0:
		case PSTATE_CONST_STR_ENC0:
		case PSTATE_BASE_NAME:
		case PSTATE_TUPL_ITEM_NAME:
		case PSTATE_KIND_ITEM_NAME:
		case PSTATE_TYPE_NAME:
		case PSTATE_DIMENSION_NAME:
		case PSTATE_DIMENSION_INT:
		case PSTATE_DIMENSION_OUT:
		case PSTATE_KIND_SEMICOLON:
			break;

		default:
			return PARSE_ERROR_INVALID_CHAR;
		}
	}
}


/** This is a private submethod of parse() creating blocks with the data of a constant previously checked by _parse_const_meta()

It returns a locked (one shot) block that must be unlocked in the same http query.

	\param p_url	A pointer to the first character of the constant.
	\param hea		A BlockHeader created in a successful _parse_const_meta.
	\param p_keeper	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** unlock() it when done.

	\return			Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::_parse_const_data(pChar &p_url, BlockHeader &hea, pTransaction *p_keeper)
{
	int state = PSTATE_INITIAL;
	int state_recency = -1, next_state;
	unsigned char cursor;

	while (true) {
		cursor		  = p_url++[0];
		next_state	  = parser_state_switch.state[state].next[cursor];
		state_recency = next_state == state ? state_recency + 1 : 0;
		state		  = next_state;

		return PARSE_NOT_IMPLEMENTED;
	}
}

} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

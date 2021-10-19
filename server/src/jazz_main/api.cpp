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


#include <sys/utsname.h>


#include "src/jazz_main/api.h"


namespace jazz_main
{

/// Http methods

#define HTTP_NOTUSED					 0		///< Rogue value to fill the LUTs
#define HTTP_OPTIONS					 1		///< http predicate OPTIONS
#define HTTP_HEAD						 2		///< http predicate HEAD
#define HTTP_GET						 3		///< http predicate GET
#define HTTP_PUT						 4		///< http predicate PUT
#define HTTP_DELETE						 5		///< http predicate DELETE

/*	-----------------------------------------------
	 Parser grammar definition
--------------------------------------------------- */

#define REX_SLASH				"[/]"
#define REX_NAME_FIRST			"[a-zA-Z0-9~]"
#define REX_NAME_ANY			"[a-zA-Z0-9\\-_~$]"
#define REX_BASE_SWITCH			"[&]"
#define REX_INFO_SWITCH			"[\\x00]"
#define REX_ENT_SWITCH			"[\\x00\\.\\]\\)]"
#define REX_KEY_SWITCH			"[\\x00\\.:=\\[\\(\\]\\)]"

#define MAX_NUM_PSTATES			14		///< Maximum number of non error states the parser can be in
#define NUM_STATE_TRANSITIONS	20		///< Maximum number of state transitions in the parsing grammar. Applies to const only.

/// Parser state values

#define PSTATE_INITIAL			 0		///< Begin parsing, assumes already seen / to avoid unnecessary initial counting
#define PSTATE_DONE_NODE		 1		///< Equivalent to PSTATE_INITIAL, except node was done and it cannot happen again
#define PSTATE_NODE0			 2		///< Already seen ///
#define PSTATE_IN_NODE			 3		///< Name starts after /// + letter, stays with valid char
#define PSTATE_BASE0			 4		///< Already seen //
#define PSTATE_IN_BASE			 5		///< Name starts after // + letter, stays with valid char
#define PSTATE_ENTITY0			 6		///< Already seen / after reading a base
#define PSTATE_IN_ENTITY		 7		///< Name starts after / + letter, stays with valid char
#define PSTATE_KEY0				 8		///< Already seen / after reading an entity
#define PSTATE_IN_KEY			 9		///< Name starts after / + letter, stays with valid char
#define PSTATE_INFO_SWITCH		10		///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_BASE_SWITCH		11		///< Found # while reading a base
#define PSTATE_ENT_SWITCH		12		///< Found # while reading a base
#define PSTATE_KEY_SWITCH		13		///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_FAILED			98		///< Set by the parser on any error (possibly in the r_value too)
#define PSTATE_COMPLETE_OK		99		///< Set by the parser on complete success

/** A vector of StateTransition. This only runs once, when contruction the API object, initializes the LUTs from a sequence of
StateTransition constants in the source of api.cpp.
*/
typedef ParseStateTransition ParseStateTransitions[NUM_STATE_TRANSITIONS];

ParseStateTransitions state_tr = {
	{PSTATE_INITIAL,	PSTATE_BASE0,		REX_SLASH},

	{PSTATE_BASE0,		PSTATE_NODE0,		REX_SLASH},
	{PSTATE_BASE0,		PSTATE_IN_BASE,		REX_NAME_FIRST},

	{PSTATE_NODE0,		PSTATE_IN_NODE,		REX_NAME_FIRST},
	{PSTATE_NODE0,		PSTATE_INFO_SWITCH,	REX_INFO_SWITCH},

	{PSTATE_IN_BASE,	PSTATE_IN_BASE,		REX_NAME_ANY},
	{PSTATE_IN_BASE,	PSTATE_ENTITY0,		REX_SLASH},
	{PSTATE_IN_BASE,	PSTATE_BASE_SWITCH,	REX_BASE_SWITCH},

	{PSTATE_IN_NODE,	PSTATE_IN_NODE,		REX_NAME_ANY},
	{PSTATE_IN_NODE,	PSTATE_DONE_NODE,	REX_SLASH},

	{PSTATE_DONE_NODE,	PSTATE_BASE0,		REX_SLASH},

	{PSTATE_ENTITY0,	PSTATE_IN_ENTITY,	REX_NAME_FIRST},

	{PSTATE_IN_ENTITY,	PSTATE_IN_ENTITY,	REX_NAME_ANY},
	{PSTATE_IN_ENTITY,	PSTATE_KEY0,		REX_SLASH},
	{PSTATE_IN_ENTITY,	PSTATE_ENT_SWITCH,	REX_ENT_SWITCH},

	{PSTATE_KEY0,		PSTATE_IN_KEY,		REX_NAME_FIRST},
	{PSTATE_KEY0,		PSTATE_KEY_SWITCH,	REX_KEY_SWITCH},

	{PSTATE_IN_KEY,		PSTATE_IN_KEY,		REX_NAME_ANY},
	{PSTATE_IN_KEY,		PSTATE_KEY_SWITCH,	REX_KEY_SWITCH},

	{MAX_NUM_PSTATES}
};

ParseNextStateLUT parser_state_switch[MAX_NUM_PSTATES];

/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

#define MHD_HTTP_ANYERROR true

#ifndef CATCH_TEST
#ifdef DEBUG
MHD_Result print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);

	return MHD_YES;
}
#endif
#endif


/// Indices inside state (anything else is a pTransaction of a PUT call).
#define	STATE_NEW_CALL			0		///< Default state: connection open for any call
#define	STATE_NOT_ACCEPTABLE	1		///< Data upload failed, query execution failed locating tagets. Returns MHD_HTTP_NOT_ACCEPTABLE
#define	STATE_BAD_REQUEST		2		///< PUT query is call malformed. Returns MHD_HTTP_BAD_REQUEST.

int callback_state [3];

char response_put_ok[]			= "0";
char response_put_fail[]		= "1";

TenBitIntLUT http_methods;				///< A LUT to convert argument const char *method int an integer code.
TenBitPtrLUT base_server;				///< A LUT to convert argument const char *method int an integer code.

#ifndef CATCH_TEST

/** Callback function for MHD. See: https://www.gnu.org/software/libmicrohttpd/tutorial.html

	Jazz does not use post processor callbacks linked with MHD_create_post_processor().
	Jazz does not use request completed callbacks linked with MHD_start_daemon().

	The only callback functions are:

		1. This (http_request_callback): The full operational blocks and instrumental API.
		2. http_apc_callback():			 An IP based (firewall like) that can filter based on called IPs.

	This function is multithreaded with the default configuration settings. (Other settings are untested for the moment.)

	The internal operation of the callback function is subject to change and the remarks in the source code are the description of it.
*/
MHD_Result http_request_callback(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
								 const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {

	// Step 1: First opportunity to end the connection before uploading or getting. Not used. We initialize con_cls for the next call.

	if (*con_cls == NULL) {
		*con_cls = &callback_state[STATE_NEW_CALL];

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int http_method = http_methods[TenBitsAtAddress(method)];

	HttpQueryState q_state;
	q_state.state = PSTATE_INITIAL;

	struct MHD_Response *response = nullptr;

	if ((uintptr_t) *con_cls < (uintptr_t) &callback_state || (uintptr_t) *con_cls > (uintptr_t) &callback_state[2]) {
		if (http_method != HTTP_PUT || !API.parse(q_state, (pChar) url, HTTP_PUT)) {
			LOGGER.log(LOG_MISS, "http_request_callback(): Trying to continue state_upload_in_progress, but API.parse() failed.");

			return MHD_NO;
		}
		q_state.rr_value.p_extra = (pExtraLocator) *con_cls;

		int sequence = (*upload_data_size == 0) ? SEQUENCE_FINAL_CALL : SEQUENCE_INCREMENT_CALL;

		switch (API.http_put((pChar) upload_data, *upload_data_size, q_state, sequence)) {
		case MHD_HTTP_CREATED:
			goto create_response_answer_put_ok;

		case MHD_HTTP_OK:
			goto continue_in_put_ok;
		}
		goto continue_in_put_notacceptable;
	}

	// Step 3 : Get rid of failed uploads without doing anything.

	if (*con_cls == &callback_state[STATE_NOT_ACCEPTABLE]) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_notacceptable;

		return MHD_YES;
	}

	if (*con_cls == &callback_state[STATE_BAD_REQUEST]) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_badrequest;

		return MHD_YES;
	}

	// Step 4 : This point is reached just once per http petition. Parse the query, returns errors and web pages, continue to API.

#ifdef DEBUG
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - cls \x20 \x20 \x20 \x20 \x20 \x20 \x20: %p", cls);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - connection \x20 \x20 \x20 : %p", connection);
	MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key, NULL);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - url \x20 \x20 \x20 \x20 \x20 \x20 \x20: %s", url);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - method \x20 \x20 \x20 \x20 \x20 : %s", method);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - version \x20 \x20 \x20 \x20 \x20: %s", version);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data \x20 \x20 \x20: %p", upload_data);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data_size : %d", *upload_data_size);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - *con_cls \x20 \x20 \x20 \x20 : %p", *con_cls);
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
#endif

	MHD_StatusCode status;

	switch (http_method) {
	case HTTP_NOTUSED:
		return API.return_error_message(connection, (pChar) url, MHD_HTTP_METHOD_NOT_ALLOWED);

	case HTTP_OPTIONS: {	// Shield variable "allow" initialization to support the goto logic.

			std::string allow;

			if (url[0] != '/' || url[1] != '/') {
				if (API.get_static(response, (pChar) url, false) == MHD_HTTP_OK)
					allow = "HEAD,GET,";

				allow = allow + "OPTIONS";
			} else {
				if (API.parse(q_state, (pChar) url, HTTP_GET))
					allow = "HEAD,GET,";

				if (API.parse(q_state, (pChar) url, HTTP_PUT))
					allow = allow + "PUT,";

				if (API.parse(q_state, (pChar) url, HTTP_DELETE))
					allow = allow + "DELETE,";

				allow = allow + "OPTIONS";
			}

			response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

			MHD_add_response_header(response, MHD_HTTP_HEADER_SERVER, "Jazz " JAZZ_VERSION " - " LINUX_PLATFORM);
			MHD_add_response_header(response, MHD_HTTP_HEADER_ALLOW, allow.c_str());
		}

		status = MHD_HTTP_NO_CONTENT;

		goto answer_status;

	case HTTP_HEAD:
	case HTTP_GET:
		if (url[0] != '/' || url[1] != '/') {

			if ((status = API.get_static(response, (pChar) url)) != MHD_HTTP_OK)
				return API.return_error_message(connection, (pChar) url, status);

			goto answer_status;

		} else if (!API.parse(q_state, (pChar) url, HTTP_GET))
			return API.return_error_message(connection, (pChar) url, MHD_HTTP_BAD_REQUEST);

		break;

	default:
		if (url[0] != '/' || url[1] != '/')
			return API.return_error_message(connection, (pChar) url, MHD_HTTP_METHOD_NOT_ALLOWED);

		else if (!API.parse(q_state, (pChar) url, http_method)) {

			if (http_method == HTTP_PUT)
				goto continue_in_put_badrequest;

			return API.return_error_message(connection, (pChar) url, MHD_HTTP_BAD_REQUEST);
		}
	}

	// Step 5 : This is the core. This point is only reached by correct API queries for the first (or only) time.

	switch (http_method) {
	case HTTP_PUT:
		status = API.http_put((pChar) upload_data, *upload_data_size, q_state, SEQUENCE_FIRST_CALL);

		break;

	case HTTP_DELETE:
		status = API.http_delete(q_state);

		break;

	default:

		status = API.http_get(response, q_state);
	}

	// Step 6 : The core finished, just distribute the answer as appropriate.

	if (http_method == HTTP_PUT) {
		if (status == MHD_HTTP_OK) {
			if (*upload_data_size) goto continue_in_put_ok;
			else 				   goto create_response_answer_put_ok;
		} else {
			if (*upload_data_size) goto continue_in_put_notacceptable;
			else				   goto create_response_answer_put_notacceptable;
		}
	}

	if (status != MHD_HTTP_OK)
		return API.return_error_message(connection, (pChar) url, status);

	if (http_method == HTTP_DELETE)
		response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	MHD_Result ret;

answer_status:

	ret = MHD_queue_response(connection, status, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_ok:

	response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_notacceptable:

	response = MHD_create_response_from_buffer(1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_NOT_ACCEPTABLE, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_badrequest:

	response = MHD_create_response_from_buffer(1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);

	MHD_destroy_response(response);

	return ret;

continue_in_put_notacceptable:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &callback_state[STATE_NOT_ACCEPTABLE];

		return MHD_YES;
	}

	return MHD_NO;

continue_in_put_badrequest:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &callback_state[STATE_BAD_REQUEST];

		return MHD_YES;
	}

	return MHD_NO;

continue_in_put_ok:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = q_state.rr_value.p_extra;

		return MHD_YES;
	}

	return MHD_NO;
}

#endif

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

	compile_next_state_LUT(parser_state_switch, MAX_NUM_PSTATES, state_tr);

	for (int i = 0; i < 1024; i++)
		http_methods[i] = HTTP_NOTUSED;

	http_methods[TenBitsAtAddress("OPTIONS")] = HTTP_OPTIONS;
	http_methods[TenBitsAtAddress("HEAD")]	  = HTTP_HEAD;
	http_methods[TenBitsAtAddress("GET")]	  = HTTP_GET;
	http_methods[TenBitsAtAddress("PUT")]	  = HTTP_PUT;
	http_methods[TenBitsAtAddress("DELETE")]  = HTTP_DELETE;

	p_channels	= a_channels;
	p_volatile	= a_volatile;
	p_persisted	= a_persisted;
	p_bebop		= a_bebop;
	p_agency	= a_agency;

	www	 = {};
}


Api::~Api() { destroy_container(); }


/** Starts the API service

Configuration-wise the API has just two keys:

- STATIC_HTML_AT_START: which defines a path to a tree of static objects that should be uploaded on start.
- REMOVE_STATICS_ON_CLOSE: removes the whole database Persisted //static when this service closes.

Besides that, this function initializes global (and object) variables used by the parser (mostly CharLUT).
*/
StatusCode Api::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	BaseNames base = {};

	p_channels->base_names(base);
	p_volatile->base_names(base);
	p_persisted->base_names(base);
	p_bebop->base_names(base);
	p_agency->base_names(base);

	for (int i = 0; i < 1024; i++)
		base_server[i] = nullptr;

	for (BaseNames::iterator it = base.begin(); it != base.end(); ++it) {
		int tt = TenBitsAtAddress(it->first.c_str());

		if (base_server[tt] != nullptr) {
			log_printf(LOG_ERROR, "Api::start(): Base name conflict with \"%s\"", it->first.c_str());

			return SERVICE_ERROR_STARTING;
		}
		base_server[tt] = it->second;
	}

	std::string statics_path;

	if (get_conf_key("STATIC_HTML_AT_START", statics_path)) {
		ret = load_statics((pChar) statics_path.c_str(), (pChar) "/", 0);

		if (ret != SERVICE_NO_ERROR) {
			log_printf(LOG_ERROR, "Api::start(): load_statics() failed loading \"%s\"", statics_path.c_str());

			return ret;
		}
	}

	if (!get_conf_key("REMOVE_STATICS_ON_CLOSE", remove_statics))
		remove_statics = false;

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Api::shut_down() {

	StatusCode err;

	if (remove_statics) {
		Locator loc = {"lmdb", "www"};
		for (Index::iterator it = www.begin(); it != www.end(); ++it) {
			strcpy(loc.key, it->second.c_str());
			if ((err = p_persisted->remove(loc)) != SERVICE_NO_ERROR)
				log_printf(LOG_MISS, "Api::shut_down(): Persisted.remove(//lmdb/www/%s) returned %d", it->second.c_str(), err);
		}
	}

	www.clear();

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** Parse an API url into an APIParseBuffer for later execution.

	\param q_state	A structure with the parts the url successfully parsed ready to be executed.
	\param p_url	The http url (that has already been checked to start with //)
	\param method	The http method in [HTTP_NOTUSED .. HTTP_DELETE]
	\param recurse	True in an assignment while processing the r_value

	\return			Some error code or SERVICE_NO_ERROR if successful.

	When parse() is successful, the content of the APIParseBuffer **must** be executed by a call (that depends on the method) and will
unlock() all the intermediate blocks.

method | call executed by
-------|-----------------
HTTP_GET, HTTP_HEAD | Api.http_get()
HTTP_PUT | Api.http_put()
HTTP_DELETE | Api.http_delete()
HTTP_OPTIONS | Nothing: options calls must call with **execution = false**

*/
bool Api::parse(HttpQueryState &q_state, pChar p_url, int method, bool recurse) {

	int buf_size;
	pChar p_out;

	if (!recurse) {
		q_state.l_node[0] = 0;
		q_state.r_node[0] = 0;
		q_state.name[0]	  = 0;
	}
	q_state.url[0] = 0;
	q_state.apply  = APPLY_NOTHING;
	q_state.state  = PSTATE_INITIAL;

	p_url++;	// parse() is only called after checking the trailing //, this skips the first / to set state to PSTATE_INITIAL

	while (true) {
		unsigned char cursor;

		cursor = *(p_url++);
		q_state.state = parser_state_switch[q_state.state].next[cursor];
		if ((q_state.state == PSTATE_KEY_SWITCH) && (cursor == '.') && (p_url[1] <= '9') && (p_url[1] >= '0'))
			q_state.state = PSTATE_IN_KEY;

		switch (q_state.state) {
		case PSTATE_NODE0:
			if (recurse)
				p_out = (pChar) &q_state.r_node;
			else
				p_out = (pChar) &q_state.l_node;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_BASE0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.base;
			else
				p_out = (pChar) &q_state.base;

			buf_size = SHORT_NAME_SIZE - 1;

			break;

		case PSTATE_ENTITY0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.entity;
			else
				p_out = (pChar) &q_state.entity;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_KEY0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.key;
			else
				p_out = (pChar) &q_state.key;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_IN_NODE:
		case PSTATE_IN_BASE:
		case PSTATE_IN_ENTITY:
		case PSTATE_IN_KEY:
			if (buf_size-- == 0) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			*(p_out++) = cursor;
			*(p_out)   = 0;

			break;

		case PSTATE_INFO_SWITCH:
			q_state.base[0]	  = 0;
			q_state.entity[0] = 0;
			q_state.key[0]	  = 0;

			if (method == HTTP_GET) {
				q_state.apply = APPLY_JAZZ_INFO;
				q_state.state = PSTATE_COMPLETE_OK;

				return true;
			}
			q_state.state = PSTATE_FAILED;

			return false;

		case PSTATE_BASE_SWITCH:
			int mc;
			if (recurse)
				mc = move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url - 1, q_state.r_value.base);
			else
				mc = move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url - 1, q_state.base);

			if (mc == RET_MV_CONST_FAILED) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.apply = mc == RET_MV_CONST_NOTHING ? APPLY_URL : APPLY_NEW_ENTITY;
			q_state.state = PSTATE_COMPLETE_OK;
			if (recurse) {
				q_state.r_value.entity[0] = 0;
				q_state.r_value.key[0]	  = 0;
			} else {
				q_state.entity[0] = 0;
				q_state.key[0]	  = 0;
			}
			return true;

		case PSTATE_ENT_SWITCH:
			if (cursor == 0 && method != HTTP_DELETE) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.state = PSTATE_COMPLETE_OK;
			if (recurse)
				q_state.r_value.key[0] = 0;
			else
				q_state.key[0] = 0;

			if (cursor != '.')
				return true;

			if (method != HTTP_GET || strcmp("new", p_url) != 0) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.apply = APPLY_NEW_ENTITY;

			return true;

		case PSTATE_KEY_SWITCH:
			q_state.state = PSTATE_FAILED;

			if (p_out == (pChar) q_state.key || p_out == (pChar) q_state.r_value.key) {
				if (cursor != '(')
					return false;

				if (recurse)
					q_state.r_value.key[0] = 0;
				else
					q_state.key[0] = 0;

				if (method != HTTP_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FUNCT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))
					q_state.apply = APPLY_FUNCTION;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;
			}

			switch (cursor) {
			case 0:
				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '.':
				if (method != HTTP_GET && method != HTTP_PUT)
					return false;

				if (strcmp("raw", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_RAW;

					return true;
				}
				if (strcmp("text", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_TEXT;

					return true;
				}
				if (method != HTTP_GET)
					return false;

				if (strcmp("new", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_NEW_ENTITY;

					return true;
				}
				if (strncmp(p_url, "attribute(", 10) != 0)
					return false;

				p_url += 10;

				int i_len;

				if (sscanf(p_url, "%i%n", &q_state.r_value.attribute, &i_len) != 1)
					return false;

				p_url += i_len;

				if (*(p_url++) != ')')
					return false;

				switch (*(p_url++)) {
				case 0:
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_GET_ATTRIBUTE;

					return true;

				case '=':
					if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING) {
						q_state.state = PSTATE_COMPLETE_OK;
						q_state.apply = APPLY_SET_ATTRIBUTE;

						return true;
					}
				}
				return false;

			case ':':
				if (method != HTTP_GET || strlen(p_url) >= NAME_SIZE)
					return false;

				strcpy(q_state.name, p_url);
				q_state.apply = APPLY_NAME;
				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '=':
				if (recurse || (method != HTTP_GET))
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING) {
					q_state.apply = APPLY_ASSIGN_CONST;
					q_state.state = PSTATE_COMPLETE_OK;

					return true;
				}
				if ((*p_url != '/') || !parse(q_state, p_url, HTTP_GET, true))
					return false;

				switch (q_state.apply) {
				case APPLY_NOTHING:
					q_state.apply = APPLY_ASSIGN_NOTHING;
					return true;
				case APPLY_NAME:
					q_state.apply = APPLY_ASSIGN_NAME;
					return true;
				case APPLY_URL:
					q_state.apply = APPLY_ASSIGN_URL;
					return true;
				case APPLY_FUNCTION:
					q_state.apply = APPLY_ASSIGN_FUNCTION;
					return true;
				case APPLY_FUNCT_CONST:
					q_state.apply = APPLY_ASSIGN_FUNCT_CONST;
					return true;
				case APPLY_FILTER:
					q_state.apply = APPLY_ASSIGN_FILTER;
					return true;
				case APPLY_FILT_CONST:
					q_state.apply = APPLY_ASSIGN_FILT_CONST;
					return true;
				case APPLY_RAW:
					q_state.apply = APPLY_ASSIGN_RAW;
					return true;
				case APPLY_TEXT:
					q_state.apply = APPLY_ASSIGN_TEXT;
					return true;
				}
				q_state.state = PSTATE_FAILED;

				return false;

			case '[':
				if (method != HTTP_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FILT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))

					q_state.apply = APPLY_FILTER;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '(':
				if (method != HTTP_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FUNCT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))
					q_state.apply = APPLY_FUNCTION;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			default:
				q_state.state = PSTATE_FAILED;

				return false;
			}
			break;

		case PSTATE_DONE_NODE:
			if (!recurse) {
				q_state.url[0] = '/';
				strcpy((pChar) &q_state.url[1], p_url);
			}
			break;

		default:
			q_state.state = PSTATE_FAILED;

			return false;
		}
	}
}


/** Check a non-API url into and return the static object related with it.

	\param response	A valid (or error) MHD_Response pointer with the resource, status, mime, etc.
	\param p_url	The http url (that has already been checked not to start with //)
	\param get_it	If true (default), it actually gets it as a response, otherwise it just check if it exists.

	\return			Some error code or SERVICE_NO_ERROR if successful.

*/
MHD_StatusCode Api::get_static(pMHD_Response &response, pChar p_url, bool get_it) {

	Index::iterator it = www.find(std::string(p_url));

	if (it == www.end())
		return MHD_HTTP_NOT_FOUND;

	Locator loc = {"lmdb", "www"};

	strcpy(loc.key, it->second.c_str());

	pTransaction p_txn;
	if (p_persisted->get(p_txn, loc) != SERVICE_NO_ERROR)
		return MHD_HTTP_BAD_GATEWAY;

	int size = (p_txn->p_block->cell_type & 0xff)*p_txn->p_block->size;

	response = MHD_create_response_from_buffer(size, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);

	pChar p_att;
	if ((p_att = p_txn->p_block->get_attribute(BLOCK_ATTRIB_MIMETYPE)) != nullptr)
		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, p_att);

	if ((p_att = p_txn->p_block->get_attribute(BLOCK_ATTRIB_LANGUAGE)) != nullptr)
		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_LANGUAGE, p_att);

	p_persisted->destroy_transaction(p_txn);

	return MHD_HTTP_OK;
}


/** Finish a query by delivering the appropriate message page.

	\param connection  The MHD connection passed to the callback function. (Needed for MHD_queue_response()ing the response.)
	\param p_url	   The url that failed.
	\param http_status The http status error (e.g., MHD_HTTP_NOT_FOUND)

	\return			   A valid answer for an MHD callback. It is an integer generated by MHD_queue_response() and returned by the callback.

	This function searches for a persistence block named ("www", "httpERR_%d") where %d is the code in decimal and serves it as an answer.
*/
MHD_Result Api::return_error_message(pMHD_Connection connection, pChar p_url, int http_status) {

	char answer[2048];

	sprintf(answer,
			"<html><head><style type=\"text/css\">"
			"*{transition: all 0.6s;}"
			"html {height: 100%%;}"
			"body{font-family: 'Lato', sans-serif; color: #081040; margin: 0;}"
			"#main{display: table; width: 100%%; height: 100vh; text-align: center;}"
			".fof{display: table-cell;}"
			".fof h1{font-size: 50px; display: inline-block; padding-right: 12px; animation: type .5s alternate infinite;}"
			"@keyframes type{from{box-shadow: inset -4px 0px 0px #102080;}to{box-shadow: inset -4px 0px 0px transparent;}}"
			"</style></head><body>"
			"<div style=\"background-color:#fffcf8;padding: 12px 30px 6px 30px;\"><h2>Http error on : %.140s</h2></div>"
			"<hr/>"
			"<div id=\"main\" style=\"background-color:#f0f8ff;\"><div class=\"fof\"><br/><br/><h1>Error %d</h1>"
			"<hr style=\"height:2px; width:35%%; border-width:0; color:red; background-color:red\">"
			"</div></div></body></html>",
			p_url,
			http_status);

	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(answer), answer, MHD_RESPMEM_MUST_COPY);

	MHD_Result ret = MHD_queue_response(connection, http_status, response);

	MHD_destroy_response(response);

	return ret;
}


/**	 Execute a put block using some one-shot block as an intermediate buffer.

	\param p_upload	A pointer to the data uploaded with the http PUT call.
	\param size		The size of the data uploaded with the http PUT call.
	\param q_state	The structure containing the parts of the url successfully parsed.
	\param sequence SEQUENCE_FIRST_CALL, SEQUENCE_INCREMENT_CALL or SEQUENCE_FINAL_CALL. (See below)

	\return			MHD_HTTP_CREATED if SEQUENCE_FINAL_CALL is successful, MHD_HTTP_OK if any othe call is successful, or any HTTP error
					status code.

	This function is **only** called after a successfull parse() of an HTTP_PUT query. It is not private because it is called for the
callback, but it is not intended for any other context.

Internals
---------

It supports any successful HTTP_PUT syntax, that is:

APPLY_NOTHING: With or without node, mandatory base, entity and key.
APPLY_RAW & APPLY_TEXT: With or without node, mandatory base, entity and key.
APPLY_URL: With or without node and just a base.

In all cases, calls with a node (it can only be l_node) q_state.url contains exactly what has to be forwarded.

Call logic:
-----------

SEQUENCE_FIRST_CALL comes first and is mandatory. On success, the functions keeps the data in a block stored in a pTransaction in
q_state.rr_value.p_extra (that pointer will be returned in successive call of the same PUT query).
SEQUENCE_INCREMENT_CALL may or may not come, if it does, it must allocate bigger blocks and store more data in the same pTransaction.
SEQUENCE_FINAL_CALL is called just once, it must destroy the pTransaction when done
*/
MHD_StatusCode Api::http_put(pChar p_upload, size_t size, HttpQueryState &q_state, int sequence) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	pTransaction p_txn = (pTransaction) q_state.rr_value.p_extra;

	switch (sequence) {
	case SEQUENCE_FIRST_CALL: {
		if (size == 0)
			return MHD_HTTP_OK;

		int dim[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};

		dim[0] = size;

		if (new_block(p_txn, CELL_TYPE_BYTE, (int *) &dim, FILL_NEW_DONT_FILL) !=  SERVICE_NO_ERROR)
			return MHD_HTTP_INSUFFICIENT_STORAGE;

		memcpy(&p_txn->p_block->tensor.cell_byte[0], p_upload, size);

		q_state.rr_value.p_extra = (pExtraLocator) p_txn; }

		return MHD_HTTP_OK;

	case SEQUENCE_INCREMENT_CALL: {
		int dim[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};

		int prev_size = p_txn->p_block->size;

		dim[0] = prev_size + size;

		pTransaction p_aux;

		if (new_block(p_aux, CELL_TYPE_BYTE, (int *) &dim, FILL_NEW_DONT_FILL) !=  SERVICE_NO_ERROR) {
			destroy_transaction(p_txn);

			return MHD_HTTP_INSUFFICIENT_STORAGE;
		}
		memcpy(&p_aux->p_block->tensor.cell_byte[0], &p_txn->p_block->tensor.cell_byte[0], prev_size);
		memcpy(&p_aux->p_block->tensor.cell_byte[prev_size], p_upload, size);

		std::swap(p_txn->p_block, p_aux->p_block);

		destroy_transaction(p_aux); }

		return MHD_HTTP_OK;
	}

	if (q_state.l_node[0] != 0) {
		int ret = p_channels->forward_put(q_state.l_node, q_state.url, p_txn->p_block);

		destroy_transaction(p_txn);

		if (ret == SERVICE_NO_ERROR)
			return MHD_HTTP_CREATED;

		return MHD_HTTP_BAD_GATEWAY;
	}

	pContainer p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

	if (p_container == nullptr) {
		destroy_transaction(p_txn);

		return MHD_HTTP_SERVICE_UNAVAILABLE;
	}

	Locator loc;

	switch (q_state.apply) {
	case APPLY_NOTHING: {
		memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

		int ret = p_container->put(loc, p_txn->p_block);

		destroy_transaction(p_txn);

		if (ret == SERVICE_NO_ERROR)
			return MHD_HTTP_CREATED; }

		return MHD_HTTP_BAD_GATEWAY;

	case APPLY_RAW: {
		pTransaction p_aux;

		int ret = new_block(p_aux, p_txn->p_block, CELL_TYPE_UNDEFINED);

		destroy_transaction(p_txn);

		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_BAD_REQUEST;

		memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

		ret = p_container->put(loc, p_aux->p_block);

		destroy_transaction(p_aux);

		if (ret == SERVICE_NO_ERROR)
			return MHD_HTTP_CREATED; }

		return MHD_HTTP_BAD_GATEWAY;

	case APPLY_TEXT: {
		pTransaction p_aux;

		int ret = new_block(p_aux, p_txn->p_block);

		destroy_transaction(p_txn);

		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_BAD_REQUEST;

		memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

		ret = p_container->put(loc, p_aux->p_block);

		destroy_transaction(p_aux);

		if (ret == SERVICE_NO_ERROR)
			return MHD_HTTP_CREATED; }

		return MHD_HTTP_BAD_GATEWAY;

	case APPLY_URL: {
		int ret = p_container->put(q_state.url, p_txn->p_block);

		destroy_transaction(p_txn);

		if (ret == SERVICE_NO_ERROR)
			return MHD_HTTP_CREATED; }

		return MHD_HTTP_BAD_GATEWAY;
	}
	destroy_transaction(p_txn);

	return MHD_HTTP_BAD_GATEWAY;
}


/**	 Execute an http DELETE of a block using the block API.

	\param q_state The structure containing the parts of the url successfully parsed.

	\return			  true if successful, log(LOG_MISS, "further details") for errors.

	This function is **only** called after a successfull parse() of an HTTP_DELETE query. It is not private because it is called for the
callback, but it is not intended for any other context.

Internals
---------

It supports any successful HTTP_PUT syntax, that is:

APPLY_NOTHING: With or without node, mandatory base and entity, with of without a key.
APPLY_URL: With or without node and just a base.

In all cases, calls with a node (it can only be l_node) q_state.url contains exactly what has to be forwarded.

*/
MHD_StatusCode Api::http_delete(HttpQueryState &q_state) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	if (q_state.l_node[0] != 0) {
		if (p_channels->forward_del(q_state.l_node, q_state.url) == SERVICE_NO_ERROR)
			return MHD_HTTP_OK;

		return MHD_HTTP_NOT_FOUND;
	}

	pContainer p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

	if (p_container == nullptr)
		return MHD_HTTP_SERVICE_UNAVAILABLE;

	switch (q_state.apply) {
	case APPLY_NOTHING: {
		Locator loc;

		memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

		if (p_container->remove(loc) == SERVICE_NO_ERROR)
			return MHD_HTTP_OK; }

		return MHD_HTTP_NOT_FOUND;

	case APPLY_URL:
		if (p_container->remove((pChar) q_state.url) == SERVICE_NO_ERROR)
			return MHD_HTTP_OK;

		return MHD_HTTP_NOT_FOUND;
	}

	return MHD_HTTP_NOT_FOUND;
}


/** Execute a get block using the instrumental API.

	\param response	A valid (or error) MHD_Response pointer with the resource. It will only be used on success.
	\param q_state	The structure containing the parts of the url successfully parsed.

	\return			MHD_HTTP_OK if successful, or a valid http status error.

	This function is **only** called after a successfull parse() of HTTP_GET and HTTP_HEAD queries. It is not private because it is called
for the callback, but it is not intended for any other context.

Internals
---------

It supports, basically everything, which is, all apply in many versions:

APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, APPLY_FUNCT_CONST, APPLY_FILTER, APPLY_FILT_CONST, APPLY_RAW, APPLY_TEXT,
APPLY_ASSIGN_NOTHING, APPLY_ASSIGN_NAME, APPLY_ASSIGN_URL, APPLY_ASSIGN_FUNCTION, APPLY_ASSIGN_FUNCT_CONST, APPLY_ASSIGN_FILTER,
APPLY_ASSIGN_FILT_CONST, APPLY_ASSIGN_RAW, APPLY_ASSIGN_TEXT, APPLY_ASSIGN_CONST, APPLY_NEW_ENTITY, APPLY_GET_ATTRIBUTE,
APPLY_SET_ATTRIBUTE and APPLY_JAZZ_INFO

To simplify, this top level function decomposes the logic into smaller parts.

*/
MHD_StatusCode Api::http_get(pMHD_Response &response, HttpQueryState &q_state) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	char		 buffer_1k[1024];
	Locator		 loc;
	pTransaction p_txn, p_aux;
	pContainer	 p_container;
	int			 ret, size;
	pChar		 p_att;

	switch (q_state.apply) {
	case APPLY_NOTHING ... APPLY_TEXT:
		if (q_state.l_node[0] != 0)
			ret = p_channels->forward_get(p_txn, q_state.l_node, q_state.url);
		else
			ret = get_left_local(p_txn, q_state);

		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_NOT_FOUND;

		size	 = (p_txn->p_block->cell_type & 0xff)*p_txn->p_block->size;
		response = MHD_create_response_from_buffer(size, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);

		p_txn->p_owner->destroy_transaction(p_txn);

		return MHD_HTTP_OK;

	case APPLY_ASSIGN_NOTHING ... APPLY_ASSIGN_CONST:
		if (q_state.r_node[0] != 0)
			ret = get_right_remote(p_txn, q_state);
		else
			ret = get_right_local(p_txn, q_state);

		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_NOT_FOUND;

		if (q_state.l_node[0] != 0) {
			sprintf(buffer_1k, "//%s/%s/%s", q_state.base, q_state.entity, q_state.key);

			ret = p_channels->forward_put(q_state.l_node, buffer_1k, p_txn->p_block);
		} else
			ret = put_left_local(q_state, p_txn->p_block);

		destroy_transaction(p_txn);

		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_BAD_GATEWAY;

		response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

		return MHD_HTTP_OK;

	case APPLY_NEW_ENTITY:
		if (q_state.l_node[0] != 0) {
			ret = p_channels->forward_get(p_txn, q_state.l_node, q_state.url);
			if (ret == SERVICE_NO_ERROR)
				p_channels->destroy_transaction(p_txn); }
		else {
			p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

			if (p_container == nullptr)
				return MHD_HTTP_SERVICE_UNAVAILABLE;

			if (q_state.url[0] == 0) {
				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

				ret = p_container->new_entity(loc);
			} else
				ret = p_container->new_entity((pChar) q_state.url);
		}
		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_BAD_REQUEST;

		response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

		return MHD_HTTP_OK;

	case APPLY_GET_ATTRIBUTE:
		if (q_state.l_node[0] != 0) {
			ret = p_channels->forward_get(p_txn, q_state.l_node, q_state.url);

			if (ret != SERVICE_NO_ERROR)
				return MHD_HTTP_NOT_FOUND;

			size	 = (p_txn->p_block->cell_type & 0xff)*p_txn->p_block->size;
			response = MHD_create_response_from_buffer(size, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);

			p_channels->destroy_transaction(p_txn);

			return MHD_HTTP_OK;
		}
		p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

		if (p_container == nullptr)
			return MHD_HTTP_SERVICE_UNAVAILABLE;

		memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

		if (p_container->get(p_txn, loc) != SERVICE_NO_ERROR)
			return MHD_HTTP_NOT_FOUND;

		p_att = p_txn->p_block->get_attribute(q_state.r_value.attribute);

		if (p_att == nullptr) {
			p_container->destroy_transaction(p_txn);

			return MHD_HTTP_NOT_FOUND;
		}
		response = MHD_create_response_from_buffer(strlen(p_att), p_att, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");

		p_container->destroy_transaction(p_txn);

		return MHD_HTTP_OK;

	case APPLY_SET_ATTRIBUTE:
		if (q_state.l_node[0] != 0)
			ret = p_channels->forward_get(p_txn, q_state.l_node, q_state.url);
		else {
			p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

			if (p_container == nullptr)
				return MHD_HTTP_SERVICE_UNAVAILABLE;

			memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);

			if (p_container->get(p_aux, loc) != SERVICE_NO_ERROR)
				return MHD_HTTP_NOT_FOUND;

			AttributeMap atts;
			p_aux->p_block->get_attributes(&atts);

			atts[q_state.r_value.attribute] = q_state.url;

			if (new_block(p_txn, p_aux->p_block, (pBlock) nullptr, &atts) != SERVICE_NO_ERROR) {
				p_container->destroy_transaction(p_aux);

				return MHD_HTTP_BAD_REQUEST;
			}
			p_container->destroy_transaction(p_aux);

			ret = p_container->put(loc, p_txn->p_block);

			destroy_transaction(p_txn);
		}
		if (ret != SERVICE_NO_ERROR)
			return MHD_HTTP_BAD_REQUEST;

		response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

		return MHD_HTTP_OK;

	case APPLY_JAZZ_INFO:
#ifdef DEBUG
		std::string st("DEBUG");
#else
		std::string st("RELEASE");
#endif
		struct utsname unn;
		uname(&unn);

		int my_idx	 = p_channels->jazz_node_my_index;
		int my_port	 = p_channels->jazz_node_port[my_idx];
		int nn_nodes = p_channels->jazz_node_name.size();

		std::string my_name = p_channels->jazz_node_name[my_idx];
		std::string my_ip	= p_channels->jazz_node_ip[my_idx];

		sprintf(buffer_1k, "Jazz\n\n version : %s\n build   : %s\n artifact: %s\n jazznode: %s (%s:%d) (%d of %d)\n "
				"sysname : %s\n hostname: %s\n kernel\x20 : %s\n sysvers : %s\n machine : %s",
				JAZZ_VERSION, st.c_str(), LINUX_PLATFORM, my_name.c_str(), my_ip.c_str(), my_port, my_idx, nn_nodes,
				unn.sysname, unn.nodename, unn.release, unn.version, unn.machine);

		response = MHD_create_response_from_buffer(strlen(buffer_1k), buffer_1k, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");

		return MHD_HTTP_OK;
	}
	return MHD_HTTP_BAD_REQUEST;
}


/** Push a copy of all the files in the path (searched recursively) to the Persisted database "static" and index their names
to be found by get_static().

It also assigns attributes:
- BLOCK_ATTRIB_URL == same relative path after the root path.
- BLOCK_ATTRIB_MIMETYPE guessed from the file extension (html, js, png, etc.)
- BLOCK_ATTRIB_LANGUAGE == en-us

	\param p_base_path		The path to the tree of webpage statics.
	\param p_relative_path	The relative path that becomes the url with a file name added, starting with /.
	\param rec_level		The level of recursion (from 0 to MAX_RECURSE_LEVEL_ON_STATICS)

	\return		Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::load_statics(pChar p_base_path, pChar p_relative_path, int rec_level) {

	if (!p_persisted->is_running()) {
		log(LOG_MISS, "Api::load_statics(): Skipped because Persistence is not running.");

		return SERVICE_NO_ERROR;
	}

	if (rec_level > MAX_RECURSE_LEVEL_ON_STATICS)
		return SERVICE_ERROR_TOO_DEEP;

	DIR *dir;
	char root_dir[1024];
	sprintf(root_dir, "%s%s", p_base_path, p_relative_path);

	if ((dir = opendir(root_dir)) != nullptr) {

		if (rec_level == 0 && !p_persisted->dbi_exists((pChar) "www")) {
			Locator loc = {"lmdb", "www", ""};
			int ret;
			if ((ret = p_persisted->new_entity(loc)) != SERVICE_NO_ERROR) {
				log(LOG_ERROR, "Api::load_statics(): Failed to create www database.");

		  		closedir(dir);

				return ret;
			}
		}

		struct dirent *ent;

		int		 file_num = 1;
		uint64_t dir_hash = MurmurHash64A(&root_dir, strlen(root_dir));

		while ((ent = readdir(dir)) != nullptr) {	// cppcheck-suppress readdirCalled ; cppcheck is wrong! readdir_r is deprecated and
													// readdir() (3) is thread safe.
			if (ent->d_type == DT_REG) {
				char fn[1024];
				int ret = snprintf(fn, 1024, "//file/%s%s", root_dir, ent->d_name);

				if (ret < 0 || ret >= 1024) {
					log(LOG_ERROR, "Api::load_statics(): File path/name too long.");

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}

				pTransaction p_base, p_txn;

				ret = p_channels->get(p_base, (pChar) &fn);

				if (ret != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "Api::load_statics(): p_channels->get() failed.");

			  		closedir(dir);

					return ret;
				}

				AttributeMap atts;
				p_base->p_block->get_attributes(&atts);

				sprintf(fn, "%s%s", p_relative_path, ent->d_name);

				atts[BLOCK_ATTRIB_URL]		= fn;
				atts[BLOCK_ATTRIB_LANGUAGE] = "en-us";

				for (int i = 0; i < 1024; i++) {
					if (fn[i] == 0)
						break;

					fn[i] = tolower(fn[i]);
				}
				pChar p_ext = strrchr(fn, '.');

				char mime_type[40] = {"application/octet-stream"};

				if (p_ext != nullptr) {
					if (strcmp(p_ext, ".htm") == 0 || strcmp(p_ext, ".html") == 0)
						strcpy(mime_type, "text/html");
					else if (strcmp(p_ext, ".css") == 0)
						strcpy(mime_type, "text/css");
					else if (strcmp(p_ext, ".png") == 0)
						strcpy(mime_type, "image/png");
					else if (strcmp(p_ext, ".js") == 0)
						strcpy(mime_type, "application/javascript");
					else if (strcmp(p_ext, ".jpg") == 0 || strcmp(p_ext, ".jpeg") == 0)
						strcpy(mime_type, "image/jpeg");
					else if (strcmp(p_ext, ".gif") == 0)
						strcpy(mime_type, "image/gif");
					else if (strcmp(p_ext, ".ico") == 0)
						strcpy(mime_type, "image/x-icon");
					else if (strcmp(p_ext, ".md") == 0 || strcmp(p_ext, ".txt") == 0)
						strcpy(mime_type, "text/plain; charset=utf-8");
					else if (strcmp(p_ext, ".json") == 0)
						strcpy(mime_type, "application/json");
					else if (strcmp(p_ext, ".mp4") == 0)
						strcpy(mime_type, "video/mp4");
					else if (strcmp(p_ext, ".pdf") == 0)
						strcpy(mime_type, "application/pdf");
					else if (strcmp(p_ext, ".xml") == 0)
						strcpy(mime_type, "application/xml");
				}
				atts[BLOCK_ATTRIB_MIMETYPE] = mime_type;

				if (new_block(p_txn, p_base->p_block, (pBlock) nullptr, &atts) != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "Api::load_statics(): new_block() with attributes failed.");

					p_channels->destroy_transaction(p_base);

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}
				p_channels->destroy_transaction(p_base);

				Locator loc = {"lmdb", "www", ""};

				sprintf(loc.key, "blk%lx_%d", dir_hash, file_num++);

				ret = p_persisted->put(loc, p_txn->p_block);

				if (ret == SERVICE_NO_ERROR)
					www[p_txn->p_block->get_attribute(BLOCK_ATTRIB_URL)] = loc.key;

				destroy_transaction(p_txn);

				if (ret != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "Api::load_statics(): p_persisted->put() failed.");

			  		closedir(dir);

					return ret;
				}
				log_printf(LOG_INFO, "www static %s loaded as %s", mime_type, fn);
			} else if (ent->d_type == DT_DIR && ent->d_name[0] != '.') {
				char next_relative_path[1024];
				int ret = snprintf(next_relative_path, 1024, "%s%s/", p_relative_path, ent->d_name);

				if (ret < 0 || ret >= 1024) {
					log(LOG_ERROR, "Api::load_statics(): nested path too long.");

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}

				ret = load_statics(p_base_path, (pChar) &next_relative_path, rec_level + 1);

				if (ret != SERVICE_NO_ERROR) {
			  		closedir(dir);

					return ret;
				}
			}
		}
  		closedir(dir);
	}

	return SERVICE_NO_ERROR;
}


/** Copy while percent-decoding a string into a buffer. Only RFC 3986 section 2.3 and RFC 3986 section 2.2 characters accepted.

	\param p_buff	 A buffer to store the result.
	\param buff_size The size of the output buffer (ending zero included).
	\param p_url	 The input string.

	\return			'true' if successful. Possible errors are wrong %-syntax, wrong char range or output buffer too small.

See https://en.wikipedia.org/wiki/Percent-encoding This is utf-8 compatible, utf-8 chars are just percent encoded one byte at a time.
*/
bool Api::expand_url_encoded(pChar p_buff, int buff_size, pChar p_url) {

	if (*(p_url++) != '&')
		return false;

	pChar p_end = p_url;

	p_end += strlen(p_url) - 1;

	if (*p_end != ';' && *p_end != ']' && *p_end != ')')
		return false;

	while (buff_size-- > 0) {
		if (p_url == p_end) {
			*(p_buff) = 0;

			return true;
		}
		switch (char ch = *(p_url++)) {
		case '!' ... '$':
		case '&' ... '/':
		case '0' ... '9':
		case ':':
		case ';':
		case '=':
		case '?':
		case '@':
		case 'A' ... 'Z':
		case '[':
		case ']':
		case '_':
		case 'a' ... 'z':
		case '~':
			*(p_buff++) = ch;
			break;

		case '%': {
			if (p_url == p_end)
				return false;

			int xhi = from_hex(*(p_url++));

			if (p_url == p_end)
				return false;

			int xlo = from_hex(*(p_url++));

			*(p_buff++) = (xhi << 4) + xlo;

			break;
		}
		default:
			return false;
		}
	}

	return false;
}


/** Copy the string "as-is" (without percent-decoding) a string into a buffer.

	\param p_buff	 A buffer to store the result. This first char must be a zero on call or it will not write anything, just check for
					 the return code and return. (This is a trick to avoid overriding the buffer in forward call. This function is an
					 internal part of parse().)
	\param buff_size The size of the output buffer (ending zero included).
	\param p_url	 The input string.
	\param p_base	 (optional) Prefix with a base to be prefixed

	\return			 RET_MV_CONST_FAILED on error (buffer sizes or start and final characters), RET_MV_CONST_NOTHING normal moving or
					 RET_MV_CONST_NEW_ENTITY there is a ";.new" ending and no errors.


Note: This replaces expand_url_encoded() since the string passed to pares is already %-decoded by libmicrohttpd.
*/
int Api::move_const(pChar p_buff, int buff_size, pChar p_url, pChar p_base) {

	if (*(p_url++) != '&')
		return RET_MV_CONST_FAILED;

	int url_len = strlen(p_url) - 1;

	pChar p_end = p_url + url_len;

	int ret = RET_MV_CONST_NOTHING;

	if (*p_end == 'w') {
		if ((*(--p_end) != 'e') || (*(--p_end) != 'n') || (*(--p_end) != '.') || (*(--p_end) != ';'))
			return RET_MV_CONST_FAILED;

		ret		 = RET_MV_CONST_NEW_ENTITY;
		url_len -= 4;
	}

	if (*p_end != ';' && *p_end != ']' && *p_end != ')')
		return RET_MV_CONST_FAILED;

	if (*p_buff != 0)
		return ret;

	if (p_base != nullptr) {
		if (buff_size < 4 + SHORT_NAME_SIZE)
			return RET_MV_CONST_FAILED;

		*(p_buff++) = '/';
		*(p_buff++) = '/';

		buff_size -= 2;

		for (int i = 0; i < SHORT_NAME_SIZE; i++) {
			char c = *(p_base++);

			buff_size--;

			if (c == 0) {
				*(p_buff++) = '/';
				break;
			}
			*(p_buff++) = c;
		}
	}

	if (url_len >= buff_size)
		return RET_MV_CONST_FAILED;

	memcpy(p_buff, p_url, url_len);

	p_buff += url_len;

	*(p_buff) = 0;

	return ret;
}


/** Parse a simple //base/entity/key string (Used inside the main Api.parse()).

	\param loc	 A Locator to store the result (that will be left in undetermined on error).
	\param p_url The input string.

	\return		 `true` if successful.
*/
bool Api::parse_locator(Locator &loc, pChar p_url) {

	int buf_size, state = PSTATE_INITIAL;
	pChar p_out;

	loc.p_extra = nullptr;

	p_url++;	// parse_locator() is only called after checking the trailing /, this skips the first / to set state to PSTATE_INITIAL

	while (true) {
		unsigned char cursor;

		cursor = *(p_url++);
		state = parser_state_switch[state].next[cursor];

		switch (state) {
		case PSTATE_BASE0:
			p_out	 = (pChar) &loc.base;
			buf_size = SHORT_NAME_SIZE - 1;

			break;

		case PSTATE_ENTITY0:
			p_out	 = (pChar) &loc.entity;
			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_KEY0:
			p_out	 = (pChar) &loc.key;
			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_IN_BASE:
		case PSTATE_IN_ENTITY:
		case PSTATE_IN_KEY:
			if (buf_size-- == 0)
				return false;

			*(p_out++) = cursor;
			*(p_out)   = 0;

			break;

		case PSTATE_ENT_SWITCH:
			loc.key[0] = 0;

		case PSTATE_KEY_SWITCH:
			if (p_out == (pChar) loc.key)
				return false;

			switch (cursor) {
			case 0:
				return true;
			case ')':
			case ']':
				return *(p_url) == 0;
			}
			return false;

		default:
			return false;
		}
	}
}


/** Creates a block from a constant read in the URL.

	\param p_txn	  A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					  Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction() when done.
	\param p_const	  The constant.
	\param make_tuple Insert it into a Tuple with "input" and "result".

	\return			'true' if successful.
*/
bool Api::block_from_const(pTransaction &p_txn, pChar p_const, bool make_tuple) {

	p_txn = nullptr;

	int size = strlen(p_const);
	int dim[MAX_TENSOR_RANK] = {size, 0, 0, 0, 0, 0};

	pTransaction p_text, p_tensor, p_result;

	if (new_block(p_text, CELL_TYPE_BYTE, (int *) &dim, FILL_NEW_DONT_FILL) != SERVICE_NO_ERROR)
		return false;

	memcpy(&p_text->p_block->tensor, p_const, size);

	if (new_block(p_tensor, p_text->p_block, CELL_TYPE_UNDEFINED) == SERVICE_NO_ERROR)
		destroy_transaction(p_text);
	else
		p_tensor = p_text;

	if (!make_tuple) {
		p_txn = p_tensor;

		return true;
	}

	dim[0] = RESULT_BUFFER_SIZE;
	if (new_block(p_result, CELL_TYPE_BYTE, (int *) &dim, FILL_NEW_WITH_ZERO) !=  SERVICE_NO_ERROR) {
		destroy_transaction(p_tensor);

		return false;
	}

	StaticBlockHeader hea[2];
	Name			  names[2]	 = {"input", "result"};
	pBlock			  p_block[2] = {p_tensor->p_block, p_result->p_block};

	memcpy(&hea[0], p_block[0], sizeof(StaticBlockHeader));
	memcpy(&hea[1], p_block[1], sizeof(StaticBlockHeader));

	p_block[0]->get_dimensions(hea[0].range.dim);
	p_block[1]->get_dimensions(hea[1].range.dim);

	int ret = new_block(p_txn, 2, hea, names, p_block);

	destroy_transaction(p_tensor);
	destroy_transaction(p_result);

	return ret == SERVICE_NO_ERROR;
}

#ifdef CATCH_TEST

Bebop  BOP	 (&jazz_elements::LOGGER, &jazz_elements::CONFIG);
Agency EPI	 (&jazz_elements::LOGGER, &jazz_elements::CONFIG);
Api	   TT_API(&jazz_elements::LOGGER, &jazz_elements::CONFIG, &CHN, &VOL, &PER, &BOP, &EPI);

#endif

} // namespace jazz_main

#ifdef CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

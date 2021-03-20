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
	 Parser grammar definition
--------------------------------------------------- */

#define REX_ALL_CLOSERS		"[\\0\\)\\]]"
#define REX_ALL_SPACES		"[ \\t]"
#define REX_COMMA			"[,]"
#define REX_DOT				"[\\.]"
#define REX_DOUBLEQUOTE		"[\"]"
#define REX_HEX_CHAR		"[0-9A-Fa-f]"
#define REX_NAME_FIRST		"[A-Za-z]"
#define REX_NAME_ANY		"[A-Za-z0-9_]"
#define REX_NON_URLENCODED	"[ \tA-Za-z0-9_~!#$&',/:;=@\\[\\]\\?\\(\\)\\*\\+\\-\\.]"
#define REX_PARENTHESIS		"[\\(]"
#define REX_PERCENT			"[%]"
#define REX_SLASH			"[/]"
#define REX_SLICE			"[\\[]"


StateTransitions state_tr = {
	{PSTATE_INITIAL,		PSTATE_INITIAL,			REX_ALL_SPACES},

	{PSTATE_INITIAL,		PSTATE_CONST_INT,		"[0-9\\-]"},

	{PSTATE_CONST_INT,		PSTATE_CONST_INT,		"[0-9]"},
	{PSTATE_CONST_INT,		PSTATE_CONST_SEP_INT,	REX_COMMA},
	{PSTATE_CONST_INT,		PSTATE_CONST_END_INT,	REX_ALL_CLOSERS},

	{PSTATE_CONST_SEP_INT,	PSTATE_CONST_SEP_INT,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_INT,	PSTATE_CONST_INT,		"[0-9\\-]"},

	{PSTATE_CONST_INT,		PSTATE_CONST_REAL,		"[e\\.]"},

	{PSTATE_CONST_REAL,		PSTATE_CONST_REAL,		"[e0-9\\-\\.]"},
	{PSTATE_CONST_REAL,		PSTATE_CONST_SEP_REAL,	REX_COMMA},
	{PSTATE_CONST_REAL,		PSTATE_CONST_END_REAL,	REX_ALL_CLOSERS},

	{PSTATE_CONST_SEP_REAL,	PSTATE_CONST_SEP_REAL,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_REAL,	PSTATE_CONST_REAL,		"[0-9\\-]"},

	{PSTATE_INITIAL,		PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},

	{PSTATE_CONST_STR0,		PSTATE_CONST_STRn,		REX_NON_URLENCODED},
	{PSTATE_CONST_STRn,		PSTATE_CONST_STRn,		REX_NON_URLENCODED},
	{PSTATE_CONST_STRn,		PSTATE_CONST_SEP_STR0,	REX_DOUBLEQUOTE},
	{PSTATE_CONST_STRn,		PSTATE_CONST_STR_ENC0,	REX_PERCENT},

	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_SEP_STR0,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_SEP_STRn,	REX_COMMA},
	{PSTATE_CONST_SEP_STR0,	PSTATE_CONST_END_STR,	REX_ALL_CLOSERS},

	{PSTATE_CONST_SEP_STRn,	PSTATE_CONST_SEP_STRn,	REX_ALL_SPACES},
	{PSTATE_CONST_SEP_STRn,	PSTATE_CONST_STR0,		REX_DOUBLEQUOTE},

	{PSTATE_CONST_STR_ENC0,	PSTATE_CONST_STR_ENC1,	REX_HEX_CHAR},
	{PSTATE_CONST_STR_ENC1,	PSTATE_CONST_STR_ENC2,	REX_HEX_CHAR},
	{PSTATE_CONST_STR_ENC2,	PSTATE_CONST_STR_ENC0,	REX_PERCENT},
	{PSTATE_CONST_STR_ENC2,	PSTATE_CONST_STRn,		REX_NON_URLENCODED},

	{PSTATE_INITIAL,		PSTATE_CONST_BASE_NAM,	REX_NAME_FIRST},

	{PSTATE_CONST_BASE_NAM,	PSTATE_CONST_BASE_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_BASE_NAM,	PSTATE_CONST_SLASH,		REX_SLASH},

	{PSTATE_CONST_CNTN_NAM,	PSTATE_CONST_CNTN_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_CNTN_NAM,	PSTATE_CONST_SLASH,		REX_SLASH},

	{PSTATE_CONST_BLCK_NAM,	PSTATE_CONST_BLCK_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_BLCK_NAM,	PSTATE_CONST_SLICE,		REX_SLICE},
	{PSTATE_CONST_BLCK_NAM,	PSTATE_CONST_CALL,		REX_PARENTHESIS},
	{PSTATE_CONST_BLCK_NAM,	PSTATE_CONST_DOT,		REX_DOT},

	{PSTATE_CONST_KIND_NAM,	PSTATE_CONST_KIND_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_KIND_NAM,	aaa,		aaa},

	{PSTATE_CONST_ITEM_NAM,	PSTATE_CONST_ITEM_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_ITEM_NAM,	aaa,		aaa},

	{PSTATE_CONST_CNTR_NAM,	PSTATE_CONST_CNTR_NAM,	REX_NAME_ANY},
	{PSTATE_CONST_CNTR_NAM,	aaa,		aaa},

#define PSTATE_CONST_SLICE				 21		///< Parser state: Parsing slicer "["
#define PSTATE_CONST_CALL				 22		///< Parser state: Parsing call "("
#define PSTATE_CONST_SLASH				 23		///< Parser state: Parsing slash "/"
#define PSTATE_CONST_DOT				 24		///< Parser state: Parsing dor "."


	{MAX_NUM_PSTATES}
};


StateSwitch parser_state;

/*	-----------------------------------------------
	 Api : I m p l e m e n t a t i o n
--------------------------------------------------- */

Api::Api(pLogger	 a_logger,
		 pConfigFile a_config,
		 pVolatile	 a_volatile,
		 pRemote	 a_remote,
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

	memset(&parser_state, -1, sizeof(parser_state));

	StateTransition *p_trans = reinterpret_cast<StateTransition *>(&state_tr);
	while (true) {
		if (p_trans->from == MAX_NUM_PSTATES)
			break;

		NextStateLUT *p_next = &parser_state.next[p_trans->from];

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

	p_volatile	= a_volatile;
	p_remote	= a_remote;
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
	p_volatile->base_names(base);
	p_remote->base_names(base);
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
int	Api::return_error_message (struct MHD_Connection *connection, int http_status)
{
	Answer answer;

	sprintf(answer.text, "<html><body><h1><br/><br/>Http error : %d.</h1></body></html>", http_status);

	struct MHD_Response *response = MHD_create_response_from_buffer (strlen(answer.text), answer.text, MHD_RESPMEM_MUST_COPY);

	int ret = MHD_queue_response (connection, http_status, response);

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
//TODO: Implement Api::remove()

	return false;
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
//TODO: Implement Api::http_get()

	return false;
}


/** Implements .get() interface to the Api Container.

The Api is a Container (the root class) and includes all the one shot functionality (6 new block methods) and a deque to lock blocks.
It does not implement get(), just defines the interface to be inherited. Api needs this extra method for keeping temporary blocks locked
over possibly more than one http callback query (in uploads) and used by different threads.

This is an implementation of the interface using block names and locators for the only purpose of storing Api-owned Blocks and have some
mechanism to recall the same blocks across http PUT queries.

	\param p_keeper	A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
					BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_what	An L_value (locator) of the block being uploaded, that will be moved to the appropriate Container when upload is done.

	\return			Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::get(pBlockKeeper *p_keeper, pLocator p_what)
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

	return PARSE_NOT_IMPLEMENTED;
}


/** This is a private submethod of parse() doing the outer level recursively. See the documentation of Api.parse() for reference.
*/
StatusCode Api::_parse_recurse (pChar &p_url, int method, L_value &l_value, R_value &r_value, bool execution, int rec_level)
{
//TODO: Implement Api::_parse_recurse()

	return PARSE_NOT_IMPLEMENTED;
}


/** This is a private submethod of parse() doing the kernel of parsing at a constant level of recursion, without doing the
execution part (done inside _parse_recurse()). See the documentation of Api.parse() for reference.
*/
StatusCode Api::_parse_exec_stage(pChar	&p_url, int method, L_value &l_value, R_value &r_value)
{
//TODO: Implement Api::_parse_exec_stage()

	return PARSE_NOT_IMPLEMENTED;
}


/** This is a private submethod of parse() parsing constants for syntax checking and retrieving the metadata.

It returns a BlockHeader that is necessary to allocate a block calling _parse_const_data() with it.

	\param p_url A pointer to the first character of the constant.
	\param hea	 A BlockHeader passed by reference that will be filled of success, possibly partially on failure.

	\return		 Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::_parse_const_meta(pChar &p_url, BlockHeader &hea)
{
//TODO: Implement Api::_parse_const_meta()

	return false;
}


/** This is a private submethod of parse() creating blocks with the data of a constant previously checked by _parse_const_meta()

It returns a locked (one shot) block that must be unlocked in the same http query.

	\param p_url	A pointer to the first character of the constant.
	\param hea		A BlockHeader created in a successful _parse_const_meta.
	\param p_keeper	A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
					BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.

	\return			Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode Api::_parse_const_data(pChar &p_url, BlockHeader &hea, pBlockKeeper *p_keeper)
{
//TODO: Implement Api::_parse_const_data()

	return PARSE_NOT_IMPLEMENTED;
}

} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

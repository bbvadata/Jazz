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


#include <curl/curl.h>
#include <zmq.h>
#include <microhttpd.h>


#include "src/jazz_elements/channel.h"


namespace jazz_elements
{


int client_test(void) {		// cppcheck-suppress unusedFunction

	printf("Connecting to hello world server…\n");
	void *context = zmq_ctx_new();
	void *requester = zmq_socket(context, ZMQ_REQ);
	zmq_connect(requester, "tcp://localhost:5555");

	int request_nbr;
	for (request_nbr = 0; request_nbr != 10; request_nbr++) {
		char buffer [10];
		printf("Sending Hello %d…\n", request_nbr);
		zmq_send(requester, "Hello", 5, 0);
		zmq_recv(requester, buffer, 10, 0);
		printf("Received World %d\n", request_nbr);
	}
	zmq_close(requester);
	zmq_ctx_destroy(context);

	return 0;
}


/** \brief A test write callback (libCURL stuff to be modified).

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
size_t write_callback(char * ptr, size_t size, size_t nmemb, void *userdata) {
	size = size*nmemb;

	// if ((uintptr_t) userdata == 0xbaaadc0ffee)

	printf("\n");

	for (int i = 0; i < size; i++)
		printf("%c", ptr[i]);

	printf("\n\n");

	return size;
}


/** \brief A remote testing point (libCURL stuff to be modified).

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
bool remote_testing_point() {			// cppcheck-suppress unusedFunction
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "http://...");
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) 0xbaaadc0ffee);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		return false;
	}

	curl_easy_cleanup(curl);

	return true;
}

/*	-----------------------------------------------
	 Channels : I m p l e m e n t a t i o n
--------------------------------------------------- */

Channels::Channels(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


Channels::~Channels() { destroy_container(); }


/** Reads config variables and sets jazz_node_* public variables.
*/
StatusCode Channels::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	if (!get_conf_key("FILESYSTEM_ROOT", filesystem_root)) {
		log(LOG_ERROR, "Channels::start() failed to find FILESYSTEM_ROOT");

		return EXIT_FAILURE;
	}

	std::string my_name;

	if (!get_conf_key("JAZZ_NODE_MY_NAME", my_name)) {
		log(LOG_ERROR, "Channels::start() failed to find JAZZ_NODE_MY_NAME");

		return EXIT_FAILURE;
	}

	jazz_node_cluster_size =  0;
	jazz_node_my_index	   = -1;

	std::string s;
	char key[40];

	for (int i = 1;; i++) {
		sprintf(key, "JAZZ_NODE_NAME_%i", i);

		if (!get_conf_key(key, s))
			break;

		jazz_node_name[i] = s;

		if (my_name == s)
			jazz_node_my_index = i;

		sprintf(key, "JAZZ_NODE_IP_%i", i);

		if (!get_conf_key(key, s)) {
			log_printf(LOG_ERROR, "Channels::start() failed to find %s", key);

			return EXIT_FAILURE;
		}

		jazz_node_ip[i] = s;

		sprintf(key, "JAZZ_NODE_PORT_%i", i);

		int port;

		if (!get_conf_key(key, port)) {
			log_printf(LOG_ERROR, "Channels::start() failed to find %s", key);

			return EXIT_FAILURE;
		}

		jazz_node_port[i] = port;

		jazz_node_cluster_size++;
	}

	if (jazz_node_my_index < 0) {
		log(LOG_ERROR, "Channels::start() failed to find JAZZ_NODE_MY_NAME in JAZZ_NODE_NAME_*");

		return EXIT_FAILURE;
	}

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Channels::shut_down() {

//TODO: Implement the Channels-specific shut_down()

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** The parser: An overwrite of the same method in Container to handle long and realistic file names, URL quirks, etc.

	\param result	A Locator to contained the parsed result on success. (Undefined content on error.) It uses the p_extra-> pointer
					in the Locator structure to match an ExtraLocator structure containing the long paths. The structure is owned by
					Channels and has to be released using destroy_extra_locator() which happens automatically when you use the native API.
	\param p_what	Some string to be parsed. (See Syntax below) E.g. //http/http://www.google.com?xy or //http[USERNAME:me]http://ibm.com

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: Because Locators are being replaced by ExtraLocator, all calls to as_locator() **must** be used **just once** by the Native API.
If an ExtraLocator returned by this is not used, you must explicitely call destroy_extra_locator().

Syntax
------

The general syntax is: //{base}/{url} or //{base}[]{url}

The parser will accept anything as the URL or file name, without imposing on character ranges, with the exception of the (optional)
initial []. If the **first** character is a [, a closing ] will be found and anything inside the [] are the **quirks**, the rest is the
URL (or file name).

The quirks have the format [USERNAME:xxx,USERPW:xxx,COOKIEFILE://base/entity/key,COOKIEJAR://base/entity/key] When any of these fields is
given it will populate the corresponding field of the ExtraLocator which will otherwise be NULL.
Both the COOKIEFILE (cookies emitted) and COOKIEJAR (cookies accepted) will be files with short names in a format good for
curl_easy_setopt() https://everything.curl.dev/libcurl-http/cookies The USERNAME is https://curl.se/libcurl/c/CURLOPT_USERNAME.html and
the USERPW https://curl.se/libcurl/c/CURLOPT_USERPWD.html

*/
StatusCode Channels::as_locator(Locator &result, pChar p_what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;
}


/** Native (Channels) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The data read from the endpoint will be stored as a rank == 1 CELL_TYPE_BYTE
					for all bases except "bash" shell output is returned as a rank == 1 CELL_TYPE_STRING
	\param what		Some Locator to the endpoint compiled by Channels::as_locator() that can only be used once.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **what**.
*/
StatusCode Channels::get(pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Channels) interface **selection of rows in a Block** retrieval.

**NOTE**: This is NOT supported by Channels since the blocks returned by endpoints would need other types for filtering. It does
call destroy_extra_locator() on **what** to avoid possible leakage and returns SERVICE_ERROR_WRONG_ARGUMENTS.
*/
StatusCode Channels::get(pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

	destroy_extra_locator(what);

	return SERVICE_ERROR_WRONG_ARGUMENTS;
}


/** Native (Channels) interface **selection of a tensor in a Tuple** retrieval.

**NOTE**: This is NOT supported by Channels since the blocks returned by endpoints cannot be Tuples. It does
call destroy_extra_locator() on **what** to avoid possible leakage and returns SERVICE_ERROR_WRONG_ARGUMENTS.
*/
StatusCode Channels::get(pTransaction &p_txn, Locator &what, pChar name) {

	destroy_extra_locator(what);

	return SERVICE_ERROR_WRONG_ARGUMENTS;
}


/** Native (Channels) interface **metadata of a Block** retrieval.

	\param hea	A StaticBlockHeader structure that will receive the metadata.
	\param what	Some Locator to the endpoint compiled by Channels::as_locator() that can only be used once.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Note that, unlike in other containers, this does the whole .get() just to return the header which is predictable except for the
data size. For the base "file", this will just verify if the file exists and return its size in a StaticBlockHeader efficiently,
for any other base, it makes sense to use get() instead to avoid downloading the same data twice.

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **what**.
*/
StatusCode Channels::header(StaticBlockHeader &hea, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Channels) interface **metadata of a Block** retrieval.

**NOTE**: This is NOT supported by Channels since the blocks returned by endpoints cannot be Tuples. It does
call destroy_extra_locator() on **what** to avoid possible leakage and returns SERVICE_ERROR_WRONG_ARGUMENTS.
*/
StatusCode Channels::header(pTransaction &p_txn, Locator &what) {

	destroy_extra_locator(what);

	return SERVICE_ERROR_WRONG_ARGUMENTS;
}


/** Native (Channels) interface for **Block storing**

	\param where	Some Locator to the destination endpoint compiled by Channels::as_locator() that can only be used once.
	\param p_block	The Block to be written/sent by Channels.
	\param mode		Some writing restriction that depends on the base. WRITE_ONLY_IF_EXISTS and WRITE_ONLY_IF_NOT_EXISTS can only be used
					on **file**. WRITE_TENSOR_DATA should be used as default, otherwise the metadata will also be written/sent.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

This does not support the **bash** base. For the **tcp** base, it will normally be followed by a get() to the same channel to get the result.

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **where**.
*/
StatusCode Channels::put(Locator &where, pBlock p_block, int mode) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Channels) interface for **creating databases**

	\param where Some Locator to the endpoint compiled by Channels::as_locator() that can only be used once. It only supports the base
				**file** and will mkdir()

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **where**.
*/
StatusCode Channels::new_entity(Locator &where) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Channels) interface for **deleting databases and blocks**:

	\param where Some Locator to the endpoint compiled by Channels::as_locator() that can only be used once.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

This only supports the bases **file** (for both files and folders) and **http** (will send an HTTP_DELETE).

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **where**.
*/
StatusCode Channels::remove(Locator &where) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Native (Channels) interface for **Block copying** (possibly across bases but inside the Channels).

	\param where Some Locator to the destination endpoint compiled by Channels::as_locator() that can only be used once.
	\param what	 Some Locator to the source endpoint compiled by Channels::as_locator() that can only be used once.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

At least one of the endpoints must have the base **file**.

**NOTE**: This can only be used once since it calls destroy_extra_locator() on **where** and **what**.
*/
StatusCode Channels::copy(Locator &where, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Channels.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Channels::base_names(BaseNames &base_names) {

	base_names["bash"]	= this;		// Runs shell scripts
	base_names["file"]	= this;		// Returns arrays of bytes with attributes for files, IndexIS for folders.
	base_names["http"]	= this;		// libCURL
	base_names["tcp"]	= this;		// zeroMQ (client)
}


/** Forwards an HTTP_GET call to another node in the Jazz cluster.

	\param p_txn  A pTransaction owned by Channels. It must be destroy_transaction()-ed after successful use.
	\param node	  The name of the endpoint node. It must be found in the cluster config.
	\param p_url  The unparsed url (server excluded) the remote Jazz server can serve.
	\param apply  A code parsed by the API in range APPLY_NOTHING .. APPLY_NEW_ENTITY

	\return		  MHD_HTTP_OK on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_get(pTransaction &p_txn, Name node, pChar p_url, int apply) {

//TODO: Implement this.

	return MHD_HTTP_FORBIDDEN;
}


/** Forwards an HTTP_PUT call to another node in the Jazz cluster.

	\param node		The name of the endpoint node. It must be found in the cluster config.
	\param p_url	The unparsed url (server excluded) the remote Jazz server can serve.
	\param p_block	A block to be put (owned by the caller).

	\return			MHD_HTTP_CREATED on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_put(Name node, pChar p_url, pBlock p_block) {

//TODO: Implement this.

	return MHD_HTTP_FORBIDDEN;
}


/** Forwards an HTTP_DELETE call to another node in the Jazz cluster.

	\param node	  The name of the endpoint node. It must be found in the cluster config.
	\param p_url  The unparsed url (server excluded) the remote Jazz server can serve.

	\return		  MHD_HTTP_OK on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_del(Name node, pChar p_url) {

//TODO: Implement this.

	return MHD_HTTP_FORBIDDEN;
}

#ifdef CATCH_TEST

Channels CHN(&LOGGER, &CONFIG);

#endif

} // namespace jazz_elements

#ifdef CATCH_TEST
#include "src/jazz_elements/tests/test_channel.ctest"
#endif

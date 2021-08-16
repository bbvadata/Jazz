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


#include "src/jazz_elements/channel.h"


namespace jazz_elements
{


int client_test (void) {		// cppcheck-suppress unusedFunction

	printf ("Connecting to hello world server…\n");
	void *context = zmq_ctx_new ();
	void *requester = zmq_socket (context, ZMQ_REQ);
	zmq_connect (requester, "tcp://localhost:5555");

	int request_nbr;
	for (request_nbr = 0; request_nbr != 10; request_nbr++) {
		char buffer [10];
		printf ("Sending Hello %d…\n", request_nbr);
		zmq_send (requester, "Hello", 5, 0);
		zmq_recv (requester, buffer, 10, 0);
		printf ("Received World %d\n", request_nbr);
	}
	zmq_close (requester);
	zmq_ctx_destroy (context);

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
bool remote_testing_point () {			// cppcheck-suppress unusedFunction
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

/** Reads config variables and sets jazz_node_* public variables.
*/
StatusCode Channels::start() {
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
//TODO: Implement Channels::shut_down()

	return SERVICE_NO_ERROR;
}


/**
//TODO: Document this.
*/
StatusCode Channels::as_locator (Locator &result, pChar p_what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;
}


/**
//TODO: Document this.
*/
StatusCode Channels::get (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::get (pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::get (pTransaction &p_txn, Locator &what, pChar name) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::header (StaticBlockHeader &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::header (pTransaction &p_txn, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::put (Locator &where, pBlock p_block, int mode) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::new_entity (Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::remove (Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/**
//TODO: Document this.
*/
StatusCode Channels::copy (Locator &where, Locator &what) {

//TODO: Implement this.

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Channels.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Channels::base_names (BaseNames &base_names)
{
	base_names["file"]	= this;
	base_names["http"]	= this;
	base_names["index"]	= this;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_channel.ctest"
#endif

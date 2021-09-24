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

	if (!get_conf_key("ENABLE_ZEROMQ_CLIENT", can_zmq)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_ZEROMQ_CLIENT");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_HTTP_CLIENT", can_curl)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_HTTP_CLIENT");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_BASH_EXEC", can_bash)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_BASH_EXEC");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_FILE_LEVEL", file_lev)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_FILE_LEVEL");

		return EXIT_FAILURE;
	}

	if (!curl_ok)
		curl_ok = can_curl && curl_global_init(CURL_GLOBAL_SSL) == CURLE_OK;

	if (!zmq_ok)
		zmq_ok = can_zmq && ((zmq_context = zmq_ctx_new()) != nullptr) && ((zmq_requester = zmq_socket(zmq_context, ZMQ_REQ)) != nullptr);

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Channels::shut_down() {

	if (curl_ok) {
		curl_global_cleanup();

		curl_ok = false;
	}

	if (zmq_ok) {
		if (zmq_requester != nullptr)
			zmq_close(zmq_requester);

		if (zmq_context != nullptr)
			zmq_ctx_destroy(zmq_context);

		zmq_requester = nullptr;
		zmq_context	  = nullptr;

		zmq_ok = false;
	}

	pipes.clear();

	for (ConnMap::iterator it = connect.begin(); it != connect.end(); ++it) {
		it->second.clear();
	}
	connect.clear();

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** Native (Channels) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The data read from the endpoint will be stored as a rank == 1 CELL_TYPE_BYTE
					for all bases except "bash" shell output is returned as a rank == 1 CELL_TYPE_STRING
	\param p_what	The endpoint.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what) {

	if ((*p_what++ != '/') || (*p_what++ != '/') || (*p_what == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_what);

	switch (base) {
	case BASE_BASH_10BIT:
		if (!can_bash)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_FILE_10BIT: {
		if (file_lev < 1)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_what += 4;
		if (*p_what++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

	    struct stat p_stat;
    	int ret = stat(p_what, &p_stat);
		if (ret != 0)
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		if (S_ISDIR(p_stat.st_mode)) {
/*
			ret = new_block(p_txn, CELL_TYPE_INDEX);

			if (ret != SERVICE_NO_ERROR)
				return ret;
*/
			return SERVICE_NOT_IMPLEMENTED;
		}

		if (S_ISREG(p_stat.st_mode)) {
			if (p_stat.st_size > MAX_BLOCK_SIZE)
				return SERVICE_ERROR_BLOCK_TOO_BIG;

			int dim[MAX_TENSOR_RANK] = {(int) p_stat.st_size, 0};

			ret = new_block(p_txn, CELL_TYPE_BYTE, dim, FILL_NEW_DONT_FILL);

			if (ret != SERVICE_NO_ERROR)
				return ret;

			bool read_ok = false;
			FILE *fp;
			fp = fopen(p_what, "rb");
			if (fp != nullptr) {
				read_ok = fread(&p_txn->p_block->tensor.cell_byte, 1, p_stat.st_size, fp) == p_stat.st_size;

				fclose(fp);
			}

			if (!read_ok) {
				destroy_transaction(p_txn);

				return SERVICE_ERROR_IO_ERROR;
			}
			return SERVICE_NO_ERROR;
		}}
		return SERVICE_ERROR_IO_ERROR;

	case BASE_HTTP_10BIT:
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface **get(2)** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get(1) instead.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what, pBlock p_row_filter) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **get(3)** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get(1) instead.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what, pChar name) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **locate** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::locate(Locator &location, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **header** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::header(StaticBlockHeader &hea, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}



/** Easy Channels interface **header** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::header(pTransaction &p_txn, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface for **Block writing**

	\param p_where	The destination endpoint.
	\param p_block	The Block to be written/sent by Channels.
	\param mode		Some writing restriction that depends on the base. WRITE_ONLY_IF_EXISTS and WRITE_ONLY_IF_NOT_EXISTS can only be used
					on **file**. WRITE_TENSOR_DATA should be used as default, otherwise the metadata will also be written/sent.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::put(pChar p_where, pBlock p_block, int mode) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	switch (base) {
	case BASE_BASH_10BIT:
		if (!can_bash)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_FILE_10BIT:
		if (file_lev < 1)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_HTTP_10BIT:
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **creating folders**

	\param p_where The path to the folder to be created via mkdir()

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This is only used with //file.
*/
StatusCode Channels::new_entity(pChar p_where) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	if (base == BASE_FILE_10BIT) {
		if (file_lev != 3)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (mkdir(p_where, 0700) == 0)
			return SERVICE_NO_ERROR;

		return SERVICE_ERROR_IO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **deleting**:

	\param p_where Some endpoint to be deleted.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::remove(pChar p_where) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	switch (base) {
	case BASE_BASH_10BIT:
		if (!can_bash)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_FILE_10BIT:
		if (file_lev < 1)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_HTTP_10BIT:
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **Block copying** (possibly across bases but inside the Channels).

	\param p_where Some destination endpoint.
	\param p_what  Some source endpoint.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This is just a get() and a put(). See the description of Channels for reference.
*/
StatusCode Channels::copy(pChar p_where, pChar p_what) {

	pTransaction p_txn;

	int ret = get(p_txn, p_what);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	ret = put(p_where, p_txn->p_block);

	destroy_transaction(p_txn);

	return ret;
}


/** "Easy" interface for **Tuple translate**

	\param p_tuple	A Tuple with two items, "input" with the data passed to the service and "result" with the data returned. the
					result will be overridden in-place without any allocation.
	\param p_pipe	Some **service** does some computation on "input" and returns "result".

	\return	SERVICE_NO_ERROR on success or some negative value (error).

This is what most frameworks would call predict(), something that takes any tensor as an input and returns another tensor. In channels,
it just gives support to some other service doing that connected via zeroMQ or bash. Outside jazz_elements, the services use this
to run their own models.
*/
StatusCode Channels::translate(pTuple p_tuple, pChar p_pipe) {

	if (   p_tuple->cell_type != CELL_TYPE_TUPLE_ITEM || p_tuple->size != 2
		|| p_tuple->index((pChar) "input") != 0 || p_tuple->index((pChar) "result") != 1
		|| (*p_pipe++ != '/') || (*p_pipe++ != '/') || (*p_pipe == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_pipe);

	switch (base) {
	case BASE_BASH_10BIT:
		if (!can_bash)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;
		break;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Add the base names for this Channels.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Channels::base_names(BaseNames &base_names) {

	base_names["bash"] = this;		// Runs shell scripts
	base_names["file"] = this;		// Returns arrays of bytes with attributes for files, IndexIS for folders.
	base_names["http"] = this;		// libCURL
	base_names["0-mq"] = this;		// zeroMQ (client)
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

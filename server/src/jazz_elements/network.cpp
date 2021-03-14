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


#include "curl/curl.h"

#include "src/jazz_elements/network.h"


namespace jazz_elements
{
/** A test write callback.

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
size_t write_callback(char * ptr, size_t size, size_t nmemb, void *userdata)
{
	size = size*nmemb;

	// if ((uintptr_t) userdata == 0xbaaadc0ffee)

	printf("\n");

	for (int i = 0; i < size; i++)
		printf("%c", ptr[i]);

	printf("\n\n");

	return size;
}


bool remote_testing_point ()
{
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
	 Cluster : I m p l e m e n t a t i o n
--------------------------------------------------- */

Cluster::Cluster(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}

/**
//TODO: Document Cluster::start()
*/
StatusCode Cluster::start()
{
//TODO: Implement Cluster::start()

	return SERVICE_NO_ERROR;
}

/**
//TODO: Document Cluster::shut_down()
*/
StatusCode Cluster::shut_down(bool restarting_service)
{
//TODO: Implement Cluster::shut_down()

	return SERVICE_NO_ERROR;
}


/*	-----------------------------------------------
	 Remote : I m p l e m e n t a t i o n
--------------------------------------------------- */

Remote::Remote(pLogger a_logger, pConfigFile a_config, pCluster	a_cluster) : Container(a_logger, a_config)
{
	p_cluster = a_cluster;
}

/**
//TODO: Document Remote::start()
*/
StatusCode Remote::start()
{
//TODO: Implement Remote::start()

	return SERVICE_NO_ERROR;
}

/**
//TODO: Document Remote::shut_down()
*/
StatusCode Remote::shut_down(bool restarting_service)
{
//TODO: Implement Remote::shut_down()

	return SERVICE_NO_ERROR;
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Remote::base_names (BaseNames &base_names)
{
	base_names["jazz_get"] = this;
	base_names["jazz_put"] = this;

	base_names["jazz_broadcast"] = this;

	base_names["http_get"] = this;
	base_names["http_put"] = this;
}


} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_network.ctest"
#endif

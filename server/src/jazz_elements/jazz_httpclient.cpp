/* Jazz (c) 2018-2019 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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

#include "src/jazz_elements/jazz_httpclient.h"


namespace jazz_httpclient
{

/** A test write callback.

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
size_t httpclient_callback(char *ptr, size_t size, size_t nmemb, void *p_keepr)
{
	size = size*nmemb;

	if (p_keepr == nullptr) {
//		jCommons.log(LOG_INFO, "write_callback() : Right userdata.");
	} else {
//		jCommons.log(LOG_MISS, "write_callback() : Wrong userdata.");
	}

	printf("\n");

	for (int i = 0; i < size; i++)
		printf("%c", ptr[i]);

	printf("\n\n");

	return size;
}


/** Any test implemented as a jazzCommons method for convenience.

	\return	true if successful, false and log(LOG_MISS, "further details") if not.
*/
bool remote_testing_point ()
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
//		log(LOG_MISS, "jazzCommons::remote_testing_point() : curl_easy_init() failed.");

		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "http://20.1.71.31:8888///");
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpclient_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) 0xbaaadc0ffee);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
//		log(LOG_MISS, "jazzCommons::remote_testing_point() : curl_easy_perform() failed.");

		return false;
	}

	curl_easy_cleanup(curl);

	return true;
}

//TODO: Remove the PoC code above when no longer necessary.


/**
//TODO: Document JazzHttpclient
*/
JazzHttpclient::JazzHttpclient(jazz_utils::pJazzLogger a_logger)	: JazzBlockKeepr(a_logger)
{
//TODO: Implement JazzHttpclient
}


/**
//TODO: Document ~JazzHttpclient()
*/
JazzHttpclient::~JazzHttpclient()
{
//TODO: Implement ~JazzHttpclient()
}


/**
//TODO: Document get_jazz_block
*/
pJazzBlockKeeprItem JazzHttpclient::get_jazz_block (const JazzURL*p_url)
{
//TODO: Implement get_jazz_block
}


/**
//TODO: Document put_jazz_block
*/
bool JazzHttpclient::put_jazz_block (	  pJazzBlockKeeprItem  p_keepr,
									 const JazzURL			  *p_url)
{
//TODO: Implement put_jazz_block
}


/**
//TODO: Document delete_jazz_resource
*/
bool JazzHttpclient::delete_jazz_resource (const JazzURL	*p_url)
{
//TODO: Implement delete_jazz_resource
}


/**
//TODO: Document get_to_keepr
*/
bool JazzHttpclient::get_to_keepr (		 JazzBlockKeepr *p_keepr,
										 JazzBlockList	 p_id,
										 int			 num_blocks,
								   const JazzURL		*p_url_base)
{
//TODO: Implement get_to_keepr
}


/**
//TODO: Document put_from_keepr
*/
bool JazzHttpclient::put_from_keepr (	   JazzBlockKeepr *p_keepr,
										   JazzBlockList   p_id,
										   int			   num_blocks,
									 const JazzURL		  *p_url_base)
{
//TODO: Implement put_from_keepr
}


/**
//TODO: Document delete_jazz_resources
*/
bool JazzHttpclient::delete_jazz_resources (	  JazzBlockList	 p_id,
												  int			 num_blocks,
											const JazzURL		*p_url_base)
{
//TODO: Implement delete_jazz_resources
}

} // namespace jazz_httpclient


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_httpclient.ctest"
#endif

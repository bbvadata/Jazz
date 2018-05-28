/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

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

//TODO: Implement module jazz_httpclient.


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


} // namespace jazz_httpclient


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_httpclient.ctest"
#endif

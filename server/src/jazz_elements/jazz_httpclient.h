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

#include "curl/curl.h"


#include "src/jazz_elements/jazz_persistence.h"


#ifndef INCLUDED_JAZZ_ELEMENTS_HTTPCLIENT
#define INCLUDED_JAZZ_ELEMENTS_HTTPCLIENT


/**< \brief Simplest functionality to operate as an http client to GET, PUT, DELETE JazzBlock descendants to/from other Jazz nodes or any
http servers.

	This module uses curl in combination with JazzBlock and Jazz allocators (one shot or volatile) to simplify the communication from
	remote sources or across Jazz nodes.

//TODO: Extend module description for jazz_httpclient when implemented.

*/
namespace jazz_httpclient
{

using namespace jazz_containers;
using namespace jazz_persistence;


/// Apparently, there is no standard URL length limit. Some browsers seem to accept over 128K, but Apache has a limit of 4K. Lets use that.
#define JAZZ_MAX_URL_LENGTH		4096


/** A URL both for inter-Jazz and web services in general
*/
struct JazzURL {
	char url[JAZZ_MAX_URL_LENGTH];
};

/**
//TODO: Write the JazzHttpclient description
*/
class JazzHttpclient: public JazzBlockKeepr {

	public:
		 JazzHttpclient(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzHttpclient();

		// Methods for JazzBlock crud

		pJazzBlockKeeprItem get_jazz_block		 (const JazzURL				*p_url);

		bool				put_jazz_block		 (		pJazzBlockKeeprItem	 p_keepr,
												  const JazzURL				*p_url);

		bool				delete_jazz_resource (const JazzURL				*p_url);

		/// A pipeline interface

		bool get_to_keepr		   (	JazzBlockKeepr	 *p_keepr,
										JazzBlockList	  p_id,
										int				  num_blocks,
									const JazzURL		 *p_url_base);

		bool put_from_keepr		   (	  JazzBlockKeepr *p_keepr,
										  JazzBlockList	  p_id,
										  int			  num_blocks,
									const JazzURL		 *p_url_base);

		bool delete_jazz_resources (	  JazzBlockList	  p_id,
										  int			  num_blocks,
									const JazzURL		 *p_url_base);

	private:

		/**
//TODO: Document the method to make a URL form a base_url and a JazzBlockIdentifier
		*/
		inline JazzURL *merge_urls (const JazzURL			  *p_url_base,
									const JazzBlockIdentifier *p_id);
//TODO: Implement the method to make a URL form a base_url and a JazzBlockIdentifier
};

}

#endif

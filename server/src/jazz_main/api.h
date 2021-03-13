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

#include "src/include/jazz_agency.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_API
#define INCLUDED_JAZZ_MAIN_API


#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"


/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_agency
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;


/** Http methods
*/
#define HTTP_NOTUSED	0
#define HTTP_OPTIONS	1
#define HTTP_HEAD		2
#define HTTP_GET		3
#define HTTP_PUT		4
#define HTTP_DELETE		5

/// Parser limits

#define MAX_RECURSION_DEPTH	16		///< Max number of nested block references in a query.


/// Parser return codes

#define PARSE_OK			0		///< Success.
#define GET_OK				0		///< Success.


/** A buffer to keep the state while parsing/executing a query
*/
struct APIParseBuffer {
	int stack_size;								///< The stack size. Avoids having to memset() the whole APIParseBuffer.

	L_value l_value[MAX_RECURSION_DEPTH];		///< A stack of L_value used during parsing/executing
	R_value r_value[MAX_RECURSION_DEPTH];		///< A stack of R_value used during parsing/executing
};

typedef struct MHD_Response *pMHD_Response;

// ConfigFile and Logger shared by all services

extern ConfigFile  CONFIG;
extern Logger	   LOGGER;


extern int http_request_callback   (void *cls,
									struct MHD_Connection *connection,
									const char *url,
									const char *method,
									const char *version,
									const char *upload_data,
									size_t *upload_data_size,
									void **con_cls);


/** \brief Api: A Service to manage the REST API.

This service parses and executes http queries. It is aware a redistributes to all the appropriate services and is called directly by
the http callback http_request_callback().

*/
class Api : public Container {

	public:

		Api (pLogger	 a_logger,
			 pConfigFile a_config,
			 pContainer	 a_container,
			 pVolatile	 a_volatile,
			 pRemote	 a_remote,
			 pPersisted	 a_persisted,
			 pCluster	 a_cluster,
			 pBebop		 a_bebop);

		StatusCode start	 ();

		StatusCode shut_down (bool restarting_service = false);

		// parsing methods

		StatusCode parse	   (const char	   *url,
								int				method,
								APIParseBuffer &pars,
								bool			execution = true);

		StatusCode get_static  (const char	   *url,
								pBlockKeeper   *p_keeper,
								bool			execution = true);

		// deliver http error pages

		int	return_error_message (struct MHD_Connection *connection,
								  int					 http_status);

		// Specific execution methods

		bool upload	   (APIParseBuffer &parse_buff,
						const char	   *upload,
						size_t			size,
						bool			continue_upload);
		bool remove	   (APIParseBuffer &parse_buff);
		bool http_get  (APIParseBuffer &parse_buff,
						pMHD_Response  &response);

	private:

		pContainer	p_container;
		pVolatile	p_volatile;
		pRemote		p_remote;
		pPersisted	p_persisted;
		pCluster	p_cluster;
		pBebop		p_bebop;

};

extern Api	API;			// The API interface

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_API

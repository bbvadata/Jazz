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


#include <map>

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


#include <microhttpd.h>

#if MHD_VERSION < 0x00097000
/*! \brief Replacement type for the future (libmicrohttpd-dev 0.9.72-2)

In a version somewhere between > 0.9.66-1 and <= 0.9.72-2 libmicrohttpd changed the return type of int MHD_YES and MHD_NO to a:

enum MHD_Result {MHD_NO = 0, MHD_YES = 1};

*/
typedef int MHD_Result;
#endif

/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_agency
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_agency;


/** \brief A buffer to keep the state while parsing/executing a query
*/
struct HttpQueryState {
	int	parser_state;
	Locator l_value, r_value;
};


typedef struct MHD_Response *pMHD_Response;


// ConfigFile and Logger shared by all services

extern ConfigFile  CONFIG;
extern Logger	   LOGGER;


extern MHD_Result http_request_callback(void *cls,
										struct MHD_Connection *connection,
										const char *url,
										const char *method,
										const char *version,
										const char *upload_data,
										size_t *upload_data_size,
										void **con_cls);


/** \brief Api: A Service to manage the REST API.

This service parses and executes http queries. It is aware and redistributes to all the appropriate services. It is called directly by
the http callback http_request_callback().

*/
class Api : public Container {

	public:

		Api (pLogger	 a_logger,
			 pConfigFile a_config,
			 pChannels	 a_channels,
			 pVolatile	 a_volatile,
			 pPersisted	 a_persisted,
			 pBebop		 a_bebop,
			 pAgency	 a_agency);

		StatusCode start	 ();
		StatusCode shut_down ();

		// parsing methods

		StatusCode parse	   (const char	   *url,
								int				method,
								HttpQueryState &q_state,
								bool			execution = true);

		StatusCode get_static  (const char	   *url,
								pMHD_Response  &response,
								bool			execution = true);

		// deliver http error pages

		MHD_Result return_error_message (struct MHD_Connection *connection,
										 int					http_status);

		// Specific execution methods

		bool http_put	 (HttpQueryState &q_state,
						  const char	 *p_upload,
						  size_t		  size,
						  bool			  continue_upload);
		bool http_delete (HttpQueryState &q_state);
		bool http_get	 (HttpQueryState &q_state,
						  pMHD_Response  &response);

#ifndef CATCH_TEST
	private:
#endif

		StatusCode _load_statics (const char *path);

		pChannels	p_channels;
		pVolatile	p_volatile;
		pPersisted	p_persisted;
		pBebop		p_bebop;
		pAgency		p_agency;
		BaseNames	base;
		IndexSS		www;
		int			remove_statics;
};

extern Api	API;			// The API interface

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_API

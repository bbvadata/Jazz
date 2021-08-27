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


/*! \brief A proper type for specifying http status codes

Before libmicrohttpd (somewhere between > 0.9.66-1 and <= 0.9.72-2) changed MHD_Result to an enum, MHD_Result was (improperly) used
to define HTTP responses. That ended-up badly on newer versions, since it was passed to a MHD_queue_response() and stopped working
as it became an enum.

This triggeresd the need, for clarity reasons only, to introduce a new type, MHD_StatusCode to refer to **HTTP responses**.

See: https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

*/
typedef unsigned int MHD_StatusCode;


/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_agency
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_agency;


/// HttpQueryState apply values (on state == PSTATE_COMPLETE_OK)

#define APPLY_NOTHING					 0		///< Just an r_value with {///node}//base/entity or {///node}//base/entity/key
#define APPLY_NAME						 1		///< //base/entity/key:name (Select an item form a Tuple by name)
#define APPLY_URL						 2		///< {///node}//base# any_url_encoded_url ; (A call to http or file)
#define APPLY_FUNCTION					 3		///< {///node}//base/entity/key(//r_base/r_entity/r_key) (A function call on a block.)
#define APPLY_FUNCT_CONST				 4		///< {///node}//base/entity/key(# any_url_encoded_const ;) (A function call on a const.)
#define APPLY_FILTER					 5		///< {///node}//base/entity/key[//r_base/r_entity/r_key] (A filter on a block.)
#define APPLY_FILT_CONST				 6		///< {///node}//base/entity/key[# any_url_encoded_const ;] (A filter on a const.)
#define APPLY_RAW						 7		///< {///node}//base/entity/key.raw (Serialize text to raw.)
#define APPLY_TEXT						 8		///< {///node}//base/entity/key.text (Serialize raw to text.)
#define APPLY_ASSIGN					 9		///< {///node}//base/entity/key=//r_base/r_entity/r_key (Assign block to block.)
#define APPLY_ASSIGN_CONST				10		///< {///node}//base/entity/key=# any_url_encoded_const ; (Assign const to block.)
#define APPLY_JAZZ_INFO					11		///< /// Show the server info.


/** \brief A buffer to keep the state while parsing/executing a query
*/
struct HttpQueryState {
	int	state;									///< The parser state from PSTATE_INITIAL to PSTATE_COMPLETE_OK
	int apply;									///< APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, .. APPLY_ASSIGN_CONST

	char node	[NAME_SIZE];					///< An optional name of a Jazz node.
	char base	[SHORT_NAME_SIZE];				///< A Locator compatible base for the l_value.
	char entity	[NAME_SIZE];					///< A Locator compatible entity for the l_value.
	char key	[NAME_SIZE];					///< A Locator compatible key for the l_value.
	char name	[NAME_SIZE];					///< A possible item name
	char url	[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)

	Locator r_value;							///< Parsed //r_base/r_entity/r_key
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

		bool parse		(HttpQueryState &q_state,
						 pChar			 p_url,
						 int			 method);

		bool get_static	(pMHD_Response  &response,
						 pChar			 p_url,
						 bool			 get_it = true);

		// deliver http error pages

		MHD_Result return_error_message (struct MHD_Connection *connection,
										 int					http_status);

		// Specific execution methods

		bool http_put	 (const char	 *p_upload,
						  size_t		  size,
						  HttpQueryState &q_state,
						  bool			  continue_upload);
		bool http_delete (HttpQueryState &q_state);
		bool http_get	 (pMHD_Response  &response,
						  HttpQueryState &q_state);

#ifndef CATCH_TEST
	private:
#endif

		bool load_statics		(pChar	  path);
		bool expand_url_encoded	(pChar	  p_buff,
								 int	  buff_size,
								 pChar	  p_url);
		bool parse_nested		(Locator &r_value,
								 pChar	  p_url);

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

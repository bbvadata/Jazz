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

#include "src/include/jazz_api.h"


#ifndef INCLUDED_JAZZ_MAIN_RESTAPI
#define INCLUDED_JAZZ_MAIN_RESTAPI

#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"


/**< \brief Jazz class JazzHttpServer

	This module defines the class JazzHttpServer with the full logic to expose all the API available in
jazz_api.h through a REST API.

//TODO: Write module description for jazz_restapi when implemented.
*/
namespace jazz_restapi
{

using namespace jazz_api;


/** Callback function used to handle a POSIX signal.
*/
typedef void (*SignalHandler) (int signum);


/** The server's MHD_Daemon created by MHD_start_daemon() and needed for MHD_stop_daemon()
*/
typedef MHD_Daemon * pMHD_Daemon;


/** \brief TODO

The REST API supports standard http commands.

GET with a valid rvalue. To read from Jazz.
HEAD with a valid rvalue. Internally the same as GET, but returns the header only.
PUT with a valid lvalue. To write blocks into a Jazz keepr.
DELETE with a valid lvalue. To delete blocks or keeprs (even recursively).
OPTIONS with a string. Parses the string and returns the commands that would accept that string as a URL.
GET with lvalue=rvalue. Assignment in the server. Similar to “PUT(lvalue, GET(rvalue))” without traffic.
There is no support for POST or TRACE, any functions other than those mentioned return an error.

//TODO: Write the JazzHttpServer description
*/
class JazzHttpServer: public JazzAPI {

	public:
		 JazzHttpServer(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzHttpServer();

		int server_start(jazz_utils::pJazzConfigFile p_config,
						 SignalHandler				 p_sig_handler);
};

}

#endif

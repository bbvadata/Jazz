/* Jazz (c) 2018-2020 kaalam.ai (The Authors of Jazz), using (under the same license):

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

#include "src/jazz_main/api.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_SERVER
#define INCLUDED_JAZZ_MAIN_SERVER

#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"

/**< \brief One liner.

//TODO: Write this!
*/

namespace jazz_main
{

/** Callback function used to handle a POSIX signal.
*/
typedef void (*pSignalHandler) (int signum);


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
class HttpServer: jazz_elements::Service {

	public:
		 HttpServer (//jazz_elements::pLogger		a_logger,
					 //jazz_elements::pConfigFile a_config,
					 //pSignalHandler				p_sig_handler,
					 //pMHD_Daemon				&p_daemon
					 );
		~HttpServer ();

};


} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_SERVER


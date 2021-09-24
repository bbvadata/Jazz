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


#include <iostream>
#include <signal.h>


#include "src/jazz_main/instances.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_MAIN
#define INCLUDED_JAZZ_MAIN_MAIN


// F U N C T I O N A L   T O D O   L I S T : For anything in src/jazz_main
// =======================================   -----------------------------

//FUTURE: Implement http API access control.
	// This can be done combining two ways:
	//   1. By defining MHD_AcceptPolicyCallback and pass it to MHD_start_daemon() in HttpServer::start(), includes:
	//		  * Caller IP
	//   2. Inside http_request_callback() using "connection" which is a key:value including things like:
	//		  * Session management and cookies (See "On cookies" below)
	//		  * User-Agent:
	//		  * Accept-Language:

// On cookies:
// -----------

// 1. The idea is explained in https://developer.mozilla.org/en-US/docs/Web/HTTP/Cookies

// 	The server, has to check if the request comes with a cookie, if not, it has to send:

// 	Set-Cookie: jazz_user=a3fWa; Expires=Thu, 31 Oct 2021 07:28:00 GMT;

// 	After that, all requests will come with:

// 	GET /sample_page.html HTTP/2.0
// 	Host: www.example.org
// 	Cookie: jazz_user=a3fWa;

// 	The server uses the cookie to keep the conversation context persisted.

// 2. The use via MHD is explained in https://www.gnu.org/software/libmicrohttpd/tutorial.html#Session-management

// 3. The disclaimer has been added to config/static/cookies.htm


/** \brief	 The application entry point.

	Most high level server logic: see the description of main()
	The entry point is replaced by the catch2 entry point for unit testing.
*/


// Command line arguments for the Jazz server

#define CMD_HELP		0	///< Command 'help' as a numerical constant (see parse_arg())
#define CMD_START		1	///< Command 'start' as a numerical constant (see parse_arg())
#define CMD_STOP		2	///< Command 'stop' as a numerical constant (see parse_arg())
#define CMD_STATUS		3	///< Command 'status' as a numerical constant (see parse_arg())

#endif // ifndef INCLUDED_JAZZ_MAIN_MAIN

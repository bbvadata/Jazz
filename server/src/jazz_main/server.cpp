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

#include "microhttpd.h"

#include "src/jazz_main/server.h"


namespace jazz_main
{

/*	-----------------------------------------------
	 HttpServer : I m p l e m e n t a t i o n
--------------------------------------------------- */

HttpServer::HttpServer(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}


/** Start the Jazz server.

\param p_sig_handler	A function (of type pSignalHandler) that will be called when the process receives a SIGTERM signal.
\param p_daemon			Returns by reference the pointer that will be used to control the MHD_Daemon.

\return			On failure, EXIT_FAILURE. On success, the thread forks and only the parent process returns EXIT_SUCCESS, the child does
not return. The application is stopped when callback signalHandler_SIGTERM exits with EXIT_SUCCESS if shutting all services was successful
or with EXIT_FAILURE if not. On failure, the caller is responsible of stopping all started services (see jazz_main.cpp).

Starting logic:

 1. Get all the MHD server config settings via get_conf_key()

	The default config file is JAZZ_DEFAULT_CONFIG_PATH but that can be changed via command line argument (see jazz_main.cpp).

 2. Registers the signal handlers for SIGTERM. (See argument p_sig_handler)

 3. Forks (== The parent exits with EXIT_SUCCESS, the child continues to call MHD_start_daemon().)

 4. Calls MHD_start_daemon()

	Then calls setsid() This creates a new session if the calling process is not a process group leader. The calling process is the leader of the
	new session, the process group leader of the new process group, and has no controlling terminal.

	And sleeps forever! (Remember, it is the child of the original caller who exited with EXIT_SUCCESS.)
*/
StatusCode HttpServer::start(pSignalHandler p_sig_handler, pMHD_Daemon &p_daemon)
{
//TODO: Implement HttpServer::start()

	return SERVICE_NO_ERROR;
}

/**
//TODO: Document HttpServer::shut_down()
*/
StatusCode HttpServer::shut_down(bool restarting_service)
{
//TODO: Implement HttpServer::shut_down()

	return SERVICE_NO_ERROR;
}

} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_server.ctest"
#endif

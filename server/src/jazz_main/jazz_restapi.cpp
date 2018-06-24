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

#include "src/jazz_main/jazz_restapi.h"

namespace jazz_restapi
{

using namespace std;


/** The server's MHD_Daemon created by MHD_start_daemon() and needed for MHD_stop_daemon()
*/
struct MHD_Daemon *Jazz_MHD_Daemon;


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum)
{
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ..." << endl;

	MHD_stop_daemon (Jazz_MHD_Daemon);

	jCommons.logger_close();

	cout << "Stopping all services ..." << endl;

	bool clok = jServices.stop_all();

	cout << "Stopped all services : " << okfail(clok) << endl;

	if (!clok)
	{
		jCommons.log(LOG_ERROR, "Failed stopping all services.");

		exit (EXIT_FAILURE);
	}

	exit (EXIT_SUCCESS);
}


//TODO: Implement module jazz_restapi.

/**
//TODO: Document JazzHttpServer()
*/
JazzHttpServer::JazzHttpServer(jazz_utils::pJazzLogger a_logger)
{
//TODO: Implement JazzHttpServer
}


/**
//TODO: Document ~JazzHttpServer()
*/
JazzHttpServer::~JazzHttpServer()
{
//TODO: Implement ~JazzHttpServer
}


/**
//TODO: Document server_start()
*/
int JazzHttpServer::server_start()
{
//TODO: Implement server_start

	return EXIT_FAILURE;
}

} // namespace jazz_restapi


#if defined CATCH_TEST
#include "src/jazz_main/tests/test_restapi.ctest"
#endif

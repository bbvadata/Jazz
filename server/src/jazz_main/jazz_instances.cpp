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

#include "src/jazz_main/jazz_instances.h"

#if !defined CATCH_TEST

namespace jazz_instances
{
	using namespace std;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

JazzConfigFile J_CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
JazzLogger	   J_LOGGER(J_CONFIG, "LOGGER_PATH");
JazzCluster	   J_CLUSTER(&J_CONFIG, &J_LOGGER);
Bebop		   J_BOP;
JazzHttpServer J_HTTP_SERVER(&J_LOGGER);
rAPI		   J_R_API(&J_LOGGER);
pyAPI		   J_PYTHON_API(&J_LOGGER);

pMHD_Daemon	   Jazz_MHD_Daemon;


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum)
{
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ..." << endl;

	MHD_stop_daemon (Jazz_MHD_Daemon);

	cout << "Stopping JazzCluster ..." << endl;

	bool stop_ok = J_CLUSTER.ShutDown() == JAZZ_API_NO_ERROR;

	cout << "Stopping Bebop ..." << endl;

	if (J_BOP.ShutDown() != JAZZ_API_NO_ERROR) stop_ok = false;

	cout << "Stopping JazzHttpServer ..." << endl;

	if (J_HTTP_SERVER.ShutDown() != JAZZ_API_NO_ERROR) stop_ok = false;

	if (!stop_ok) {
		J_LOGGER.log(LOG_ERROR, "Errors occurred stopping the server.");

		exit (EXIT_FAILURE);
	}

	exit (EXIT_SUCCESS);
}

} // namespace jazz_instances

#endif

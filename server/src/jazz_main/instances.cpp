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


#include "src/jazz_main/instances.h"


namespace jazz_main
{
	using namespace std;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

ConfigFile	J_CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
Logger		J_LOGGER(J_CONFIG, "LOGGER_PATH");
HttpServer	J_HTTP_SERVER(&J_LOGGER, &J_CONFIG);
pMHD_Daemon	Jazz_MHD_Daemon;


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum)
{
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ..." << endl;

	MHD_stop_daemon (Jazz_MHD_Daemon);

	cout << "Stopping HttpServer ..." << endl;

	// bool stop_ok = J_HTTP_SERVER.stop();

	// ... Stop other services here.

	// if (!stop_ok) {
	// 	J_LOGGER.log(LOG_ERROR, "Errors occurred stopping the server.");

	// 	exit (EXIT_FAILURE);
	// }

	exit (EXIT_SUCCESS);
}

} // namespace jazz_main

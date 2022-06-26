/* Jazz (c) 2018-2022 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_main/instances.h"


namespace jazz_main
{
	using namespace std;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

#ifndef CATCH_TEST

ConfigFile	CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
Logger		LOGGER(CONFIG, "LOGGER_PATH");

// Services

Channels	CHANNELS (&LOGGER, &CONFIG);
Volatile	VOLATILE (&LOGGER, &CONFIG);
Persisted	PERSISTED(&LOGGER, &CONFIG);

// Uplifted containers:

#include "src/uplifted/uplifted_instances.cpp"

// Http server:

HttpServer	HTTP(&LOGGER, &CONFIG);

#endif

// Callbacks

pMHD_Daemon	Jazz_MHD_Daemon;


#ifndef CATCH_TEST

bool start_service(pService service, char const *service_name) {
	cout << "Starting " << service_name << " ... ";

	if (service->start() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred starting %s.", service_name);

		return false;
	} else {
		cout << "ok" << endl;
		LOGGER.log_printf(LOG_INFO, "Service %s started Ok.", service_name);

		return true;
	}
}


bool stop_service(pService service, char const *service_name) {
	cout << "Stopping " << service_name << " ... ";

	if (service->shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred stopping %s.", service_name);

		return false;
	} else {
		cout << "ok" << endl;
		LOGGER.log_printf(LOG_INFO, "Service %s stopped Ok.", service_name);

		return true;
	}
}

#endif


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ... ok." << endl;

	MHD_stop_daemon(Jazz_MHD_Daemon);

	bool stop_ok = true;

#ifndef CATCH_TEST

	if (!stop_service(&HTTP,	  "HttpServer")) stop_ok = false;
	if (!stop_service(&API,		  "Api"))		 stop_ok = false;
	if (!stop_service(&PERSISTED, "Persisted"))	 stop_ok = false;
	if (!stop_service(&VOLATILE,  "Volatile"))	 stop_ok = false;
	if (!stop_service(&CHANNELS,  "Channels"))	 stop_ok = false;

#endif

	if (stop_ok) exit(EXIT_SUCCESS); else exit(EXIT_FAILURE);
}

} // namespace jazz_main

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


#include "src/jazz_main/instances.h"


namespace jazz_main
{
	using namespace std;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

ConfigFile	CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
Logger		LOGGER(CONFIG, "LOGGER_PATH");

// Services

Agency		EPI		  (&LOGGER, &CONFIG);
Bebop		BOP		  (&LOGGER, &CONFIG);
Cluster		CLUSTER	  (&LOGGER, &CONFIG);
Container	ONE_SHOT  (&LOGGER, &CONFIG);
Persisted	PERSISTED (&LOGGER, &CONFIG);
Remote		REMOTE	  (&LOGGER, &CONFIG);
Volatile	VOLATILE  (&LOGGER, &CONFIG);
Api			API		  (&LOGGER, &CONFIG);
HttpServer	HTTP	  (&LOGGER, &CONFIG);

// Callbacks

pMHD_Daemon	Jazz_MHD_Daemon;


bool start_service(pService service, char* service_name)
{
	cout << "Starting " << service_name << " ... ";

	if (service->start() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred starting %s.", service_name);

		return false;
	} else {
		cout << "ok" << endl;

		return true;
	}
	}


bool stop_service(pService service, char* service_name)
{
	cout << "Stopping " << service_name << " ... ";

	if (service->shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred stopping %s.", service_name);

		return false;
	} else {
		cout << "ok" << endl;

		return true;
	}
	}

	if (EPI.shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log(LOG_ERROR, "Errors occurred stopping EPI.");

		stop_ok = false;
	} else {
		cout << "ok" << endl;
	}

	if (BOP.shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log(LOG_ERROR, "Errors occurred stopping BOP.");

		stop_ok = false;
	} else {
		cout << "ok" << endl;
	}

	if (BEAT.shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log(LOG_ERROR, "Errors occurred stopping BEAT.");

		stop_ok = false;
	} else {
		cout << "ok" << endl;
	}

	if (stop_ok) exit (EXIT_SUCCESS); else exit (EXIT_FAILURE);
}

} // namespace jazz_main

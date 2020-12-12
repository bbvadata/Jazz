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


#include "src/jazz_elements/service.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Service : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Start a Service descendant.

	This is the first method of a Service to be called.

	If start() returns true, it MAY get reload() calls and WILL get just one stop() call.
	If start() returns false, it will never be called again.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from Service)
*/
bool Service::start ()
{
	return true;
}


/** Stop a Service descendant.

	This is the last method of a Service to be called.

	It is called only once and only if start() was successful.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from Service)
*/
bool Service::stop ()
{
	return true;
}


/** Start a Service descendant.

	This is the first method of a Service to be called.

	If start() returns true, it MAY get reload() calls and WILL get just one stop() call.
	If start() returns false, it will never be called again.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from Service)
*/
bool Service::reload ()
{
	return true;
}

/*	-----------------------------------------------
	 Services : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Initialize the Services (Method 1: by directly giving it the output_file_name).

	Stores a copy of the file name,
	Calls InitLogger() for the rest of the initialization.
*/
Services::Services (pLogger aLog)
{
	pLog = aLog;
}


/** Adds a service to the list of Service objects managed by the Services.
*/
bool Services::register_service(Service* service)
{
	if (num_services >= MAX_NUM_SERVICES)
	{
		pLog->log(LOG_MISS, "Too many services in Services::register_service().");

		return false;
	}

	pService[num_services]	 = service;
	started [num_services++] = false;

	return true;
}


/** Starts all the registered services that have not been started yet.

	In case of a service failing to start, start_all() aborts not trying to start any other service.

	\return True on success. LOG_MISS and false on abort.
*/
bool Services::start_all ()
{
	for (int i = 0; i < num_services; i++)
	{
		if (!started[i])
		{
			if (!pService[i]->start())
			{
				pLog->log(LOG_MISS, "Some service failed to start, start_all() aborting.");

				return false;
			}
			started[i] = true;
		}
	}

	return true;
}


/** Stops all the registered services that have been started.

	In case of a service failing to stop, stop_all() will still close the rest of the started services.

	\return True on success. LOG_MISS and false on any failure.
*/
bool Services::stop_all ()
{
	bool st_ok = true;

	for (; num_services >= 0; --num_services)
	{
		if (started[num_services])
		{
			st_ok = pService[num_services]->stop() & st_ok;

			started[num_services] = true;
		}
	}

	if (!st_ok) pLog->log(LOG_MISS, "Some service(s) failed to stop in stop_all().");

	return st_ok;
}


/** Reloads all the registered services that have been started.

	In case of a service failing to reload, reload_all() aborts not trying to reload any other service.

	\return True on success. LOG_MISS and false on abort.
*/
bool Services::reload_all ()
{
	for (int i = 0; i < num_services; i++)
	{
		if (started[i])
		{
			if (!pService[i]->reload())
			{
				pLog->log(LOG_MISS, "Some service failed to reload, reload_all() aborting.");

				return false;
			}
		}
	}

	return true;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_service.ctest"
#endif

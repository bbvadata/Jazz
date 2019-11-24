/* Jazz (c) 2018-2019 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

   2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

		Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

	  This product includes software developed at

	   BBVA (https://www.bbva.com/)

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   3. LMDB: Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

   Licensed under http://www.OpenLDAP.org/license.html

   Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "src/include/jazz_api.h"

namespace jazz_api
{

/**
//TODO: Document JazzAPI()
*/
JazzAPI::JazzAPI(jazz_utils::pJazzLogger	 a_logger,
				 jazz_utils::pJazzConfigFile a_config) : JazzCache(a_logger, a_config)
{
//TODO: Implement JazzAPI
}


/**
//TODO: Document ~JazzAPI()
*/
JazzAPI::~JazzAPI()
{
	ShutDown();
}


/** Start the JazzAPI object loading its settings from a JazzConfigFile if necessary.

	\return JAZZ_API_NO_ERROR or any other API_ErrorCode in cases errors occurred. (Errors will also be logged out.)
*/
API_ErrorCode JazzAPI::StartService ()
{
	log(LOG_INFO, "Completed JazzAPI::StartService()");

	return JAZZ_API_NO_ERROR;
}


/** Close the JazzAPI object persisting pending cached write operations, freeing resources, etc.

	\param restarting_service Tell the object that it will be used again immediately if true and that makes any difference.
	\return					  JAZZ_API_NO_ERROR or any other API_ErrorCode in cases errors occurred. (Errors will also be logged out.)
*/
API_ErrorCode JazzAPI::ShutDown (bool restarting_service)
{
	log(LOG_INFO, "Completed JazzAPI::ShutDown()");

	return JAZZ_API_NO_ERROR;
}


/**
//TODO: Document rAPI()
*/
rAPI::rAPI(jazz_utils::pJazzLogger	 a_logger,
		   jazz_utils::pJazzConfigFile a_config)	: JazzAPI(a_logger, a_config)
{
//TODO: Implement rAPI
}


/**
//TODO: Document ~rAPI()
*/
rAPI::~rAPI()
{
//TODO: Implement ~rAPI
}


} // namespace jazz_api


#if defined CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif

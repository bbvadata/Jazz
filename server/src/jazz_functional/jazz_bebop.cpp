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

#include "src/jazz_functional/jazz_bebop.h"

namespace jazz_bebop
{

//TODO: Implement module jazz_bebop.

/**
//TODO: Document Bebop()
*/
Bebop::Bebop(jazz_utils::pJazzLogger	 a_logger,
			 jazz_utils::pJazzConfigFile a_config)	: JazzObject(a_logger, a_config)
{
//TODO: Implement Bebop
}


/**
//TODO: Document ~Bebop()
*/
Bebop::~Bebop()
{
	ShutDown();
}


/** Start the Bebop object loading its settings from a JazzConfigFile if necessary.

	\return JAZZ_API_NO_ERROR or any other API_ErrorCode in cases errors occurred. (Errors will also be logged out.)
*/
API_ErrorCode Bebop::StartService ()
{
	log(LOG_INFO, "Completed Bebop::StartService()");

	return JAZZ_API_NO_ERROR;
}


/** Close the Bebop object persisting pending cached write operations, freeing resources, etc.

	\param restarting_service Tell the object that it will be used again immediately if true and that makes any difference.
	\return					  JAZZ_API_NO_ERROR or any other API_ErrorCode in cases errors occurred. (Errors will also be logged out.)
*/
API_ErrorCode Bebop::ShutDown (bool restarting_service)
{
	log(LOG_INFO, "Completed Bebop::ShutDown()");

	return JAZZ_API_NO_ERROR;
}

} // namespace jazz_bebop


#if defined CATCH_TEST
#include "src/jazz_functional/tests/test_bebop.ctest"
#endif

/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/include/jazz_elements.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_BASE_API
#define INCLUDED_JAZZ_BEBOP_BASE_API


/** \brief A language to access any container by base using locators.

	BaseAPI is a Container and a parent of: Core, ModelsAPI and API.

	\see Container, Core, ModelsAPI and API
*/
namespace jazz_bebop
{

using namespace jazz_elements;

/** \brief A buffer to keep the state while parsing/executing a query
*/
struct ApiQueryState {
	int	state;									///< The parser state from PSTATE_INITIAL to PSTATE_COMPLETE_OK
	int apply;									///< APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, .. APPLY_ASSIGN_CONST

	char l_node	[NAME_SIZE];					///< An optional name of a Jazz node that is either the only one or the left of assignment.
	char r_node	[NAME_SIZE];					///< An additional optional name of a Jazz node for the right part in an assignment.
	char base	[SHORT_NAME_SIZE];				///< A Locator compatible base for the l_value.
	char entity	[NAME_SIZE];					///< A Locator compatible entity for the l_value.
	char key	[NAME_SIZE];					///< A Locator compatible key for the l_value.
	char name	[NAME_SIZE];					///< A possible item name
	char url	[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)

	Locator r_value;							///< Parsed //r_base/r_entity/r_key
	Locator rr_value;							///< Parsed //r_base/r_entity/r_key(//rr_base/rr_entity/rr_key)
};


/** \brief BaseAPI: The parent of API and Core.

This manages parsing queries and them to the appropriate containers.

*/
class BaseAPI : public Container {

	public:

		BaseAPI(pLogger	a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted);
	   ~BaseAPI();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();
};
typedef BaseAPI *pBaseAPI;		///< A pointer to a BaseAPI

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_BASE_API

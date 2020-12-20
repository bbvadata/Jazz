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

#include "src/jazz_main/server.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_INSTANCES
#define INCLUDED_JAZZ_MAIN_INSTANCES


/**< \brief One liner.

//TODO: Write this!
*/

namespace jazz_main
{

using namespace jazz_elements;

#define JAZZ_DEFAULT_CONFIG_PATH "config/jazz_config.ini"

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

extern ConfigFile  J_CONFIG;
extern Logger	   J_LOGGER;
extern HttpServer  J_HTTP_SERVER;
extern pMHD_Daemon Jazz_MHD_Daemon;

void signalHandler_SIGTERM(int signum);

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_INSTANCES

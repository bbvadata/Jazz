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


#include <iostream>
#include <signal.h>


#include "src/jazz_main/instances.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_MAIN
#define INCLUDED_JAZZ_MAIN_MAIN


// F U N C T I O N A L   T O D O   L I S T : For anything in src/jazz_main
// =======================================   -----------------------------

//TODO: Reach [TEST] level in api (Feb 28)
//TODO: Reach [MVP] level in api (Apr 04)
//TODO: Reach [TEST] level in server (Feb 28)
//TODO: Reach [MVP] level in server (Apr 04)
//TODO: Implement cookie-based user state.
//TODO: Implement static API access control.
//TODO: Implement server API access control.
//TODO: Implement backend API access control.


/**< \brief	 The application entry point.

	Most high level server logic: see the description of main()
	The entry point is replaced by the catch2 entry point for unit testing.
*/


// Command line arguments for the Jazz server

#define CMD_HELP		0	///< Command 'help' as a numerical constant (see parse_arg())
#define CMD_START		1	///< Command 'start' as a numerical constant (see parse_arg())
#define CMD_STOP		2	///< Command 'stop' as a numerical constant (see parse_arg())
#define CMD_STATUS		3	///< Command 'status' as a numerical constant (see parse_arg())

#endif // ifndef INCLUDED_JAZZ_MAIN_MAIN

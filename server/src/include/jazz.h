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

#include <chrono>
#include <iostream>
#include <map>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

/**< \brief Basic JAZZ types and definitions.

	This file contains constants, codeless types and API-level classes.
*/

#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"

#if defined CATCH_TEST

	#ifndef jzz_IG_CATCH
	#define jzz_IG_CATCH

		#include "src/catch2/catch.hpp"

	#endif

#endif

/*~ end of automatic header ~*/

#include <math.h>

#include "src/include/jazz_platform.h"

#ifndef jzz_IG_JAZZ
#define jzz_IG_JAZZ




#endif

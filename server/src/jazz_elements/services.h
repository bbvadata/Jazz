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

// #include "src/jazz_elements/xxx.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_SERVICES
#define INCLUDED_JAZZ_ELEMENTS_SERVICES


/**< \brief One liner.

//TODO: Write this!
*/

namespace jazz_elements
{

#define MAX_NUM_JAZZ_SERVICES	   8	///< The maximum number of jazzService instances stored in a jazzServices.

/** Ancestor class of all Jazz services requiring start/stop/reload. This is an empty container of virtual methods that should be implemented
in the descendants.
*/
class jazzService {

	public:

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();
};


/** A collection of jazzService descendants operated through a single point of access.
*/
class jazzServices {

	public:

		bool register_service(jazzService * service);

		bool start_all	();
		bool stop_all	();
		bool reload_all ();

	private:

		int			 num_services = 0;
		bool		 started [MAX_NUM_JAZZ_SERVICES];
		jazzService* pService[MAX_NUM_JAZZ_SERVICES];
};

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_SERVICES


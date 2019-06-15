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


#include "src/include/jazz_platform.h"


/**< \brief Basic JAZZ types and definitions.

	This file merges headers in src/jazz_elements/ to define all the higher level types and root classes. This is what you need to create
your own descendent classes to extend Jazz en C++. If you need the core implementations, Bebop or extending in scripting languages, you
need jazz_api.h
*/


#ifndef INCLUDED_JAZZ_INCLUDE_JAZZ
#define INCLUDED_JAZZ_INCLUDE_JAZZ

#include "src/jazz_elements/jazz_datablocks.h"
#include "src/jazz_elements/jazz_utils.h"
#include "src/jazz_elements/jazz_containers.h"
#include "src/jazz_elements/jazz_persistence.h"
#include "src/jazz_elements/jazz_httpclient.h"
#include "src/jazz_elements/jazz_classes.h"
#include "src/jazz_elements/jazz_stdcore.h"


#endif

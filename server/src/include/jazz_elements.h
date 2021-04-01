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


#include "src/include/jazz_platform.h"


#ifndef INCLUDED_JAZZ_ELEMENTS
#define INCLUDED_JAZZ_ELEMENTS


/* Includes everything in namespace jazz_elements without using it by default.

The namespace jazz_elements contains everything to build Jazz except the http server, the agency parts and the details of the Bebop
language implementation. This is: utilities, implementations of all the data (block, tuple, kind) and code (field) types and the most
fundamental services: volatile (which allocates blocks in RAM), persisted (which persists block in lmdb), network (extends Jazz to a
cluster and also includes REST API client) and flux (the fundamental Jazz equivalent to a logical "device").
*/


// F U N C T I O N A L   T O D O   L I S T : For anything in src/jazz_elements
// =======================================   ---------------------------------

//TODO: Reach [MVP] level in volatile (Apr 11)
//TODO: Reach [MVP] level in persisted (Apr 25)
//TODO: Reach [MVP] level in network (May 9)
//TODO: Reach [MVP] level in flux (May 16)


#include "src/jazz_elements/types.h"
#include "src/jazz_elements/utils.h"
#include "src/jazz_elements/block.h"
#include "src/jazz_elements/kind.h"
#include "src/jazz_elements/tuple.h"
#include "src/jazz_elements/network.h"
#include "src/jazz_elements/container.h"
#include "src/jazz_elements/volatile.h"
#include "src/jazz_elements/persisted.h"
#include "src/jazz_elements/column.h"
#include "src/jazz_elements/table.h"
#include "src/jazz_elements/flux.h"


#endif // ifndef INCLUDED_JAZZ_ELEMENTS

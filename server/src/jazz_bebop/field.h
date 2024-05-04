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

#include "src/jazz_bebop/snippet.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_FIELD
#define INCLUDED_JAZZ_BEBOP_FIELD


/** \brief Formal Fields: Organizing op-codes and snippets by Kind.

Fields are morphisms defined as op-codes and snippets in a unique way for each pair of Kinds.
*/

namespace jazz_bebop
{

class Field : public jazz_elements::Container {

	public:

		Field(jazz_elements::pLogger	 a_logger,
			  jazz_elements::pConfigFile a_config,
			  pPack						 a_pack);
	   ~Field();

		jazz_elements::StatusCode start	();
		jazz_elements::StatusCode shut_down();
};
typedef Field *pField;


#ifdef CATCH_TEST

// Instancing Field
// -----------------

extern Field FLS;

#endif

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_FIELD

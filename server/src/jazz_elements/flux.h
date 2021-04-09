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


// #include <stl_whatever>


#include "src/jazz_elements/table.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_FLUX
#define INCLUDED_JAZZ_ELEMENTS_FLUX


namespace jazz_elements
{

/** \brief A table of two columns **source** and **destination**, with only one special index **time**.

A flux is a one-directional communication of tuples using one abstraction. Therefore, tuples (which are exchanged in any flux
interaction) are indexed in some abstraction (a tree of indices). Different branches of a flux may even have different kinds. It is the
only mechanism of communication. It includes reading or writing to files (trees of folders are part of abstraction), exploring large
collections of books, videos, etc. and streaming (where the index is time). In Jazz, "device" is not a precise word as in unix. What counts
is the flux, its abstraction defines the API. The microphone, camera, motor system, ... are fluxes. We simplify. When both directions
are required, there are two fluxes up and down, so a QA game has two fluxes. Storage is read/write, but not a flux.

*/
class Flux : public Block {

	public:

};

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_FLUX

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


#include "src/jazz_elements/kind.h"


namespace jazz_elements
{

/** Check the internal validity of a Kind (item structure, dimensions, etc.)
not repeated of invalid item names.

	\return KIND_TYPE_NOTAKIND on error or KIND_TYPE_KIND if every check passes ok.
*/
int Kind::audit()
{
	if (cell_type != CELL_TYPE_KIND_ITEM | size <= 0)
		return KIND_TYPE_NOTAKIND;

	std::set <int> items;

	for (int i = 0; i < size; i++) {
		ItemHeader *p_it_hea = &tensor.cell_item[i];

		if (  p_it_hea->cell_type < 0 | p_it_hea->rank < 1 | p_it_hea->rank > MAX_TENSOR_RANK | p_it_hea->level < 0
		    | p_it_hea->name <= STRING_EMPTY | p_it_hea->size != 0 | p_it_hea->data_start != 0)
				return KIND_TYPE_NOTAKIND;

		if (items.find(p_it_hea->name) != items.end())
			return KIND_TYPE_NOTAKIND;

		items.insert(p_it_hea->name);
	}

	for (int i = 0; i < size; i++) {
		ItemHeader *p_it_hea = &tensor.cell_item[i];

		for (int j = 0; j < p_it_hea->rank; j++) {
			int k = p_it_hea->dim[j];
			if (k < 0) {
				if (items.find(-k) != items.end())
					return KIND_TYPE_NOTAKIND;
			}
		}
	}

	return KIND_TYPE_KIND;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_kind.ctest"
#endif

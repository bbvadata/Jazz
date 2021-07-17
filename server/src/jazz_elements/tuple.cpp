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


#include "src/jazz_elements/tuple.h"


namespace jazz_elements
{

/** Check the internal validity of a Tuple (item structure, dimensions, etc.)

	\return MIXED_TYPE_INVALID on error or MIXED_TYPE_TUPLE if every check passes ok.
*/
int Tuple::audit()
{
	int dims[MAX_TENSOR_RANK];

	if (cell_type != CELL_TYPE_TUPLE_ITEM || size <= 0)
		return MIXED_TYPE_INVALID;

	for (int i = 0; i < size; i++) {
		ItemHeader *p_it_hea = &tensor.cell_item[i];

		if (   p_it_hea->cell_type <= 0 || p_it_hea->rank < 1 || p_it_hea->rank > MAX_TENSOR_RANK
		    || p_it_hea->name <= STRING_EMPTY || p_it_hea->data_start <= 0)
				return MIXED_TYPE_INVALID;

		if (!valid_name(&p_string_buffer()->buffer[p_it_hea->name]))
			return MIXED_TYPE_INVALID;

		pBlock p_block = block(i);

		if (p_it_hea->rank != p_block->rank || p_it_hea->cell_type != p_block->cell_type)
			return MIXED_TYPE_INVALID;

		p_block->get_dimensions(dims);

		for (int j = 0; j < p_it_hea->rank; j++) {
			if (p_it_hea->dim[j] != dims[j])
				return MIXED_TYPE_INVALID;
		}
	}

	return MIXED_TYPE_KIND;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_tuple.ctest"
#endif

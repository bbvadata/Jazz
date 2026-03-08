/* Jazz (c) 2018-2026 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include <math.h>
#include <cstring>


#include "src/jazz_elements/types.h"


namespace jazz_elements
{

/** Constants for the types of cells in a Tensor
	\return An R-compatible numeric NA.
*/
inline double R_ValueOfNA() {
	union {double d; int i[2];} na;

	na.i[1] = 0x7ff00000;
	na.i[0] = 1954;

	return na.d;
}

float	F_NA	= nanf("");
double	R_NA	= R_ValueOfNA();
ff_fp16	F16_NA	= {0x7e00};			///< FLOAT16_NA: 0x7e00 is the IEEE 754 half-precision floating point representation of NaN
ff_fp16	BF16_NA	= {0x7fc0};			///< BFLOAT16_NA: 0x7fc0 is the Brain Floating Point, half-precision representation of NaN

uint32_t F_NA_uint32;	///< A binary exact copy of F_NA
uint64_t R_NA_uint64;	///< A binary exact copy of R_NA

/** Initialize F_NA_uint32 and R_NA_uint64 with the binary representation of F_NA and R_NA, respectively.
	\return true	Always true, just to set a flag.
*/
inline bool init_uint_na() {
	memcpy(&F_NA_uint32, &F_NA, sizeof(F_NA));
	memcpy(&R_NA_uint64, &R_NA, sizeof(R_NA));

	return true;
}

bool uinit_na_initialized = init_uint_na();	///< A flag to ensure that F_NA_uint32 and R_NA_uint64 are initialized before use.

} // namespace jazz_elements

#ifdef CATCH_TEST
#include "src/jazz_elements/tests/test_types.ctest"
#endif

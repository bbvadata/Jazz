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


/**< \brief Miscelaneous utility functions for Jazz.

	This module defines many unrelated functions needed by Jazz. The only rule is: functions, not classes and
without global variables. For example, the logger does not belong here, since it is a configurable object, but
some elements of the logger that are just pure functions are here.
*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#define TENBITS_LUT_SIZE 1024	///< The size of a table indexable by all possible output values of TenBitsAtAddress()


#ifndef INCLUDED_JAZZ_ELEMENTS_UTILS
#define INCLUDED_JAZZ_ELEMENTS_UTILS

namespace jazz_utils
{

int		  CountBytesFromUtf8	(char *buff, int len);
char	 *ExpandEscapeSequences	(char *buff);
pid_t 	  FindProcessIdByName	(const char *name);
uint64_t  MurmurHash64A			(const void *key, int len);

/** Get ten bits taking the least significant 5 of the first two characters of a string.
	Warning: No pointer validation or length check. Never use on nullptr or "".
*/
inline int TenBitsAtAddress (const char* str)
{
	return ((str[1] & 0x1f) << 5) | (str[0] & 0x1F);
}

}

#endif

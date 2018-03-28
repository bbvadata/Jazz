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

//TODO: Follow the style guide on naming.
//TODO: Follow the style guide on the use of the namespace.
//TODO: Move tests to a test file (conditionally included).


#include "src/jazz_elements/jazz_utils.h"

/** Count the number of bytes required by an utf-8 string of len characters.

\param buff The string (not necessarily null-terminated)
\param len	The number of characters in the string

\return		The numebr of bytes in the string.

The RFC http://www.ietf.org/rfc/rfc3629.txt says:

Char. number range	|		 UTF-8 octet sequence
(hexadecimal)	|			   (binary)
--------------------+---------------------------------------------
0000 0000-0000 007F | 0xxxxxxx
0000 0080-0000 07FF | 110xxxxx 10xxxxxx
0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
int CountBytesFromUtf8(char *buff, int len)
{
	int bytes = 0;

	while (len > 0)	{
		len--;
		bytes++;
		char lb = *buff++;

		if ((lb & 0xE0) == 0xC0) {			// 110x xxxx
			buff++;
			bytes++;
		}
		else if ((lb & 0xF0) == 0xE0) {		// 1110 xxxx
			buff  += 2;
			bytes += 2;
		}
		else if ((lb & 0xF8 ) == 0xF0) {	// 1111 0xxx
			buff  += 3;
			bytes += 3;
		}
	}

	return bytes;
}


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_utils.ctest"
#endif

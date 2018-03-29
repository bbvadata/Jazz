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

//TODO: Move all the utils and break the logger in functions.


#include <map>
#include <string.h>

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


/** Expand escaped strings at runtime.

Public Domain by Jerry Coffin.

Interprets a string in a manner similar to that the compiler does string literals in a program.	 All escape sequences are
longer than their translated equivalant, so the string is translated in place and either remains the same length or becomes shorter.

April 17 tests built and compliant with:

\\a	 07	   Alert (Beep, Bell) (added in C89)[1]
\\b	 08	   Backspace
\\f	 0C	   Formfeed
\\n	 0A	   Newline (Line Feed); see notes below
\\r	 0D	   Carriage Return
\\t	 09	   Horizontal Tab
\\v	 0B	   Vertical Tab
\\\\ 5C	   Backslash
\\'	 27	   Single quotation mark
\\"	 22	   Double quotation mark
\\?	 3F	   Question mark (used to avoid trigraphs)
\\nnn	   The byte whose numerical value is given by nnn interpreted as an octal number
\\xhh	   The byte whose numerical value is given by hh… interpreted as a hexadecimal number

https://en.wikipedia.org/wiki/Escape_sequences_in_C
*/
char *ExpandEscapeSequences(char * buff)
{
	char        *pt	 = buff;
	size_t		 len = strlen(buff);
	unsigned int num, any;

	while (nullptr != (pt = strchr(pt, '\\'))) {
		int numlen = 1;
		switch (pt[1]) {
		case 'a':
			*pt = '\a';
			break;

		case 'b':
			*pt = '\b';
			break;

		case 'f':
			*pt = '\f';
			break;

		case 'n':
			*pt = '\n';
			break;

		case 'r':
			*pt = '\r';
			break;

		case 't':
			*pt = '\t';
			break;

		case 'v':
			*pt = '\v';
			break;

		case '\\':
			break;

		case '\'':
			*pt = '\'';
			break;

		case '\"':
			*pt = '\"';
			break;

		case '\?':
			*pt = '\?';
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			if (pt[2] && pt[3] && sscanf(&pt[1], "%3o", &num) == 1 && sscanf(&pt[2], "%3o", &any) == 1 && sscanf(&pt[3], "%3o", &any) == 1)
			{
				numlen = 3;
				*pt	 = (char) num;
			}
			else
			{
				numlen = 1;
				*pt = pt[1];
			}
			break;

		case 'x':
			if (pt[2] && pt[3] && sscanf(&pt[2], "%2x", &num) == 1 && sscanf(&pt[3], "%2x", &any) == 1)
			{
				numlen = 3;
				*pt	 = (char) num;
			}
			else
			{
				numlen = 1;
				*pt = pt[1];
			}
			break;

		default:
			*pt = pt[1];
		}

		int siz = pt - buff + numlen;
		pt++;
		memmove(pt, pt + numlen, len - siz);
	}

	return buff;
}


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_utils.ctest"
#endif

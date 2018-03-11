/* BBVA - Jazz: A lightweight analytical web server for data-driven applications.
   ------------

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <signal.h>

using namespace std;

/**< \brief Many utilities implemented as C functions not part of any class.
*/

#include "src/include/jazz_commons.h"

/*~ end of automatic header ~*/


/** Find the pid of a process given its name.

	It does NOT find itself as it is intended to find other processes of the same name.

	\param name The name of the process as typed on the console. In case parameters were given to start the process,
	it is just the left part before the first space.

	\return the pid of the process if found, 0 if not.
 */
pid_t proc_find(const char* name)
{
	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];

	if (!(dir = opendir("/proc"))) {
		perror("can't open /proc");
		return 0;
	}

	pid_t pid_self = getpid();

	while((ent = readdir(dir)) != NULL) {
		// if endptr is not a null character, the directory is not entirely numeric, so ignore it
		long lpid = strtol(ent->d_name, &endptr, 10);
		if (*endptr != '\0') {
			continue;
		}

		// try to open the cmdline file
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");

		if (fp) {
			if (fgets(buf, sizeof(buf), fp) != NULL) {
				// check the first token in the file, the program name
				char* first = strtok(buf, " ");

				// cout << first << endl;
				if (!strcmp(first, name) && (pid_t) lpid != pid_self) {
					fclose(fp);
					closedir(dir);
					return (pid_t) lpid;
				}
			}
			fclose(fp);
		}
	}

	closedir(dir);
	return 0;
}


/** Check if a file exists.

	\param fnam The file name.
	\return true if exists
 */
bool exists(const char* fnam)
{
	ifstream ff(fnam);

	return ff.good();
}


/** Return a boolean status as a text message

	\param b The value to be returned.
	\return Either "Ok." or "*FAILED*".
 */
const char* okfail(bool b)
{
	if (b) return "Ok.";

	return("*FAILED*");
}


/** Remove space and tab for a string.

	\param s Input string
	\return String without space or tab.
*/
string remove_sptab(string s)
{
	for(int i = s.length() - 1; i >= 0; i--) if(s[i] == ' ' || s[i] == '\t') s.erase(i, 1);

	return (s);
}

/*	-----------------------------------------------
	  H A S H I N G
--------------------------------------------------- */

#define MURMUR_SEED	  76493		///< Just a 5 digit prime


/** MurmurHash2, 64-bit versions, by Austin Appleby

	(from https://sites.google.com/site/murmurhash/)

	All code is released to the public domain. For business purposes, Murmurhash is
	under the MIT license.

	The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
	and endian-ness issues if used across multiple platforms.

	// 64-bit hash for 64-bit platforms
*/
uint64_t MurmurHash64A (const void * key, int len)
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int	   r = 47;

	uint64_t h = MURMUR_SEED ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end  = data + (len/8);

	while(data != end)
	{
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7)
	{
		case 7: h ^= uint64_t(data2[6]) << 48;
		case 6: h ^= uint64_t(data2[5]) << 40;
		case 5: h ^= uint64_t(data2[4]) << 32;
		case 4: h ^= uint64_t(data2[3]) << 24;
		case 3: h ^= uint64_t(data2[2]) << 16;
		case 2: h ^= uint64_t(data2[1]) << 8;
		case 1: h ^= uint64_t(data2[0]);
				h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
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
char * expand_escaped(char * buff)
{
	char *		 pt	 = buff;
	size_t		 len = strlen(buff);
	unsigned int num, any;

	while (NULL != (pt = strchr(pt, '\\')))
	{
		int numlen = 1;
		switch (pt[1])
		{
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
				if (pt[2] && pt [3] && sscanf(&pt[1], "%3o", &num) == 1 && sscanf(&pt[2], "%3o", &any) == 1 && sscanf(&pt[3], "%3o", &any) == 1)
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
				if (pt[2] && pt [3] && sscanf(&pt[2], "%2x", &num) == 1 && sscanf(&pt[3], "%2x", &any) == 1)
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
int count_utf8_bytes(char *	buff, int len)
{
	int bytes = 0;

	while (len > 0)
	{
		len--;
		bytes++;
		char lb = *buff++;
		if ((lb & 0xE0) == 0xC0)	  // 110x xxxx
		{
			buff++;
			bytes++;
		}
		else if ((lb & 0xF0) == 0xE0) // 1110 xxxx
		{
			buff  += 2;
			bytes += 2;
		}
		else if ((lb & 0xF8 ) == 0xF0) // 1111 0xxx
		{
			buff  += 3;
			bytes += 3;
		}
	}

	return bytes;
}

/*	-----------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST

SCENARIO("Testing tenbits().")
{
	REQUIRE(tenbits("7") == 0x17);
	REQUIRE(tenbits("0") == 0x10);
	REQUIRE(tenbits("a") == 0x01);

	REQUIRE(tenbits("77") == 0x2f7);
	REQUIRE(tenbits("70") == 0x217);
	REQUIRE(tenbits("7a") == 0x037);
	REQUIRE(tenbits("a7") == 0x2e1);
	REQUIRE(tenbits("a0") == 0x201);
	REQUIRE(tenbits("aa") == 0x021);
	REQUIRE(tenbits("00") == 0x210);

	REQUIRE(tenbits("77x")	 == 0x2f7);
	REQUIRE(tenbits("70y")	 == 0x217);
	REQUIRE(tenbits("7az")	 == 0x037);
	REQUIRE(tenbits("a7xx")	 == 0x2e1);
	REQUIRE(tenbits("a0yy")	 == 0x201);
	REQUIRE(tenbits("aazz")	 == 0x021);
	REQUIRE(tenbits("00ttt") == 0x210);
}


SCENARIO("Testing count_utf8_bytes().")
{
	char buf1[80] = {"abcdefghij"};
	char buf2[80] = {"ácido"};
	char buf3[80] = {"eñe"};
	char buf4[80] = {"\xF0...\xE0..\xC0.Z"};

	REQUIRE(count_utf8_bytes(buf1,	0) ==  0);
	REQUIRE(count_utf8_bytes(buf1,	1) ==  1);
	REQUIRE(count_utf8_bytes(buf1,	2) ==  2);
	REQUIRE(count_utf8_bytes(buf1, 10) == 10);

	REQUIRE(count_utf8_bytes(buf2,	0) ==  0);
	REQUIRE(count_utf8_bytes(buf2,	1) ==  2);
	REQUIRE(count_utf8_bytes(buf2,	2) ==  3);
	REQUIRE(count_utf8_bytes(buf2,	5) ==  6);

	REQUIRE(count_utf8_bytes(buf3,	0) ==  0);
	REQUIRE(count_utf8_bytes(buf3,	1) ==  1);
	REQUIRE(count_utf8_bytes(buf3,	2) ==  3);
	REQUIRE(count_utf8_bytes(buf3,	3) ==  4);

	REQUIRE(count_utf8_bytes(buf4,	0) ==  0);
	REQUIRE(count_utf8_bytes(buf4,	1) ==  4);
	REQUIRE(count_utf8_bytes(buf4,	2) ==  7);
	REQUIRE(count_utf8_bytes(buf4,	3) ==  9);
	REQUIRE(count_utf8_bytes(buf4,	4) == 10);
}


SCENARIO("Testing expand_escaped().")
{
	char buff[80];

	buff[0] = 0;
	REQUIRE(!strlen(expand_escaped(buff)));

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "Hello, world!")), "Hello, world!"));

	buff[5] = 4;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\abc")), "\abc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\ab")), "9\ab"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\a")), "89\a"));

	REQUIRE(buff[5] == 4);
	buff[3] = 2;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\a")), "\a"));

	REQUIRE(buff[3] == 2);
	buff[5] = 5;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\bbc")), "\bbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\bb")), "9\bb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\a")), "89\a"));

	REQUIRE(buff[5] == 5);
	buff[3] = 3;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\b")), "\b"));

	REQUIRE(buff[3] == 3);
	buff[5] = 6;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\fbc")), "\fbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\fb")), "9\fb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\f")), "89\f"));

	REQUIRE(buff[5] == 6);
	buff[3] = 4;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\f")), "\f"));

	REQUIRE(buff[3] == 4);
	buff[5] = 7;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\nbc")), "\nbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\nb")), "9\nb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\n")), "89\n"));

	REQUIRE(buff[5] == 7);
	buff[3] = 5;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\n")), "\n"));

	REQUIRE(buff[3] == 5);
	buff[5] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\rbc")), "\rbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\rb")), "9\rb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\r")), "89\r"));

	REQUIRE(buff[5] == 8);
	buff[3] = 6;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\r")), "\r"));

	REQUIRE(buff[3] == 6);
	buff[5] = 9;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\tbc")), "\tbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\tb")), "9\tb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\t")), "89\t"));

	REQUIRE(buff[5] == 9);
	buff[3] = 7;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\t")), "\t"));

	REQUIRE(buff[3] == 7);
	buff[5] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\vbc")), "\vbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\vb")), "9\vb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\v")), "89\v"));

	REQUIRE(buff[5] == 8);
	buff[3] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\v")), "\v"));

	REQUIRE(buff[3] == 8);
	buff[5] = 7;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\abc")), "\abc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\ab")), "9\ab"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\a")), "89\a"));

	REQUIRE(buff[5] == 7);
	buff[3] = 9;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\a")), "\a"));

	REQUIRE(buff[3] == 9);
	buff[5] = 6;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\\bc")), "\\bc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\\\b")), "9\\b"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\\\")), "89\\"));

	REQUIRE(buff[5] == 6);
	buff[3] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\\")), "\\"));

	REQUIRE(buff[3] == 8);
	buff[5] = 5;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\'bc")), "\'bc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\\'b")), "9\'b"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\\'")), "89\'"));

	REQUIRE(buff[5] == 5);
	buff[3] = 7;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\'")), "\'"));

	REQUIRE(buff[3] == 7);
	buff[5] = 4;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\"bc")), "\"bc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\\"b")), "9\"b"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\\"")), "89\""));

	REQUIRE(buff[5] == 4);
	buff[3] = 6;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\a")), "\a"));

	REQUIRE(buff[3] == 6);
	buff[5] = 3;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\\?bc")), "\?bc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\\?b")), "9\?b"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\\?")), "89\?"));

	REQUIRE(buff[5] == 3);
	buff[3] = 5;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\a")), "\a"));

	REQUIRE(buff[3] == 5);
	buff[7] = 1;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\141bc")), "abc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "d\\145f")), "def"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "z\\040A")), "z A"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "z\\40A")),	 "z40A"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "78\\071")), "789"));

	REQUIRE(buff[7] == 1);
	buff[5] = 9;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\112")), "J"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\011")), "\t"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\11")),  "11"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\007")), "\a"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\07")),  "07"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\7")),   "7"));

	REQUIRE(buff[5] == 9);
	buff[5] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\cbc")), "cbc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "9\\cb")), "9cb"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "89\\c")), "89c"));

	REQUIRE(buff[5] == 8);
	buff[3] = 7;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\c")), "c"));

	REQUIRE(buff[3] == 7);
	buff[7] = 9;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x61bc")), "abc"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "d\\x65f")), "def"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "z\\x20A")), "z A"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "z\\x09A")), "z\tA"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "z\\x9G")),	 "zx9G"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "78\\x39")), "789"));

	REQUIRE(buff[7] == 9);
	buff[5] = 8;

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x4a")), "J"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x4A")), "J"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x09")), "\t"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x9")),  "x9"));
	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x78")), "x"));

	REQUIRE(buff[5] == 8);

	REQUIRE(!strcmp(expand_escaped(strcpy(buff, "\\x078")), "\a8"));
}


SCENARIO("Elementary hashing.")
{
	REQUIRE(sizeof(uint64_t) == 8);
	REQUIRE(MURMUR_SEED	== 76493);

	int today = 02022017;

	REQUIRE(MurmurHash64A(&today, 4) == 1361846023767824266);
	//cout << "Origin of time's hash " << MurmurHash64A(&today, 4) << endl;

	GIVEN("Three sets of vars of different types. A!=B, A==C")
	{
		int	   ia	  = 299792458;
		double da	  = 3.141592;
		char   sa[16] = "Hello, world!";

		REQUIRE(sizeof(ia) == 4);
		REQUIRE(sizeof(da) == 8);
		REQUIRE(sizeof(sa) == 16);

		int	   ib	  = 299792758;
		double db	  = 3.141591999;
		char   sb[16] = "Hello, world.";

		int	   ic	  = 299792458;
		double dc	  = 3.141592;
		char   sc[16] = "Hello, world!";

		uint64_t hia, hda, hsa;

		hia = MurmurHash64A(&ia, sizeof(ia));
		hda = MurmurHash64A(&da, sizeof(da));
		hsa = MurmurHash64A(&sa, sizeof(sa));

		WHEN("I hash the first set.")
		{
			THEN("I get different hashes ...")
			{
				REQUIRE(hia != hda);
				REQUIRE(hia != hsa);
				REQUIRE(hda != hsa);
			}

			THEN("... but identical if I repeat.")
			{
				REQUIRE(hia == MurmurHash64A(&ia, sizeof(ia)));
				REQUIRE(hda == MurmurHash64A(&da, sizeof(da)));
				REQUIRE(hsa == MurmurHash64A(&sa, sizeof(sa)));
			}
		}
		WHEN("I hash the second set.")
		{
			uint64_t hib, hdb, hsb;

			hib = MurmurHash64A(&ib, sizeof(ib));
			hdb = MurmurHash64A(&db, sizeof(db));
			hsb = MurmurHash64A(&sb, sizeof(sb));

			THEN("I get different hashes.")
			{
				REQUIRE(hib != hdb);
				REQUIRE(hib != hsb);
				REQUIRE(hdb != hsb);
			}

			THEN("They are all different from those in A ...")
			{
				REQUIRE(hia != hib);
				REQUIRE(hda != hdb);
				REQUIRE(hda != hdb);
			}

			THEN("... but identical if I repeat.")
			{
				REQUIRE(hib == MurmurHash64A(&ib, sizeof(ib)));
				REQUIRE(hdb == MurmurHash64A(&db, sizeof(db)));
				REQUIRE(hsb == MurmurHash64A(&sb, sizeof(sb)));
			}
		}
		WHEN("I hash the third set.")
		{
			uint64_t hic, hdc, hsc;

			hic = MurmurHash64A(&ic, sizeof(ic));
			hdc = MurmurHash64A(&dc, sizeof(dc));
			hsc = MurmurHash64A(&sc, sizeof(sc));

			THEN("I get different hashes.")
			{
				REQUIRE(hic != hdc);
				REQUIRE(hic != hsc);
				REQUIRE(hdc != hsc);
			}

			THEN("They are all identical to those in A ...")
			{
				REQUIRE(hia == hic);
				REQUIRE(hda == hdc);
				REQUIRE(hda == hdc);
			}

			THEN("... and identical if I repeat.")
			{
				REQUIRE(hic == MurmurHash64A(&ic, sizeof(ic)));
				REQUIRE(hdc == MurmurHash64A(&dc, sizeof(dc)));
				REQUIRE(hsc == MurmurHash64A(&sc, sizeof(sc)));
			}
		}
	}
}

#endif

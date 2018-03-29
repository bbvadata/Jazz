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

#include "src/include/jazz01_commons.h"

/*~ end of automatic header ~*/


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

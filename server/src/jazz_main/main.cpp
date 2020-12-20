/* Jazz (c) 2018-2020 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#define CATCH_CONFIG_MAIN		//This tells Catch2 to provide a main() - has no effect when CATCH_TEST is not defined

#include "src/jazz_main/main.h"


#if !defined CATCH_TEST


/** Display the Jazz logo message automatically appending JAZZ_VERSION.
 */
void show_credits()
{
	cout << "\x20 888888" << endl
		 << "\x20 \x20 `88b" << endl
		 << " \x20 \x20 888" << endl
		 << " \x20 \x20 888\x20 8888b.\x20 88888888 88888888" << endl
		 << " \x20 \x20 888 \x20 \x20 `88b\x20 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 .d888888 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 88P 888\x20 888\x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 `Y888888 88888888 88888888" << endl
		 << " \x20 .d88P" << endl
		 << " .d88P'" << endl
		 << "888P'" << endl << endl
		 << " \x20 \x20 (c)" << JAZZ_YEARS << " - kaalam.ai - The Authors of Jazz" << endl << endl
		 << " \x20 \x20This product includes software developed at BBVA" << endl
		 << " \x20 \x20 Licensed under the Apache License, Version 2.0" << endl
		 << " \x20 \x20 \x20 http://www.apache.org/licenses/LICENSE-2.0" << endl
		 << " \x20 \x20 \x20 \x20 \x20version: " << JAZZ_VERSION << " (" << LINUX_PLATFORM << ")" << endl
#ifdef DEBUG
		 << endl
		 << " \x20 \x20 \x20 \x20 \x20 \x20 *** D E B U G - B U I L D ***" << endl
#endif
		 << endl;
}


int main(int argc, char* argv[])
{

	exit(EXIT_FAILURE);
}


#endif


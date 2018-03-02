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

using namespace std;

/*! \brief Instances of all jazzService descendants.

	All the descendants (via jazzWebSource, jzzBLOCKS or jzzFUNCTIONAL) except jzzAPI are instantiated in jzzINSTANCES when the application runs
normally DEBUG or NDEBUG) except under CATCH_TEST. jzzAPI is instantiated in jzzAPI.cpp since jzzAPI needs all other descendants.

	See the RFC instantiation.html for details on instantiation.
*/

#include "src/jazz_main/jazz_instances.h"

/*~ end of automatic header ~*/

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

jzzBLOCKCONV  jBLOCKC;

/*	-----------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST
TEST_CASE("Test for jzzINSTANCES")
{
	// No real tests here. This line is added to avoid a warning when running ./switch_tests.R

	REQUIRE(sizeof(jzzBLOCKCONV) == sizeof(jBLOCKC));
}
#endif

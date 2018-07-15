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

#include "src/jazz_elements/jazz_stdcore.h"

namespace jazz_stdcore
{

/**
//TODO: Document JazzCoreTypecasting()
*/
JazzCoreTypecasting::JazzCoreTypecasting(jazz_utils::pJazzLogger a_logger)	: JazzObject(a_logger)
{
//TODO: Implement JazzCoreTypecasting
}


/**
//TODO: Document ~JazzCoreTypecasting()
*/
JazzCoreTypecasting::~JazzCoreTypecasting()
{
//TODO: Implement ~JazzCoreTypecasting
}


bool JazzCoreTypecasting::FromR (pJazzBlock p_source, pJazzBlock &p_dest)
{

}


bool JazzCoreTypecasting::ToR (pJazzBlock p_source, pJazzBlock &p_dest)
{

}


bool JazzCoreTypecasting::FromText (pJazzBlock p_source, pJazzBlock &p_dest, int type, char *fmt)
{

}


bool JazzCoreTypecasting::ToText (pJazzBlock p_source, pJazzBlock &p_dest, const char *fmt)
{

}

} // namespace jazz_stdcore


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_stdcore.ctest"
#endif

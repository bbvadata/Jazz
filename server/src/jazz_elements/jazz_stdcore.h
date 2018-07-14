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


#include "src/jazz_elements/jazz_classes.h"


#ifndef INCLUDED_JAZZ_ELEMENTS_STDCORE
#define INCLUDED_JAZZ_ELEMENTS_STDCORE


/**< \brief Arithmetic, logic and type conversion stdcore applicable to JazzDataBlock structures.

	This module defines pure functions that accept JazzDataBlock structures as arguments and return JazzDataBlock
structures or throw exceptions. These functions are called "stdcore" for a reason, they are the simplest blocks
approximately matching Bebop bytecode instructions.

//TODO: Extend module description for jazz_stdcore when implemented.

*/
namespace jazz_stdcore
{

using namespace jazz_datablocks;
using namespace jazz_containers;


/**
//TODO: Write the JazzCoreTypecasting description
*/
class JazzCoreTypecasting : public JazzObject {

	public:
		 JazzCoreTypecasting(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzCoreTypecasting();

		bool FromR	  (pJazzBlock p_source, pJazzBlock &p_dest);
		bool ToR	  (pJazzBlock p_source, pJazzBlock &p_dest);

		bool FromText (pJazzBlock p_source, pJazzBlock &p_dest, int type, char *fmt);
		bool ToText   (pJazzBlock p_source, pJazzBlock &p_dest, const char *fmt);
};


/**
//TODO: Write the JazzCoreArithmetic description
*/
class JazzCoreArithmetic : public JazzObject {

	public:
		 JazzCoreArithmetic(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzCoreArithmetic();

};


/**
//TODO: Write the JazzCoreMath description
*/
class JazzCoreMath : public JazzObject {

	public:
		 JazzCoreArithmetic(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzCoreArithmetic();

};

}

#endif

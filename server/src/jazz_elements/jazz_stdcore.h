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


//TODO: Document interface for module jazz_stdcore.

//TODO: Implement interface for module jazz_stdcore.

class JazzCoreTypecasting {

	public:
		 JazzCoreTypecasting(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzCoreTypecasting();

		bool FromR	  (pJazzBlock p_source, pJazzBlock &p_dest);
		bool ToR	  (pJazzBlock p_source, pJazzBlock &p_dest);

		bool FromText (pJazzBlock p_source, pJazzBlock &p_dest, int type, char *fmt);
		bool ToText   (pJazzBlock p_source, pJazzBlock &p_dest, const char *fmt);


		/** Wrapper method logging events through a JazzLogger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param message	A message.

			See JazzLogger for details.
		*/
		inline void log (int loglevel, const char *message) { if (p_log != nullptr) p_log->log(loglevel, message); }

		/** Wrapper method logging events through a JazzLogger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param fmt		The printf-style format string.
			\param ...		The list of parameters as a variadic list of parameters.

			See JazzLogger for details.
		*/
		inline void log_printf (int loglevel, const char *fmt, ...) {
			if (p_log != nullptr) {
				va_list args;
				va_start(args, fmt);
				p_log->log_printf(loglevel, fmt, args);
				va_end(args);
			}
		}

	private:

		jazz_utils::pJazzLogger	p_log;
};

}

#endif

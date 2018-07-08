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

/* NOTES from the Jazz draft description including tasks for this
   --------------------------------------------------------------

//TODO: BopCompiler Bebop compiles into bytecode automatically.

//TODO: A Bebop function is a block of compiled bytecode with its source code stored as an attribute.

//TODO: Bebop is functional (== functions accept functions as arguments and can return functions), blocks are immutable, reads are safe,
		writes are idempotent, side effects are not possible.

//TODO: Bebop source code uses operators (both in the standard core or defined in Bebop) to support “natural expressions”. Operators are
		only syntactic sugar for functions.

//TODO: Function overloading is possible with and order and a match for type checking. This provides some metaprogramming, like defining
		math over many types in one declaration. This also applies to operators (since they are just functions).

//TODO: Bebop is object oriented. Bebop supports multiple inheritance using combined classes of any number of parents.
//TODO: Objects can be forked from other objects. A forked object points to its ancestor and is initialized with its state without using
		extra storing space (until its state changes from that of its ancestor).
//TODO: A class is internally a keepr that contains variables and functions.

//TODO: Errors in a wrong mutation can be handled by the ancestor function. Bebop supports alternative functions for managing errors.

//TODO: Automatic function result caching. Use of "memoize" directive. (Or memoize by default and declare "prevent_result_cache")

//TODO: Consider 'using ... as ...' the easily create aliases of lvalues and rvalues.
//TODO: Consider 'matching ... as {... {} ... {} ... {}}' as the case syntax. (Bebop supports match for switching (as in Rust).)
//TODO: Consider a logic for type assertion (possibly with regex) A bebop function has a low-level way to specify the first, 2nd, ... block
		passed as arguments. It may also put restriction on types (any numeric sorted, etc.) that throw exceptions when not met. Since
		refering to the argument as $1, $2, ... is unfriendly, the argument declaration implicitly creates a series of 'using $1 as
		latitude' statement that allow using the argument with a nice name and an assertion on its type.

//TODO: Program flow level 1 - 'for each ... {}'
//TODO: Program flow level 2 - 'if ... {}', 'if not ... {}', 'if ... {} else {}'
//TODO: Program flow level 3 - 'while ... {}', 'do {} while ...'
//TODO: Program flow level 4 - 'break', 'fail ...', 'return ...'

*/

#include "src/jazz_functional/jazz_dataframe.h"


#ifndef INCLUDED_JAZZ_FUNCTIONAL_BEBOP
#define INCLUDED_JAZZ_FUNCTIONAL_BEBOP


/**< \brief TODO

//TODO: Write module description for jazz_bebop when implemented.
*/
namespace jazz_bebop
{

//TODO: Document interface for module jazz_bebop.

//TODO: Implement interface for module jazz_bebop.

class Bebop	: public JazzObject {

	public:
		 Bebop(jazz_utils::pJazzLogger a_logger = nullptr);
		~Bebop();

		API_ErrorCode StartService (jazz_utils::pJazzConfigFile p_config 		   = nullptr);
		API_ErrorCode ShutDown     (bool 						restarting_service = false);

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

//TODO: Define Bebop
//TODO: Implement Bebop
//TODO: Document Bebop
//TODO: Test Bebop

	private:

		jazz_utils::pJazzLogger	p_log;
};

}

#endif

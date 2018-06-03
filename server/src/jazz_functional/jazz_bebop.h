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

Bebop source code uses operators (both in the standard core or defined in Bebop) to support “natural expressions”.
Bebop has aliases and other syntactic sugar. E.g., using /xxx.yy.zz as z
Bebop is functional, blocks are immutable, reads are safe, writes are idempotent, side effects are not possible.
Bebop is object oriented.
Bebop supports multiple inheritance using combined classes of any number of parents.
Objects can be forked from other objects. A forked object points to its ancestor and is initialized with its state without using extra storing space (until its state changes from that of its ancestor).
Bebop supports match for type checking. This provides some metaprogramming, like defining math over many types in one declaration.
Bebop supports match for switching (as in Rust).
Bebop supports alternative functions for managing errors.

//TODO: Review assertions on bebop listed above

object forking,
pattern matching and
alternative function

//TODO: Review assertions on support for automated coding listed above

//TODO: Automatic function result caching

//TODO: Consider 'using ... as ...' the easily create aliases of lvalues and rvalues.
//TODO: Consider 'matching ... as {... {} ... {} ... {}}' as the case syntax.
//TODO: Consider how to make regex a first class citizen.
//TODO: Consider a logic for type assertion (possibly with regex) A bebop function has a low-level way to specify the first, 2nd, ... block
		passed as arguments. It may also put restriction on types (any numeric sorted, etc.) that throw exceptions when not met. Since
		refering to the argument as $1, $2, ... is unfriendly, the argument declaration implicitly creates a series of 'using $1 as
		latitude' statement that allow using the argument with a nice name and an assertion on its type.

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

}

#endif

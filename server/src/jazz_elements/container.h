/* Jazz (c) 2018-2021 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include <map>
#include <atomic>
#include <thread>
#include <stdarg.h>
#include <regex>


#include "src/jazz_elements/tuple.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_CONTAINER
#define INCLUDED_JAZZ_ELEMENTS_CONTAINER


namespace jazz_elements
{

/// Block API (dimensions of structures)

#define NAME_SIZE						32					///< Size of a Name (ending 0 included)
#define NAME_LENGTH						NAME_SIZE - 1		///< Maximum length of a Name.name
#define MAX_CONTAINERS_IN_LOCATOR		7					///< A locator has (max) this # of container/sub-container names + block name.
#define MAX_CONTRACTS_IN_RVALUE			6					///< An rvalue operation can apply (max) that many contracts.
#define QUERY_LENGTH					4096				///< Maximum length of an API query


/// Block API (syntax related)

#define REGEX_VALIDATE_NAME				"^[a-zA-Z][a-zA-Z0-9_]{0,30}$"	///< Regex validating a Name

/// Block API (method arguments)

/// Block API (error codes)


/** The identifier of a Container type, a container inside another container, a Block descendant in a container, a field in a Tuple or
Kind, or the name of a contract. It must be a string matching REGEX_VALIDATE_NAME.
*/
struct Name {
	char name [NAME_SIZE];
};

/** A binary block identifier internal to the Container. Typically a MurmurHash64A of the Block name.
*/
typedef uint64_t BlockId64;

/** An atomically increased (via fetch_add() and fetch_sub()) 32 bit signed integer to use as a lock.
*/
typedef std::atomic<int32_t> Lock32;

// Forward pointer types:

typedef struct BlockKeeper 	*pBlockKeeper;
typedef class Container		*pContainer;

/** A pair of pointers to manage allocation inside an array of BlockKeeper as a deque.
*/
struct OneShotDeque {
	pBlockKeeper	p_prev, p_next;		///< A pair of pointers to keep this (the descendant) in a double linked list
};

/** The extra space in a BlockKeeper.

	This union keeps Container specific data.
*/
union KeeperData
{
	Name		 name;					///< Name of the block (used by Container descendants using locators)
	OneShotDeque deque;					///< A pair of pointer (used by the root class Container)
};

/** The minimum struc to keep track of block allocation in a Container.
The descendants will use classes with more data inheriting from this.
The caller can **only read** the data from here by using the pBlockKeeper returned by the Container API.
*/
struct BlockKeeper {
	pBlock			p_block;			///< A pointer to the Block (if status == BLOCK_STATUS_READY)
	pContainer		p_owner;			///< A pointer to the Container instance serving API calls related to this block.
	BlockId64		block_id;			///< A 64 bit binary block id. Can be a hash of the block locator, but not necessarily.
	Lock32			lock;				///< An atomically updated int to lock the Keeper (can only be used via p_owner->lock(), ..)
	int				status;				///< The status of the block request (sync or async errors, wait, ready, etc.)
	KeeperData		data;				///< Some data used by the Container service
};


/** \brief Container: A Service to manage Jazz blocks. All Jazz blocks are managed by this or a descendant of this.

This is the root class for all containers. It has memory alloc for one-shot block allocation and methods for filtering and serialization.
Its descendants are: Volatile, Remote and Persisted, completing all possible block allocations: one-shot, volatile, remote and persisted.

It provides, "rules of the game" for all descendants;

- A Block locator mechanism: Container name, Block name and contracts
- The struc BlockKeeper to store Blocks
- Thread safety (and thread specific storage just for this class)
- An allocation API .new(locked=True), .lock(), .unlock()
- A crud API .get(), .put(only_if=PUT_ALWAYS), .delete()
- Support for Block contracts
- A configuration style that will be honored by all descendants

It provides, specifically for the Container class;

- A deque of BlockKeeper structures: .l_push(), .l_pop(), .l_peek(), .r_push(), .r_pop(), .r_peek()

One-shot Block allocation
-------------------------

This is the only container providing one-shot Block allocation. This does not mean, unlike in previous versions, the caller owns the
pointer. One-shot Block allocation is intended for computing intermediate blocks like: blocks generated from constants, from slicing,
returned by functions, etc. In these blocks, the locator is meaningless, the caller will typically allocate by l_push() and recover
by l_pop(). A Block returned via l_pop() is still owned by the Container and requires explicit .unlock()-ing by the caller.

This also the only container that is thread specific. Anywhere else, blocks are uniquely identified by their locators in a
thread-transparent way for the caller. Since blocks here do not have locators, the whole operation happens in the context of a `thread_idx`.
The `thread_idx` is **not** a thread id as returned by `pthread_self()`, it is an index in the thread pool. Thread-aware services
(normally API and Bebop cores) have a `thread_idx` valid during the lifetime of whatever operations require call to this Container.

*/
class Container : public Service {

	public:

		Container (pLogger	   a_logger,
				   pConfigFile a_config);

		Service_ErrorCode start		();
		Service_ErrorCode shut_down	(bool restarting_service = false);
};


} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CONTAINER

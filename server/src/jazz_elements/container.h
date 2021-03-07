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
#define MAX_NESTED_CONTAINERS			6					///< (max) sub-container names in a locator (base is resolved to a pointer).
#define MAX_CONTRACTS_IN_RVALUE			6					///< An rvalue operation can apply (max) that many contracts.
#define QUERY_LENGTH					4096				///< Maximum length of an API query
#define ANSWER_LENGTH					4096				///< Maximum length of an Answer

/// Block API (syntax related)

#define REGEX_VALIDATE_NAME				"^[a-zA-Z][a-zA-Z0-9_]{0,30}$"	///< Regex validating a Name

/// Block API (method arguments)

/// Block API (error and status codes)

#define BLOCK_STATUS_READY				 0					///< BlockKeeper.status: p_block-> is safe to use
#define BLOCK_STATUS_ASYNC_WAIT			 1					///< BlockKeeper.status: async content pending, wait or call p_owner->sleep()
#define BLOCK_STATUS_ASYNC_FAIL			-1					///< BlockKeeper.status: async failed, still locked, call p_owner->unlock()
#define BLOCK_STATUS_SYNC_FAIL			-2					///< BlockKeeper.status: sync failed, still locked, call p_owner->unlock()
#define BLOCK_STATUS_SYNC_UNLOCKED		-3					///< BlockKeeper.status: block destroying, do nothing, forget the pointer

/** The identifier of a Container type, a container inside another container, a Block descendant in a container, a field in a Tuple or
Kind, or the name of a contract. It must be a string matching REGEX_VALIDATE_NAME.
*/
typedef char Name[NAME_SIZE];

/** A string possibly returned by a contract. Contracts return either a block or an answer.
*/
struct Answer {
	char	text[ANSWER_LENGTH];		///< A message, metadata, lists of items, columns, etc.
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
	OneShotDeque deque;					///< A pair of pointers (used by the root class Container)
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

/** A Locator is used by all Containers using block names == all except the root deque. It locates a block (existing or new) and is the
base of both lvalues and rvalues.
*/
struct Locator {
	bool			only_if_exists;						///< For lvalue: fail if the block does not exist.
	bool			only_if_new;						///< For lvalue: fail if the block already exists.
	bool			only_if_local;						///< Fail if not in this node (== don't route it if elsewhere)
	bool			gwp_update;							///< GWP (Group Wide Propagation) message for synching sharded columns and tables
	Name			container[MAX_NESTED_CONTAINERS];	///< All the sub-container names (the base container is used to route the call)
	Name			block;								///< The block name
};

/**
*/
struct ContractRequest {
	Name			action;								///<
	pBlockKeeper	p_args;								///<
};

/**
*/
struct Lvalue {
	Locator			path;								///<
};

/**
*/
struct Rvalue {
	Locator			path;								///<
	ContractRequest contract[MAX_CONTRACTS_IN_RVALUE];	///<
};

/** \brief Container: A Service to manage Jazz blocks. All Jazz blocks are managed by this or a descendant of this.

This is the root class for all containers. It has memory alloc for one-shot block allocation and methods for filtering and serialization.
Its descendants are: Volatile, Remote and Persisted, completing all possible block allocations: one-shot, volatile, remote and persisted.

It follows the "rules of the game" using:

- Name, Answer and ContractRequest
- BlockKeeper, Locator
- All the constants, error codes, etc.

It provides a neat API for all descendants, including:

- Transparent Thread safety (and thread specific storage just for this class)
- An API for async calls (Remote): .sleep()
- Allocation: .new(), .lock(), .unlock()
- Crud: .put(), .delete()
- Support for contracts: .get()
- A configuration style for all descendants

It provides, exposed by the root Container class, but also used internally by descendants;

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

Instances and inheritance
-------------------------

Note that the descendants don't inherit each other. E.g., Volatile does not have a Remote implementation, but they all have the basic
deque mechanism inherited from Container: i.e., they can all allocate new blocks for their own purposes. Configuration-wise the
allocations set by ONE_SHOT_MAX_KEEPERS, ONE_SHOT_WARN_BLOCK_KBYTES, ... only apply to the instance of the class Container, the
descendants use their own limits in which they include this allocation combined with whatever other allocations they do. The total
allocation of the Jazz node is the sum of all (plus some small amount used by libraries, etc. that is not dependant on data size).

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

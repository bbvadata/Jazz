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

#define NAME_LENGTH						NAME_SIZE - 1		///< Maximum length of a Name.name
#define MAX_NESTED_CONTAINERS			2					///< (max) sub-container names in a locator (base is resolved to a pointer).
#define MAX_ITEMS_IN_KIND				64					///< The number of items merged into a kind or tuple.


/// Block API (method arguments)

#define FILL_NEW_DONT_FILL				0		///< Don't initialize at all.
#define FILL_NEW_WITH_ZERO				1		///< Initialize with binary zero.
#define FILL_NEW_WITH_NA				2		///< Initialize with the appropriate NA for the cell_type.
#define FILL_WITH_TEXTFILE				3		///< Initialize a tensor with the content of argument p_text in new_jazz_block().
#define FILL_BOOLEAN_FILTER				4		///< Create a boolean (CELL_TYPE_BYTE_BOOLEAN) filter with the values in p_bool_filter.
#define FILL_INTEGER_FILTER				5		///< Create an integer (CELL_TYPE_INTEGER) filter with the values in p_bool_filter.

#define BUILD_TUPLE						1		///< Build a Tuple out of data items or fail.
#define BUILD_KIND						2		///< Build a Kind out of metadata items or fail.

/// Block API (error and status codes)

#define BLOCK_STATUS_READY				 0		///< BlockKeeper.status: p_block-> is safe to use

/// Thread safety

#define LOCK_NUM_RETRIES_BEFORE_YIELD	100		///< Number of retries when lock fails before calling this_thread::yield()

/// sqrt(2^31) == # simultaneous readers to outweight a writer == # simultaneous writers to force an overflow
#define LOCK_WEIGHT_OF_WRITE			46341


/** A binary block identifier internal to the Container. Typically a MurmurHash64A of the Block name.
*/
typedef uint64_t BlockId64;

/** An atomically increased (via fetch_add() and fetch_sub()) 32 bit signed integer to use as a lock.
*/
typedef std::atomic<int32_t> Lock32;

// Forward pointer types:

typedef struct Names		*pNames;
typedef struct BlockKeeper 	*pBlockKeeper;
typedef class  Container	*pContainer;
typedef struct Locator		*pLocator, *pL_value;
typedef struct R_value		*pR_value;
typedef struct Items		*pItems;

/** A map of names for the containers (or structure engines like "map" or "tree" inside Volatile).
*/
typedef std::map<std::string, pContainer> BaseNames;

/** A pair of pointers to manage allocation inside an array of BlockKeeper as a deque.
*/
struct OneShotDeque {
	pBlockKeeper	p_prev, p_next;		///< A pair of pointers to keep this (the descendant) in a double linked list
};

/** An array of Item names (used to select items in a Tuple).
*/
struct Names {
	Name name[0];		///< The item names. First zero breaks.
};

This minimalist struc is the only block wrapper across anything. Anything is: file I/O, http client CRUD, http server GET and PUT, shell
commands, Volatile, Persisted and Index objects (an stdlib map that serializes to and from a block).

Transaction allocation is only handled by the owner.
*/
struct Transaction {
	pBlock			p_block;	///< A pointer to the Block (if status == BLOCK_STATUS_READY)
	pBlock			p_route;	///< Anything defining the transaction as a (fixed sized) block allocated in an array inside the owner
	Lock32			_lock_;		///< An atomically updated int to lock the Keeper to support modifying the Block
	int				status;		///< The status of the block transaction
	pContainer		p_owner;	///< A pointer to the Container instance serving API calls related to this block
};
typedef Transaction *pTransaction;


/** \brief Container: A Service to manage Jazz blocks. All Jazz blocks are managed by this or a descendant of this.

This is the root class for all containers. It has memory alloc for one-shot block allocation and methods for filtering and serialization.
Its descendants are: Volatile, Remote and Persisted, completing all possible block allocations: one-shot, volatile and persisted.

It follows the "rules of the game" using:

- Name, Answer and ContractRequest
- BlockKeeper, Locator
- All the constants, error codes, etc.

It provides a neat API for all descendants, including:

- Transparent thread safety .enter_read() .enter_write() .leave_read() .leave_write() .lock_container() .unlock_container()
- An API for async calls (Remote): .callback()
- Allocation: .new_block(), .unlock()
- Crud: .put(), .remove()
- Support for contracts: .get()
- Support for container names in the API .base_names()
- A configuration style for all descendants

It provides, exposed by the root Container class, but also used internally by descendants;

- A transparent deque of BlockKeeper structures

One-shot Block allocation
-------------------------

This is the only container providing one-shot Block allocation. This does not mean, unlike in previous versions, the caller owns the
pointer. One-shot Block allocation is intended for computing intermediate blocks like: blocks generated from constants, from slicing,
returned by functions, etc. In these blocks, the locator is meaningless, the caller will create it by new_block(), use it and call
unlock() when done.

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
	   ~Container ();

	   // Service API

		StatusCode start	   ();
		StatusCode shut_down   ();

		// .enter_read() .enter_write() .leave_read() .leave_write() .lock_container() .unlock_container()

		void enter_read		   (pBlockKeeper  p_keeper);
		void enter_write	   (pBlockKeeper  p_keeper);
		void leave_read		   (pBlockKeeper  p_keeper);
		void leave_write	   (pBlockKeeper  p_keeper);

		// - Allocation: .new_block(), .unlock()

		StatusCode new_block   (pBlockKeeper &p_keeper,
								int			  cell_type,
								int			 *dim,
								AttributeMap *att			  = nullptr,
								int			  fill_tensor	  = FILL_NEW_DONT_FILL,
								bool		 *p_bool_filter	  = nullptr,
								int			  stringbuff_size = 0,
								const char	 *p_text		  = nullptr,
								char		  eol			  = '\n');

		StatusCode new_block   (pBlockKeeper &p_keeper,
								pItems		  p_item,
								pNames		  p_item_name,
						   		int			  build			  = BUILD_TUPLE,
								AttributeMap *att			  = nullptr);

		StatusCode new_block   (pBlockKeeper &p_keeper,
								pBlock		  p_from,
						   		pBlock		  p_row_filter,
								AttributeMap *att			  = nullptr);

		StatusCode new_block   (pBlockKeeper &p_keeper,
								pBlock		  p_block,
								pNames		  p_item_name,
								AttributeMap *att			  = nullptr);

		StatusCode new_block   (pBlockKeeper &p_keeper,
								pChar		 &p_source,
						   		pBlock		  p_as_block	  = nullptr,
								AttributeMap *att			  = nullptr);

		StatusCode new_block   (pBlockKeeper &p_keeper,
								pBlock		  p_block,
						   		int			  format		  = AS_BEBOP,
								AttributeMap *att			  = nullptr);

		void unlock			   (pBlockKeeper &p_keeper);

		// Crud: .put(), .remove()

		StatusCode put		   (pLocator	  p_where,
								pBlock		  p_block,
								pContainer	  p_sender		 = nullptr,
								BlockId64	  block_id		 = 0);

		StatusCode remove	   (pLocator	  p_what,
								pContainer	  p_sender		 = nullptr,
								BlockId64	  block_id		 = 0);

		// Support for contracts: .get()

		StatusCode get		   (pBlockKeeper &p_keeper,
								pR_value	  p_rvalue,
								pContainer	  p_sender		 = nullptr,
								BlockId64	  block_id		 = 0);

		StatusCode get		   (pBlockKeeper &p_keeper,
								pLocator	  p_what);

		// Support for container names in the API .base_names()

		void base_names		   (BaseNames 	 &base_names);

		// Async calls (Remote): .callback()

		virtual void callback  (BlockId64	  block_id,
								StatusCode	  result);

#ifndef CATCH_TEST
	private:
#endif

		/** A private hard lock for Container-critical operations. E.g., Adding a new block to the deque.

			Needeless to say: Use only for a few clockcycles over the critical part and always unlock_container() no matter what.
		*/
		inline void lock_container () {
			int		retry = 0;
			int32_t lock = 0;
			int32_t next = 1;

			while (true) {
				if (_lock_.compare_exchange_weak(lock, next))
					return;

				if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
					std::this_thread::yield();
					retry = 0;
				}
			}
		}

		/** Release the private hard lock for Container-critical operations.
		*/
		void unlock_container () {
			_lock_ = 0;
		}

		/** Allocate a BlockKeeper to share a block via the API.
		*/
		inline StatusCode new_keeper (pBlockKeeper &p_keeper) {
			if (alloc_bytes > warn_alloc_bytes & !alloc_warning_issued) {
				log_printf(LOG_WARN, "Service Container exceeded RAM %0.2f Mb of %0.2f Mb",
						   (double) alloc_bytes/ONE_MB, (double) warn_alloc_bytes/ONE_MB);
				alloc_warning_issued = true;
			}

			lock_container();

			if (p_free == nullptr) {
				unlock_container();
				p_keeper = nullptr;
				return SERVICE_ERROR_NO_MEM;
			}

			p_keeper = p_free;
			p_free	 = p_free->data.deque.p_next;

			p_keeper->p_block = nullptr;
			p_keeper->status  = BLOCK_STATUS_EMPTY;
			p_keeper->_lock_  = 0;

			p_keeper->data.deque.p_next = p_alloc;
			p_keeper->data.deque.p_prev = nullptr;

			if (p_alloc != nullptr)
				p_alloc->data.deque.p_prev = p_keeper;

			p_alloc = p_keeper;

			unlock_container();
			return SERVICE_NO_ERROR;
		}

		/** Dealloc the Block in the keeper (if not null) and free the BlockKeeper API.
		*/
		inline void destroy_keeper (pBlockKeeper &p_keeper) {
			if (p_keeper->p_block != nullptr) {
				alloc_bytes -= p_keeper->p_block->total_bytes;
				free(p_keeper->p_block);
				p_keeper->p_block = nullptr;
			}

			lock_container();

			if (p_keeper->data.deque.p_prev == nullptr)
				p_alloc = p_keeper->data.deque.p_next;
			else
				p_keeper->data.deque.p_prev->data.deque.p_next = p_keeper->data.deque.p_next;

			if (p_keeper->data.deque.p_next != nullptr)
				p_keeper->data.deque.p_next->data.deque.p_prev = p_keeper->data.deque.p_prev;

			p_keeper->data.deque.p_next = p_free;

			p_free	 = p_keeper;
			p_keeper = nullptr;

			unlock_container();
		}

		/** An std::malloc() that increases .alloc_bytes on each call and fails on overcommit.
		*/
		inline void* malloc (size_t size) {
			if (alloc_bytes + size >= fail_alloc_bytes)
				return nullptr;
			void * ret = std::malloc(size);
			if (ret != nullptr)
				alloc_bytes += size;
			return ret;
		}

		StatusCode new_container	();
		StatusCode destroy_container();

		int max_num_keepers;
		uint64_t alloc_bytes, warn_alloc_bytes, fail_alloc_bytes;
		bool alloc_warning_issued;
		pBlockKeeper p_buffer, p_alloc, p_free;
		Lock32 _lock_;
};

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CONTAINER

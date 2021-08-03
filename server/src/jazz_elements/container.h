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
#define MAX_SIZE_OF_CELL_AS_TEXT		48					///< What an integer, bool, float or date can take as text

/// Serialization CONST
#define NA_AS_TEXT						{"NA\0"}			///< A constant representing NA in all types supporting it.
#define LENGTH_NA_AS_TEXT				2					///< The length of the sequence NA without the trailing zero.

/// Serialization: parser error codes
#define PARSE_ERROR_UNEXPECTED_EOF		101		///< Parsing error, end of input buffer found before complete parsing.
#define PARSE_ERROR_UNEXPECTED_CHAR		102		///< Parsing error, mandatory character mismatch.
#define PARSE_ERROR_ITEM_NAME			103		///< Parsing error, failed to get an item name.
#define PARSE_ERROR_ITEM_NAME_MISMATCH	104		///< Parsing error, item name does not match name in p_as_kind.
#define PARSE_ERROR_TENSOR_EXPLORATION	105		///< Parsing error, parsing shape and size of a tensor failed.

#define PARSE_ERROR_KIND_EXPLORATION	106		///< Parsing error, parsing type and shape of a kind failed.
#define PARSE_ERROR_TOO_MANY_ITEMS		107		///< Parsing error, kind of tuple have too many items.
#define PARSE_ERROR_EXPECTED_EOF		108		///< Parsing error, non-space characters found after complete parsing.
#define PARSE_ERROR_TENSOR_FILLING		109		///< Parsing error, while filling a tensor.
#define PARSE_ERROR_TEXT_FILLING		110		///< Parsing error, while deserializing a tensor of text.

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
#define BLOCK_STATUS_READY				 0		///< Transaction.status: p_block-> is safe to use
#define BLOCK_STATUS_EMPTY				 1		///< BlockKeeper.status: successful new_keeper() and new_block() or get() in progress.

/// Thread safety
#define LOCK_NUM_RETRIES_BEFORE_YIELD	100		///< Number of retries when lock fails before calling this_thread::yield()

/// sqrt(2^31) == # simultaneous readers to outweight a writer == # simultaneous writers to force an overflow
#define LOCK_WEIGHT_OF_WRITE			46341


/// An atomically increased (via fetch_add() and fetch_sub()) 32 bit signed integer to use as a lock.
typedef std::atomic<int32_t> Lock32;


/// A (forward defined) pointer to a Container
typedef class Container *pContainer;


/// A map of names for the containers (or structure engines like "map" or "tree" inside Volatile).
typedef std::map<std::string, pContainer> BaseNames;


/** \brief Transaction: A wrapper over a Block that defines the communication of a block over get/put/remove/copy.

This minimalist struc is the only block wrapper across anything. Anything is: file I/O, http client CRUD, http server GET and PUT, shell
commands, Volatile, Persisted and Index objects (an stdlib map that serializes to and from a block).

Transaction allocation is only handled by the owner, a Container or descendant.
*/
struct Transaction {
	union {
		pBlock			p_block;	///< A pointer to the Block (if status == BLOCK_STATUS_READY) for Tensor, Kind and Tuple
		pBlockHeader	p_hea;		///< A pointer to the Block (if status == BLOCK_STATUS_READY) for Index
	};
	pBlock			p_route;	///< Anything defining the transaction as a (fixed sized) block allocated in an array inside the owner
	Lock32			_lock_;		///< An atomically updated int to lock the Keeper to support modifying the Block
	int				status;		///< The status of the block transaction
	pContainer		p_owner;	///< A pointer to the Container instance serving API calls related to this block
};
typedef Transaction *pTransaction;


/// An internal (for Container) Transaction with pointers for a deque
struct StoredTransaction: Transaction {
	StoredTransaction *p_prev, *p_next;
};
typedef StoredTransaction *pStoredTransaction;


/** \brief Container: A Service to manage Jazz blocks. All Jazz blocks are managed by this or a descendant of this.

This is the root class for all containers. It is basically an abstract class with some helpful methods but is not instanced as an object.
Its descendants are: Channels, Volatile and Persisted (in jazz_elements) + anything allocating RAM, Bebop, Agency, and the Api.

There is no class Channel (in singular), copy() is a method that copies blocks across Containers (or different media in Channels).
Channels does all the block transactions across media (files, folders, shell, urls, other Containers, ...).

Container provides a neat API for all descendants, including:

- Transparent thread safety .enter_read() .enter_write() .leave_read() .leave_write() .lock_container() .unlock_container()
- Allocation: .new_block(), .destroy()
- Crud: .get(), .put(), .remove(), .copy()
- Support for container names in the API .base_names()
- A configuration style for all descendants

It provides, exposed by the root Container class, but also used internally by descendants;

- A transparent deque of Transaction structures

One-shot Block allocation
-------------------------

This is the only container providing one-shot Block allocation. This does not mean, unlike in previous versions, the caller owns the
pointer. One-shot Block allocation is intended for computing intermediate blocks like: blocks generated from constants, from slicing,
returned by functions, etc. In these blocks, the locator is meaningless, the caller will create it by new_block(), use it and call
destroy() when done.

Instances and inheritance
-------------------------

Note that the descendants don't inherit each other, but they all have the basic deque mechanism inherited from Container: i.e., they
can all allocate new blocks for their own purposes. Configuration-wise the allocations set by ONE_SHOT_MAX_KEEPERS,
ONE_SHOT_WARN_BLOCK_KBYTES, ... only apply to the instance of the class Container, the descendants use their own limits in which they
include this allocation combined with whatever other allocations they do. The total allocation of the Jazz node is the sum of all (plus
some small amount used by libraries, etc. that is not dependant on data size).

Container descendants
---------------------

Note that Continers own their Transactions and may allocate p_route blocks including all kinds of things like session cookies or
credentials for libcurl calls. Any container will reroute a destroy() call to its owner.

Scope of jazz_elements
----------------------

Everything works at binary level, operations on blocks at this level are very simple, just copying, deleting, filtering a Tensor by
(int or bool) indices, filtering a Tuple by item name and support every medium in the more conceivably efficient way.
Serialization (to and from text) is done at the Api level, running code in jazz_bebop and agency (yomi) at jazz_agency.

new_block()
-----------

**NOTE** that new_block() has 7 forms. It is always called new_block() to emphasize that what the funcion does is create a new block (vs.
sharing a pointer to an existing one). Therefore, the container allocates and owns it an requires a destroy() call when no longer needed.
The forms cover all the supported ways to do basic operations like filtering and serializing.

   -# new_block(): Create a Tensor from raw data specifying everything from scratch.
   -# new_block(): Create a Kind or Tuple from arrays of StaticBlockHeader, names, and, in the case of a tuple, Tensors.
   -# new_block(): Create a Tensor by selecting rows (filtering) from another Tensor.
   -# new_block(): Create a Tensor by selecting an item from a Tuple.
   -# new_block(): Create a Tensor, Kind or Tuple from a Text block kept as a Tensor of CELL_TYPE_BYTE of rank == 1.
   -# new_block(): Create a Tensor of CELL_TYPE_BYTE of rank == 1 with a text serialization of a Tensor, Kind or Tuple.
   -# new_block(): Create an empty Index block. It is dynamically allocated, it contains an std:map, and destroy()-ed just like the others.

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

		void enter_read		   (pTransaction  p_txn);
		void enter_write	   (pTransaction  p_txn);
		void leave_read		   (pTransaction  p_txn);
		void leave_write	   (pTransaction  p_txn);

		// - Allocation: .new_block(), .destroy()

		// 1. new_block(): Create a Tensor from raw data specifying everything from scratch.
		StatusCode new_block   (pTransaction	   &p_txn,
								int					cell_type,
								int				   *dim,
								int					fill_tensor		= FILL_NEW_DONT_FILL,
								bool			   *p_bool_filter	= nullptr,
								int					stringbuff_size	= 0,
								const char		   *p_text			= nullptr,
								char				eol				= '\n',
								AttributeMap	   *att				= nullptr);

		// 2. new_block(): Create a Kind or Tuple from arrays of StaticBlockHeader, names, and, in the case of a tuple, Tensors.
		StatusCode new_block   (pTransaction	   &p_txn,
								int					num_items,
								pBlock				p_hea[],
								Name				p_names[],
								pBlock				p_block[],
								AttributeMap	   *dims			= nullptr,
								AttributeMap	   *att				= nullptr);

		// 3. new_block(): Create a Tensor by selecting rows (filtering) from another Tensor.
		StatusCode new_block   (pTransaction	   &p_txn,
								pBlock				p_from,
						   		pBlock				p_row_filter,
								AttributeMap	   *att				= nullptr);

		// 4. new_block(): Create a Tensor by selecting an item from a Tuple.
		StatusCode new_block   (pTransaction	   &p_txn,
								pTuple				p_from,
						   		pChar				name,
								AttributeMap	   *att				= nullptr);

		// 5. new_block(): Create a Tensor, Kind or Tuple from a Text block kept as a Tensor of CELL_TYPE_BYTE of rank == 1.
		StatusCode new_block   (pTransaction	   &p_txn,
								pBlock				p_from_text,
						   		int					cell_type,
								pKind				p_as_kind		= nullptr,
								AttributeMap	   *att				= nullptr);

		// 6. new_block(): Create a Tensor of CELL_TYPE_BYTE of rank == 1 with a text serialization of a Tensor, Kind or Tuple.
		StatusCode new_block   (pTransaction	   &p_txn,
								pBlock				p_from_raw,
						   		pChar				p_fmt			= nullptr,
								AttributeMap	   *att				= nullptr);

		// 7. new_block(): Create an empty Index block.
		StatusCode new_block   (pTransaction	   &p_txn,
								int					cell_type);

		void destroy		   (pTransaction &p_txn);

		// Crud: .get(), .put(), .remove()

		StatusCode get		   (pChar		  p_what,
								pTransaction &p_txn);
		StatusCode put		   (pBlock		  p_block,
								pChar		  p_where);
		StatusCode remove	   (pChar		  p_what);
		StatusCode copy		   (pChar		  p_what,
								pChar		  p_where);

		// Support for container names in the API .base_names()

		void base_names		   (BaseNames 	 &base_names);

#ifndef CATCH_TEST
	private:
#endif

		char HEX[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

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

		/** Allocate a Transaction to share a block via the API.
		*/
		inline StatusCode new_transaction (pTransaction &p_txn) {
			if (alloc_bytes > warn_alloc_bytes & !alloc_warning_issued) {
				log_printf(LOG_WARN, "Service Container exceeded RAM %0.2f Mb of %0.2f Mb",
						   (double) alloc_bytes/ONE_MB, (double) warn_alloc_bytes/ONE_MB);
				alloc_warning_issued = true;
			}

			lock_container();

			if (p_free == nullptr) {
				unlock_container();
				p_txn = nullptr;
				return SERVICE_ERROR_NO_MEM;
			}

			p_txn  = p_free;
			p_free = p_free->p_next;

			p_txn->p_block = nullptr;
			p_txn->p_route = nullptr;
			p_txn->status  = BLOCK_STATUS_EMPTY;
			p_txn->_lock_  = 0;
			p_txn->p_owner = this;

			pStoredTransaction(p_txn)->p_next = p_alloc;
			pStoredTransaction(p_txn)->p_prev = nullptr;

			if (p_alloc != nullptr)
				p_alloc->p_prev = pStoredTransaction(p_txn);

			p_alloc = pStoredTransaction(p_txn);

			unlock_container();
			return SERVICE_NO_ERROR;
		}

		/** \brief (UNSAFE) Dealloc the Block in the p_tnx->p_block (if not null) and free the Transaction API.

			This (faster) method assumes the Container owns the Transaction and nobody else is using it. Use destroy() as a safer
			alternative.
		*/
		inline void destroy_internal (pTransaction &p_txn) {
			if (p_txn->p_block != nullptr) {
				switch (p_txn->p_block->cell_type) {
				case CELL_TYPE_INDEX_II:
					p_txn->p_hea->index.index_ii.~map();

					break;

				case CELL_TYPE_INDEX_IS:
					p_txn->p_hea->index.index_is.~map();

					break;

				case CELL_TYPE_INDEX_SI:
					p_txn->p_hea->index.index_si.~map();

					break;

				case CELL_TYPE_INDEX_SS:
					p_txn->p_hea->index.index_ss.~map();
				};

				alloc_bytes -= p_txn->p_block->total_bytes;
				free(p_txn->p_block);
				p_txn->p_block = nullptr;
			}

			lock_container();

			if (pStoredTransaction(p_txn)->p_prev == nullptr)
				p_alloc = pStoredTransaction(p_txn)->p_next;
			else
				pStoredTransaction(p_txn)->p_prev->p_next = pStoredTransaction(p_txn)->p_next;

			if (pStoredTransaction(p_txn)->p_next != nullptr)
				pStoredTransaction(p_txn)->p_next->p_prev = pStoredTransaction(p_txn)->p_prev;

			pStoredTransaction(p_txn)->p_next = p_free;

			p_free = pStoredTransaction(p_txn);
			p_txn  = nullptr;

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
		pStoredTransaction p_buffer, p_alloc, p_free;
		Lock32 _lock_;

		/** Bla,

//TODO: Document this

		*/
		inline int skip_space(pChar &p_in, int &num_bytes) {
			while (num_bytes > 0) {
				if (p_in[0] == ' ' || p_in[0] == '\t') {
					p_in++;
					num_bytes--;
				} else
					break;
			}

			return num_bytes;
		}

		/** Bla,

//TODO: Document this

		*/
		inline char get_char(pChar &p_in, int &num_bytes) {
			if (num_bytes < 1)
				return 0;

			num_bytes--;
			return (p_in++)[0];
		}

		/** Bla,

//TODO: Document this

		*/
		inline bool get_item_name(pChar &p_in, int &num_bytes, pChar p_out) {
			if (skip_space(p_in, num_bytes) <= 0)
				return false;

			if (get_char(p_in, num_bytes) != '"')
				return false;

			char ch = get_char(p_in, num_bytes);

			if (ch < 'A' || ch > 'z' || (ch > 'Z' && ch < 'a'))
				return false;

			(p_out++)[0] = ch;

			for (int i = 1; i < NAME_SIZE; i++) {
				ch = get_char(p_in, num_bytes);

				if (ch == '"')
					break;

				if (ch < '0' || ch > 'z' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < '_') || ch == 0x60)
					return false;

				(p_out++)[0] = ch;
			}

			if (ch != '"' && (get_char(p_in, num_bytes) != '"'))
				return false;

			if (skip_space(p_in, num_bytes) <= 0)
				return false;

			if (get_char(p_in, num_bytes) != ':')
				return false;

			return true;
		}

		/** Bla,

//TODO: Document this

		*/
		inline int buff_size(ItemHeader &item_hea) {
			int ret = item_hea.dim[0];

			for (int i = 1; i < item_hea.rank; i++)
				ret *= item_hea.dim[i];

			return ret + item_hea.item_size + 1;
		}

		bool get_shape_and_size	 (pChar &p_in, int &num_bytes, int cell_type, ItemHeader *item_hea);
		bool get_type_and_shape	 (pChar &p_in, int &num_bytes, ItemHeader *item_hea, AttributeMap *dims);
		bool fill_text_buffer	 (pChar &p_in, int &num_bytes, pChar p_out);
		bool fill_tensor		 (pChar &p_in, int &num_bytes, pBlock p_block);

		int tensor_int_as_text	 (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_bool_as_text	 (pBlock p_block, pChar p_dest);
		int tensor_float_as_text (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_string_as_text(pBlock p_block, pChar p_dest);
		int tensor_time_as_text	 (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_tuple_as_text (pTuple p_tuple, pChar p_dest, pChar p_fmt, int item_len[]);
		int tensor_kind_as_text	 (pKind  p_kind,  pChar p_dest);

		inline void opening_brackets(int rank, pChar &p_ret) {
			for (int i = 0; i < rank; i++)
				(p_ret++)[0] = '[';

			p_ret[0] = 0;
		}

		inline void as_shape(int rank, pChar p_ret, int dim[], pKind p_kind) {
			(p_ret++)[0] = '[';

			for (int i = 0; i < rank; i++) {
				int k = dim[i];
				if (k < 0) {
					char *p_dim = &p_kind->p_string_buffer()->buffer[-k];
					p_ret += sprintf(p_ret, "%i", k);

				} else
					p_ret += sprintf(p_ret, "%i", k);

				if (i < rank - 1) {
					(p_ret++)[0] = ',';
					(p_ret++)[0] = ' ';
				}
			}

			(p_ret++)[0] = ']';

			p_ret[0] = 0;
		}

		inline void as_hex(pChar &p_dest, char bl) {
			(p_dest++)[0] = '\\';
			(p_dest++)[0] = 'x';
			(p_dest++)[0] = HEX[bl & 0xf0 >> 4];
			(p_dest++)[0] = HEX[bl & 0x0f];
		}

		inline void separator(int rank_1, int shape[], int idx[], pChar &p_ret) {
			for (int i = rank_1; i >= 0; i --) {
				idx[i]++;

				if (idx[i] == shape[i]) {
					idx[i] = 0;
					(p_ret++)[0] = ']';
				} else {
					(p_ret++)[0] = ',';
					(p_ret++)[0] = ' ';

					for (int j = 0; j < rank_1 - i; j++)
						(p_ret++)[0] = '[';

					break;
				}
			}
			p_ret[0] = 0;
		}

		inline int separator_len(int rank_1, int shape[], int idx[]) {
			int ret = 0;

			for (int i = rank_1; i >= 0; i --) {
				idx[i]++;

				if (idx[i] == shape[i]) {
					idx[i] = 0;
					ret++;
				} else {
					ret += 2;

					if (i == rank_1)
						return ret;

					ret += rank_1 - i;

					return ret;
				}
			}
			return ret;
		}
};

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CONTAINER

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
#define FILL_NEW_DONT_FILL				  0		///< Don't initialize at all.
#define FILL_NEW_WITH_ZERO				  1		///< Initialize with binary zero.
#define FILL_NEW_WITH_NA				  2		///< Initialize with the appropriate NA for the cell_type.
#define FILL_WITH_TEXTFILE				  3		///< Initialize a tensor with the content of argument p_text in new_jazz_block().
#define FILL_BOOLEAN_FILTER				  4		///< Create a boolean (CELL_TYPE_BYTE_BOOLEAN) filter with the values in p_bool_filter.
#define FILL_INTEGER_FILTER				  5		///< Create an integer (CELL_TYPE_INTEGER) filter with the values in p_bool_filter.

#define BUILD_TUPLE						  1		///< Build a Tuple out of data items or fail.
#define BUILD_KIND						  2		///< Build a Kind out of metadata items or fail.

/// Block API (error and status codes)
#define BLOCK_STATUS_READY				  0		///< Transaction.status: p_block-> is safe to use
#define BLOCK_STATUS_EMPTY				  1		///< Transaction.status: successful new_transaction() and new_block() or get() in progress.

/// Thread safety
#define LOCK_NUM_RETRIES_BEFORE_YIELD	100		///< Number of retries when lock fails before calling this_thread::yield()

/// sqrt(2^31) == # simultaneous readers to outweight a writer == # simultaneous writers to force an overflow
#define LOCK_WEIGHT_OF_WRITE			46341

// State based parser types:
#define EIGHT_BIT_LONG					256		///< Length of a NextStateLUT.
#define MAX_TRANSITION_REGEX_LEN		 32		///< Length of regex for state transitions. Used only in constants for LUT construction.


/** A lookup table for all the possible values of a char mapped into an 8-bit state.
*/
struct ParseNextStateLUT {
	unsigned char next[EIGHT_BIT_LONG];
};


/** A way to build constants defining the transtition from one state to the next via a regex.
*/
struct ParseStateTransition {
	int  from;
	int	 to;
	char rex[MAX_TRANSITION_REGEX_LEN];
};


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
	Lock32				_lock_;		///< An atomically updated int to lock the Transaction to support modifying the Block
	int					status;		///< The status of the block transaction
	pContainer			p_owner;	///< A pointer to the Container instance serving API calls related to this block
};
typedef Transaction *pTransaction;


/** \brief Locator: A minimal structure to define the location of resources inside a Container.

This is used by all Container descendants, it can be extended using p_extra to something else. E..g, a UniversalLocator
*/
struct Locator {
	char base	[SHORT_NAME_SIZE];	///< A Jazz node level unique name to locate a Container and possibly a type of service inside it.
	char entity	[NAME_SIZE];		///< Another abstraction inside node.container.base, like the name of a table in a database.
	char key	[NAME_SIZE];		///< A key identifying a block inside the entity.

	pChar p_extra;					///< A pointer to extend this structure with Container specific data (like URLs, cookies, credentials).
};


/// An internal (for Container) Transaction with pointers for a deque
struct StoredTransaction: Transaction {
	StoredTransaction *p_prev, *p_next;
};
typedef StoredTransaction *pStoredTransaction;


/** \brief Container: A Service to manage Jazz blocks. All Jazz blocks are managed by this or a descendant of this.

This is the root class for all containers. It is basically an abstract class with some helpful methods but is not instanced as an object.
Its descendants are: Channels, Volatile and Persisted (in jazz_elements) + anything allocating RAM, Bebop, Agency, and the Api.

There is no class Channel (in singular), copy() is a method that copies blocks across Containers (or different media in Channels).
Channels does all the block transactions across media (files, folders, shell, urls, zeroMQ pipes, Index types, ...).

Container provides a neat API for all descendants, including:

- Transparent thread safety .enter_read() .enter_write() .leave_read() .leave_write() .lock_container() .unlock_container()
- Allocation: .new_block(), .destroy()
- Crud: .get(), .header(), .put(), .new_entity(), .remove(), .copy()
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
can all allocate new blocks for their own purposes. Configuration-wise the allocations set by ONE_SHOT_MAX_TRANSACTIONS,
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
								StaticBlockHeader	p_hea[],
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

		void destroy		   (pTransaction	   &p_txn);

		// Crud: .get(), .put(), .remove()

		StatusCode get		   (pTransaction	   &p_txn,
								pChar				p_what);
		StatusCode put		   (pChar				p_where,
								pBlock				p_block);
		StatusCode remove	   (pChar				p_what);
		StatusCode copy		   (pChar				p_where,
								pChar				p_what);

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
					alloc_bytes -= sizeof(BlockHeader);

					break;

				case CELL_TYPE_INDEX_IS:
					p_txn->p_hea->index.index_is.~map();
					alloc_bytes -= sizeof(BlockHeader);

					break;

				case CELL_TYPE_INDEX_SI:
					p_txn->p_hea->index.index_si.~map();
					alloc_bytes -= sizeof(BlockHeader);

					break;

				case CELL_TYPE_INDEX_SS:
					p_txn->p_hea->index.index_ss.~map();
					alloc_bytes -= sizeof(BlockHeader);

					break;

				default:
					alloc_bytes -= p_txn->p_block->total_bytes;
				};
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

		/** A spacial alloc for blocks owned by a Transaction. It clears cell_type and total_bytes assumed to be valid by destroy().
		*/
		inline pBlock block_malloc(size_t size) {
			pBlock p_blk = (pBlock) malloc(size);

			if (p_blk != nullptr) {
				p_blk->cell_type   = 0;
				p_blk->total_bytes = 0;
			}

			return p_blk;
		}

		StatusCode new_container	();
		StatusCode destroy_container();

		int max_transactions;
		uint64_t alloc_bytes, warn_alloc_bytes, fail_alloc_bytes;
		bool alloc_warning_issued;
		pStoredTransaction p_buffer, p_alloc, p_free;
		Lock32 _lock_;

		/** Skip space or tab characterr while parsing

			\param p_in			The input char stream cursor.
			\param num_bytes	The number of bytes with data above *p_in

			\return	The number of bytes still to be read after skipping
		*/
		inline int skip_space(pChar &p_in, int &num_bytes) {
			while (num_bytes > 0) {
				if (*p_in == 0)
					return 0;

				if (*p_in == ' ' || *p_in == '\t') {
					p_in++;
					num_bytes--;
				} else
					break;
			}

			return num_bytes;
		}

		/** Returns the char at input cursor shifting the cursor by one or zero if there are no mor characters to be read.

			\param p_in			The input char stream cursor.
			\param num_bytes	The number of bytes with data above *p_in

			\return	The char read
		*/
		inline char get_char(pChar &p_in, int &num_bytes) {
			if (num_bytes < 1)
				return 0;

			num_bytes--;

			return *(p_in++);
		}

		/** Returns the char at input cursor shifting the cursor by one or zero if there are no mor characters to be read.

			\param p_in			The input char stream cursor.
			\param num_bytes	The number of bytes with data above *p_in
			\param p_out		The Name buffer that gets the answer if no error was found
			\param check_quotes	Expects the name to be quoted (like item names) or not (like dimensions)
			\param check_colon	Expects the name to be followed by a colon (like item names).

			\return	True on success
		*/
		inline bool get_item_name(pChar &p_in, int &num_bytes, pChar p_out, bool check_quotes = true, bool check_colon = true) {
			if (skip_space(p_in, num_bytes) <= 1)
				return false;

			if (check_quotes && (get_char(p_in, num_bytes) != '"'))
				return false;

			char ch = get_char(p_in, num_bytes);

			if (ch < 'A' || ch > 'z' || (ch > 'Z' && ch < 'a'))
				return false;

			*(p_out++) = ch;

			for (int i = 1; i < NAME_LENGTH; i++) {
				if (!check_quotes && (*p_in == ',' || *p_in == ']'))
					break;

				ch = get_char(p_in, num_bytes);

				if (check_quotes && ch == '"')
					break;

				if (ch < '0' || ch > 'z' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < '_') || ch == 0x60)
					return false;

				*(p_out++) = ch;
			}

			*p_out = 0;

			if (check_quotes && (ch != '"') && (get_char(p_in, num_bytes) != '"'))
				return false;

			if (check_colon) {
				if (skip_space(p_in, num_bytes) <= 1)
					return false;

				if (get_char(p_in, num_bytes) != ':')
					return false;
			}

			return true;
		}

		/** Read an integer from the input cursor

			\param p_in			The input char stream cursor.
			\param num_bytes	The number of bytes with data above *p_in
			\param result		The result

			\return	True on success
		*/
		inline bool sscanf_int32(pChar &p_in, int &num_bytes, int &result) {
			int r_len;

			if (sscanf(p_in, "%i%n", &result, &r_len) > 0) {
				p_in	  += r_len;
				num_bytes -= r_len;

				return num_bytes >= 0;
			} else
				return false;
		}

		/** Pushes a decimal representation of an int into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_int_cell(pChar cell, pChar &p_st, int * &p_out) {

			if (p_st == cell) {
				*(p_out++) = INTEGER_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			if (sscanf(p_st, "%i", p_out) != 1)
				return false;

			p_out++;

			return true;
		}

		/** Pushes a decimal representation of a long int into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_int_cell(pChar cell, pChar &p_st, long long * &p_out) {

			if (p_st == cell) {
				*(p_out++) = LONG_INTEGER_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			if (sscanf(p_st, "%lli", p_out) != 1)
				return false;

			p_out++;

			return true;
		}

		/** Pushes a decimal representation of an 8 bit bool into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_bool_cell(pChar cell, pChar &p_st, bool * &p_out) {

			if (p_st == cell) {
				*reinterpret_cast<uint8_t *>(p_out++) = BYTE_BOOLEAN_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			*(p_out++) = cell[0] == '1';

			return true;
		}

		/** Pushes a decimal representation of a 32 bit bool into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_bool_cell(pChar cell, pChar &p_st, uint32_t * &p_out) {

			if (p_st == cell) {
				*(p_out++) = BOOLEAN_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			*(p_out++) = cell[0] == '1';

			return true;
		}

		/** Pushes a decimal representation of a 32 bit float into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_real_cell(pChar cell, pChar &p_st, float * &p_out) {

			if (p_st == cell) {
				*(p_out++) = SINGLE_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			if (sscanf(p_st, "%f", p_out) != 1)
				return false;

			p_out++;

			return true;
		}

		/** Pushes a decimal representation of a 64 bit float into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor

			\return	True on success
		*/
		inline bool push_real_cell(pChar cell, pChar &p_st, double * &p_out) {

			if (p_st == cell) {
				*(p_out++) = DOUBLE_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			if (sscanf(p_st, "%lf", p_out) != 1)
				return false;

			p_out++;

			return true;
		}

		/** Pushes a decimal representation of a time point into a tensor cell in a block.

			\param cell		The fixed sized buffer storing the string (actively written by p_st).
			\param p_st		The cursor writing to cell. In case of a NA, it will be moved to &cell to clear.
			\param p_out	A pointer to the cell in the tensor
			\param fmt		The address of DEF_FLOAT_TIME passed to this inline function

			\return	True on success
		*/
		inline bool push_time_cell(pChar cell, pChar &p_st, time_t * &p_out, pChar fmt) {

			if (p_st == cell) {
				*(p_out++) = TIME_POINT_NA;

				return true;
			}
			*p_st = 0;
			p_st  = cell;

			struct tm timeinfo = {0};

			if (strptime(p_st, fmt, &timeinfo) == nullptr)
				return false;

			time_t xx = timegm(&timeinfo);
			if (xx < 0)
				return false;

			*(p_out++) = xx;

			return true;
		}

		bool get_type_and_shape	 (pChar &p_in, int &num_bytes, ItemHeader *item_hea, IndexSI &dims);
		bool get_shape_and_size	 (pChar &p_in, int &num_bytes, int cell_type, ItemHeader *item_hea);
		bool fill_text_buffer	 (pChar &p_in, int &num_bytes, pChar p_out, int num_cells, int is_NA[], int hasLN[]);
		bool fill_tensor		 (pChar &p_in, int &num_bytes, pBlock p_block);

		int new_text_block		 (pTransaction &p_txn, ItemHeader &item_hea, pChar &p_in, int &num_bytes, AttributeMap *att);

		int tensor_int_as_text	 (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_bool_as_text	 (pBlock p_block, pChar p_dest);
		int tensor_float_as_text (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_string_as_text(pBlock p_block, pChar p_dest);
		int tensor_time_as_text	 (pBlock p_block, pChar p_dest, pChar p_fmt);
		int tensor_tuple_as_text (pTuple p_tuple, pChar p_dest, pChar p_fmt, int item_len[]);
		int tensor_kind_as_text	 (pKind  p_kind,  pChar p_dest);

		/** Writes opening brackets for a tensor

			\param rank		The tensor rank
			\param p_ret	The cursor to the output buffer
		*/
		inline void opening_brackets(int rank, pChar &p_ret) {
			for (int i = 0; i < rank; i++)
				*(p_ret++) = '[';

			*p_ret = 0;
		}

		/** Writes the shape of a Tensor in a Kind

			\param rank		The tensor rank
			\param p_ret	The cursor to the output buffer
			\param dim		The shape
			\param p_kind	The kind from which dimension names should be read
		*/
		inline pChar as_shape(int rank, int dim[], pChar p_ret, pKind p_kind) {
			*(p_ret++) = '[';

			for (int i = 0; i < rank; i++) {
				int k = dim[i];
				if (k < 0) {
					char *p_dim_name = &p_kind->p_string_buffer()->buffer[-k];
					strcpy(p_ret, p_dim_name);
					p_ret += strlen(p_dim_name);

				} else
					p_ret += sprintf(p_ret, "%i", k);

				if (i < rank - 1) {
					*(p_ret++) = ',';
					*(p_ret++) = ' ';
				}
			}

			*(p_ret++) = ']';

			*p_ret = 0;

			return p_ret;
		}

		/** Writes a char as 0xFF

			\param p_dest	The cursor to the output buffer
			\param bl		The character
		*/
		inline void as_hex(pChar &p_dest, uint8_t bl) {
			*(p_dest++) = '\\';
			*(p_dest++) = 'x';
			*(p_dest++) = HEX[bl >> 4];
			*(p_dest++) = HEX[bl & 0x0f];
		}

		/** Returns the binary value of a hex char assuming it is in range.

			\param c	The character which is either 0-9, a-f or A-F
			\return		The binary value of the char
		*/
		inline int from_hex(char c) {
			return (c < 65) ? c - 48 : (c > 96) ? c - 87 : c - 55;
		}

		/** Writes the separator between two cells in a tensor (counting brackets, comma, ..)

			\param rank_1	The tensor rank - 1
			\param dim		The shape
			\param idx		The current index expressed inside the shape
			\param p_ret	The cursor to the output buffer
		*/
		inline void separator(int rank_1, int dim[], int idx[], pChar &p_ret) {
			for (int i = rank_1; i >= 0; i--) {
				idx[i]++;

				if (idx[i] == dim[i]) {
					idx[i] = 0;
					*(p_ret++) = ']';
				} else {
					*(p_ret++) = ',';
					*(p_ret++) = ' ';

					for (int j = 0; j < rank_1 - i; j++)
						*(p_ret++) = '[';

					break;
				}
			}
			*p_ret = 0;
		}

		/** Computes the length of the separator between two cells in a tensor (counting brackets, comma, ..)

			\param rank_1	The tensor rank - 1
			\param dim		The shape
			\param idx		The current index expressed inside the shape

			\return	The length taken by the separator
		*/
		inline int separator_len(int rank_1, int dim[], int idx[]) {
			int ret = 0;

			for (int i = rank_1; i >= 0; i--) {
				idx[i]++;

				if (idx[i] == dim[i]) {
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

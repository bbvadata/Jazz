/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

   2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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


#include <map>
#include <atomic>
#include <thread>
#include <stdarg.h>
#include <regex>


#include "src/jazz_elements/jazz_datablocks.h"
#include "src/jazz_elements/jazz_utils.h"

/**< \brief Container classes for JazzBlock objects.

  JazzBlock objects can be:\n

1. One-shot - Owned by the caller of new_jazz_block()\n
2. Volatile - Owned by some JazzBlockKeepr descendant that is not a JazzPersistence (or descendant)\n
3. Persisted - Owned by a JazzPersistence (or descendant), typically a JazzSource

*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_CONTAINERS
#define INCLUDED_JAZZ_ELEMENTS_CONTAINERS


namespace jazz_containers
{

using namespace jazz_datablocks;

#define JAZZ_MAX_BLOCK_ID_LENGTH								 32		///< Maximum length for a block name
#define JAZZ_REGEX_VALIDATE_BLOCK_ID  "^(/|\\.)[[:alnum:]_]{1,30}$"		///< Regex validating a JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_LOCAL								'.'		///< First char of a LOCAL JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_DISTRIB							'/'		///< First char of a DISTRIBUTED JazzBlockIdentifier
#define JAZZ_LOCK_READING_RETRY_NUMTIMES						100		///< # retries when lock fails (reading) before this_thread::yield();
#define JAZZ_LOCK_WRITING_RETRY_NUMTIMES						100		///< # retries when lock fails (writing) before this_thread::yield();
#define JAZZ_LOCK_KICKING_RETRY_NUMTIMES					   1000		///< # retries when writing ok but with readers before this_thread::yield();
/// (Approx) sqrt(2^31) == # simultaneous readers to outweight a writer == # simultaneous writers to force an overflow
#define JAZZ_LOCK_WEIGHT_OF_WRITE							  46341

/// Values for argument fill_tensor of new_jazz_block()
#define JAZZ_FILL_

#define JAZZ_FILL_NEW_DONT_FILL		0	///< Don't initialize at all.
#define JAZZ_FILL_NEW_WITH_ZERO		1	///< Initialize with binary zero.
#define JAZZ_FILL_NEW_WITH_NA		2	///< Initialize with the appropriate NA for the cell_type.
#define JAZZ_FILL_BOOLEAN_FILTER	3	///< Create a boolean (CELL_TYPE_BYTE_BOOLEAN) filter with the values in p_bool_filter.
#define JAZZ_FILL_INTEGER_FILTER	4	///< Create an integer (CELL_TYPE_INTEGER) filter with the values in p_bool_filter.
#define JAZZ_FILL_WITH_TEXTFILE		5	///< Initialize a tensor with the content of argument p_text in new_jazz_block().

/// Values for argument set_has_NA of close_jazz_block()
#define JAZZ_SET_HAS_NA_

#define JAZZ_SET_HAS_NA_FALSE		0	///< Set to false without checking (unsafe in case there are and not-NA-aware arithmetic is used)
#define JAZZ_SET_HAS_NA_TRUE		1	///< Set to true without checking (safe even if there are, NA-aware arithmetic is always safe)
#define JAZZ_SET_HAS_NA_AUTO		2	///< Check if there are and set accordingly (slowest option when closing, best later)

/// Values for the keys of the attributes in JazzBlock

#define BLOCK_ATTR_SECTION_SIZE									   10000	///< Maximum number of keys per section
#define BLOCK_ATTR_BASEOF_CONTAINERS								   0	///< Base of attribute keys in this module
#define BLOCK_ATTR_CONTAINERS_EMPTY		BLOCK_ATTR_BASEOF_CONTAINERS + 1	///< No attributes defined, creates a <this:empty string>
#define BLOCK_ATTR_CONTAINERS_FILTER	BLOCK_ATTR_BASEOF_CONTAINERS + 2	///< A new filter: creates a <this:empty string>

#define BLOCK_ATTR_BASEOF_PERSISTENCE	BLOCK_ATTR_BASEOF_CONTAINERS  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_persistence.h
#define BLOCK_ATTR_BASEOF_CLASSES		BLOCK_ATTR_BASEOF_PERSISTENCE + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_classes.h
#define BLOCK_ATTR_BASEOF_PRIMITIVES	BLOCK_ATTR_BASEOF_CLASSES	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_primitives.h
#define BLOCK_ATTR_BASEOF_PROCESSCALL	BLOCK_ATTR_BASEOF_PRIMITIVES  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_processcall.h
#define BLOCK_ATTR_BASEOF_CLUSTER		BLOCK_ATTR_BASEOF_PROCESSCALL + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_cluster.h
#define BLOCK_ATTR_BASEOF_COLUMN		BLOCK_ATTR_BASEOF_CLUSTER	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_column.h
#define BLOCK_ATTR_BASEOF_ARCHIVE		BLOCK_ATTR_BASEOF_COLUMN	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_archive.h
#define BLOCK_ATTR_BASEOF_DATAFRAME		BLOCK_ATTR_BASEOF_ARCHIVE	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_dataframe.h
#define BLOCK_ATTR_BASEOF_BEBOP			BLOCK_ATTR_BASEOF_DATAFRAME	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_bebop.h
#define BLOCK_ATTR_BASEOF_RESTAPI		BLOCK_ATTR_BASEOF_BEBOP		  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_restapi.h
#define BLOCK_ATTR_BASEOF_EXTENSIONS	BLOCK_ATTR_BASEOF_RESTAPI	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys from C++ extensions


/** A readable block identifier. It must be a string matching JAZZ_REGEX_VALIDATE_BLOCK_ID. This name is the key identifying
the JazzBlock in a JazzPersistence, JazzSource or via the API source.block (local) source/block (distributed).
*/
struct JazzBlockIdentifier {
	char key[JAZZ_MAX_BLOCK_ID_LENGTH];
};


/** A binary block identifier. It is a MurmurHash64A of the JazzBlockIdentifier computed with JazzBlockKeepr.hash_block_id
*/
typedef uint64_t JazzBlockId64;


typedef struct JazzBlockKeeprItem *pJazzBlockKeeprItem;		///< A pointer to a JazzBlockKeeprItem
typedef struct JazzTreeItem		  *pJazzTreeItem;			///< A pointer to a JazzTreeItem
typedef struct JazzQueueItem	  *pJazzQueueItem;			///< A pointer to a JazzQueueItem


/** All volatile JazzBlock objects are tracked in a double linked list of JazzBlockKeeprItem descendants.
The JazzBlockKeeprItem structure is the minimum to allocate the objects in the list.
*/
struct JazzBlockKeeprItem {
	pJazzBlock			p_jazz_block;						///< A pointer to the JazzBlock
	pJazzBlockKeeprItem	p_alloc_prev, p_alloc_next;			///< A pair of pointers to keep this (the descendant) in a double linked list
	JazzBlockId64		block_id64;							///< Hash of block_id
	JazzBlockIdentifier	block_id;							///< The block ID is a zero-ended C string with strlen() < JAZZ_MAX_BLOCK_ID_LENGTH
};


/** The root class for different JazzTree descendants
*/
struct JazzTreeItem: JazzBlockKeeprItem {
	pJazzTreeItem p_parent, p_first_child, p_next_sibling;	///< Pointers to navigate the tree
	int			  num_visits, num_wins;						///< For simplest case MCTS exploration
};


/** The root class for different AATBlockQueue descendants
*/
struct JazzQueueItem: JazzBlockKeeprItem {
	int		  level;										///< Level in the AA tree (used for auto-balancing)
	int		  times_used;									///< Times the block has been reassigned in the queue
	double	  priority;										///< A priority value to implement a priority queue
	uint64_t  time_to_build;								///< The time required to compute the block (real or estimated) in microseconds
	TimePoint last_used;									///< Timestamp of last use
};


/** A map to search a JazzBlockKeeprItem by the block_id64 of its corresponding JazzBlock. This is the essential element
that converts a queue into a cache. A cache is a queue where elements can be searched by id and lower priority items
can be freed to allocate space.
*/
typedef std::map<JazzBlockId64, pJazzQueueItem> JazzBlockMap;


/** An atomically increased (via fetch_add() and fetch_sub()) 32 bit signed integer to use as a lock.
*/
typedef std::atomic<int32_t> JazzLock;


pJazzBlock new_jazz_block (pJazzBlock	  p_as_block,
						   pJazzFilter	  p_row_filter = nullptr,
						   AllAttributes *att		   = nullptr);


pJazzBlock new_jazz_block (int			  cell_type,
						   int			 *dim,
						   AllAttributes *att			  = nullptr,
						   int			  fill_tensor	  = JAZZ_FILL_NEW_WITH_NA,
						   bool			 *p_bool_filter	  = nullptr,
						   int			  stringbuff_size = 0,
						   const char	 *p_text		  = nullptr,
						   char			  eoln			  = '\n');


/** Set has_NA, the creation time and the hash64 of a JazzBlock based on the content of the tensor

	Despite its name, this function does not actually "close" anything. JazzBlock manipulation is based on "good will",
after calling close_jazz_block() the owner should not change the content. If you do, you should close_jazz_block() again after.

	close_jazz_block() can be called any number of times on the same block.

	\param p_block The block to be "closed".
*/
inline void close_jazz_block(pJazzBlock p_block, int set_has_NA = JAZZ_SET_HAS_NA_AUTO) {
	switch (set_has_NA) {
	case JAZZ_SET_HAS_NA_FALSE:
		p_block->has_NA = false;
		break;
	case JAZZ_SET_HAS_NA_TRUE:
		p_block->has_NA = p_block->cell_type != CELL_TYPE_BYTE;	// CELL_TYPE_BYTE must always be has_NA == FALSE
		break;
	default:
		p_block->has_NA = p_block->find_NAs_in_tensor();
	}

#ifdef DEBUG	// Initialize the RAM between the end of the string buffer and last allocated byte for Valgrind.
	{
		pJazzStringBuffer psb = p_block->p_string_buffer();

		char *pt1 = (char *) &psb->buffer[psb->last_idx],
			 *pt2 = (char *) p_block + p_block->total_bytes;

		while (pt1 < pt2) {
			pt1[0] = 0;
			pt1++;
		}
	}
#endif

	p_block->hash64	 = jazz_utils::MurmurHash64A(&p_block->tensor, p_block->total_bytes - sizeof(JazzBlockHeader));
	p_block->created = std::chrono::steady_clock::now();
}

void free_jazz_block(pJazzBlock &p_block);


/** Convert JazzBlockIdentifier into its corresponding JazzBlockId64.

	\param p_id The JazzBlockIdentifier to be hashed. Note that a nullptr or a string of length 0, are both considered invalid and return
the special JazzBlockId64 0.
	\return The JazzBlockId64 or 0 if invalid.

	Note: Invalid JazzBlockId64 can be normal when a block has not been persisted and only the hash is known (cached results of functions).
*/
inline JazzBlockId64 hash_block_id(const char *p_id) {
	if (p_id == nullptr || p_id[0] == 0)
		return 0;
	return jazz_utils::MurmurHash64A(p_id, strlen(p_id));
}

/** Root class for all JazzBlock containers, including JazzPersistence containers.

JazzBlocks can be allocated in three ways:

1. One-shot - Created with jazz_containers::new_jazz_block(), destroyed with jazz_containers::new_jazz_block(), owned by the caller.
2. Volatile - Created with jazz_containers::JazzBlockKeepr::new_jazz_block() or (JazzBlockKeepr descendant)::new_jazz_block() and not
required to be freed, but can be freed using (JazzBlockKeepr descendant::)free_jazz_block(). Some objects (like cache and priority queues)
may remove JazzBlocks from the container to free the space for higher prioritized JazzBlocks. Volatile containers (those not descending from
JazzPersistence) keep the JazzBlockKeeprItem block containers in local RAM, but they may point to blocks physically stored across a cluster.
In volatile containers alloc_keeprs()/realloc_keeprs()/destroy_keeprs() control local allocation of the JazzBlockKeeprItem buffer.
3. Persisted - Created with jazz_persistence::JazzPersistence::new_jazz_block() or (JazzPersistence descendant::)new_jazz_block() and not
required to be freed, but can be freed using (JazzPersistence descendant::)free_jazz_block(). Unlike volatile JazzBlocks, persisted
JazzBlocks are not controlled by a JazzBlockKeeprItem. Difference between JazzPersistence and JazzSource is the former implements a strict
JazzBlockKeepr interface that can be used from c++ to do things like select information from blocks without assigning or copying them, the
latter has a much simpler interface that is exported to Python and R and provides what a script language programmer would expect at the price
of not always benefiting from the memory-mapped file allocation in LMDB that underlies JazzPersistence.

When an JazzBlockKeepr fills, blocks fail to allocate but never get automatically destroyed.

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class JazzBlockKeepr {

	public:
		 JazzBlockKeepr(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzBlockKeepr();

		// Methods for buffer allocation

		bool alloc_keeprs  (int num_items);
		void destroy_keeprs();

		// Method for block ID validation

		/** Validate a JazzBlockIdentifier
			\param p_id A JazzBlockIdentifier
			\return		True if p_id matches the regex JAZZ_REGEX_VALIDATE_BLOCK_ID
		*/
		inline bool valid_block_identifier(const char *p_id) {
			return std::regex_match(p_id, block_id_rex);
		}
		// Methods for JazzBlock allocation

		pJazzBlockKeeprItem new_jazz_block (const JazzBlockId64	id64,
											pJazzBlock			p_as_block,
											pJazzFilter			p_row_filter  = nullptr,
											AllAttributes	   *att			  = nullptr);

		pJazzBlockKeeprItem new_jazz_block (const JazzBlockId64	id64,
											int					cell_type,
											JazzTensorDim	   *dim,
											AllAttributes	   *att,
											int					fill_tensor		= JAZZ_FILL_NEW_WITH_NA,
											bool			   *p_bool_filter	= nullptr,
											int					stringbuff_size	= 0,
											const char		   *p_text			= nullptr,
											char				eoln			= '\n');

		pJazzBlockKeeprItem new_jazz_block (const JazzBlockIdentifier *p_id,
											pJazzBlock			  	   p_as_block,
											pJazzFilter			  	   p_row_filter	= nullptr,
											AllAttributes			  *att			= nullptr);

		pJazzBlockKeeprItem new_jazz_block (const JazzBlockIdentifier *p_id,
											int						   cell_type,
											JazzTensorDim			  *dim,
											AllAttributes			  *att,
											int						   fill_tensor	   = JAZZ_FILL_NEW_WITH_NA,
											bool					  *p_bool_filter   = nullptr,
											int						   stringbuff_size = 0,
											const char				  *p_text		   = nullptr,
											char					   eoln			   = '\n');

		pJazzBlockKeeprItem new_keepr_item ();

		void free_jazz_block(pJazzBlockKeeprItem p_item);

		/// A virtual method returning the size of the JazzBlockKeeprItem descendant that JazzBlockKeepr needs for allocation
		virtual int item_size() { return sizeof(JazzBlockKeeprItem); }

		/** Enter a thread exclusion code area for reading
		This function must be called by all the readers that enter a thread exclusion area. In this context, a reader means a thread that
		is incompatible with threads modifying data (writers) but any number of readers can enter the code area at the same time.

		It is mandatory that all paths exiting the area call leave_reading() as soon as the exclusion no longer applies.

		<b>This method never returns on failure!!</b> The most probable failure is a writer not releasing the lock.

			\param _lock_ The lock controlling the exclusion area. (Must be initialized as 0 before using.)
		*/
		inline void enter_reading(JazzLock &_lock_) {
			int retry = 0;
			while (true) {
				int32_t lock = _lock_;
				if (lock >= 0) {
					int32_t next = lock + 1;
					if (_lock_.compare_exchange_weak(lock, next))
						return;
				}
				if (++retry > JAZZ_LOCK_READING_RETRY_NUMTIMES) {
					std::this_thread::yield();
					retry = 0;
				}
			}
		}

		/** Leave a thread exclusion code area for reading
		This function must be called exactly once by all the readers that previously called enter_reading() to enter a thread exclusion area.
		In this context, a reader means a thread that is incompatible with threads modifying data (writers) but any number of readers can
		enter the code area at the same time.

			\param _lock_ The lock controlling the exclusion area. (Must be initialized as 0 before using.)
		*/
		inline void leave_reading(JazzLock &_lock_) {
			int retry = 0;
			while (true) {
				int32_t lock = _lock_;
				int32_t next = lock - 1;
				if (_lock_.compare_exchange_weak(lock, next))
					return;

				if (++retry > JAZZ_LOCK_READING_RETRY_NUMTIMES) {
					std::this_thread::yield();
					retry = 0;
				}
			}
		}

		/** Enter a thread exclusion code area for writing
		This function must be called by all the writers that enter a thread exclusion area. In this context, a writer means a thread that
		is incompatible with any other threads (readers or other writers).

		It is mandatory that all paths exiting the area call leave_writing() as soon as the exclusion no longer applies.

		<b>This method never returns on failure!!</b> The most probable failure is any other thread not releasing the lock.

			\param _lock_ The lock controlling the exclusion area. (Must be initialized as 0 before using.)
		*/
		inline void enter_writing(JazzLock &_lock_) {
			int retry = 0;
			while (true) {
				int32_t lock = _lock_;
				if (lock >= 0) {
					int32_t next = lock - JAZZ_LOCK_WEIGHT_OF_WRITE;
					if (_lock_.compare_exchange_weak(lock, next)) {
						while (true) {
							if (_lock_ == -JAZZ_LOCK_WEIGHT_OF_WRITE)
								return;
							if (++retry > JAZZ_LOCK_KICKING_RETRY_NUMTIMES) {
								std::this_thread::yield();
								retry = 0;
							}
						}
					}
				}
				if (++retry > JAZZ_LOCK_WRITING_RETRY_NUMTIMES) {
					std::this_thread::yield();
					retry = 0;
				}
			}
		}

		/** Leave a thread exclusion code area for writing
		This function must be called exactly once by all the writers that previously called enter_writing() to enter a thread exclusion area.
		In this context, a writer means a thread that is incompatible with any other threads (readers or other writers).

			\param _lock_ The lock controlling the exclusion area. (Must be initialized as 0 before using.)
		*/
		inline void leave_writing(JazzLock &_lock_) {
			int retry = 0;
			while (true) {
				int32_t lock = _lock_;
				int32_t next = lock + JAZZ_LOCK_WEIGHT_OF_WRITE;
				if (_lock_.compare_exchange_weak(lock, next))
					return;

				if (++retry > JAZZ_LOCK_WRITING_RETRY_NUMTIMES) {
					std::this_thread::yield();
					retry = 0;
				}
			}
		}

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

		std::basic_regex<char>	block_id_rex {JAZZ_REGEX_VALIDATE_BLOCK_ID};
		int						keepr_item_size, num_allocd_items;
		pJazzQueueItem			p_buffer_base, p_first_item, p_first_free;
		JazzLock				_buffer_lock_;
		jazz_utils::pJazzLogger	p_log;
};


/** Root class for all JazzBlock containers implementing trees.

Trees are the base for many descendants, including searches (MCTS, minimax with alpha-beta pruning, loss minimization, etc.) and file
systems (archives and archive descendants) both local and distributed. This class simply implements a JazzTreeItem with tree navigation
(p_parent, p_first_child, p_next_sibling) and basic stats that could be used in MCTS and other trees (num_visits, num_wins).

To create descendants, just create a container that inherits JazzTreeItem, declare item_size() with the appropriate size and implement
the methods. The inherited ::new_jazz_block() and alloc_keeprs()/realloc_keeprs()/destroy_keeprs() will work as expected.

When an JazzTree fills, blocks fail to allocate but never get automatically destroyed.

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class JazzTree: public JazzBlockKeepr {

	public:

		/// A virtual method returning the size of JazzTreeItem that JazzBlockKeepr needs for allocation
		virtual int item_size() { return sizeof(JazzTreeItem); }

		pJazzTreeItem p_tree_root = nullptr;
};


/** A JazzBlockKeepr descendant implementing a priority queue using auto balancing AA trees. The priority of a block is set by calling
a virtual function set_item_priority() when the block is created or moved. This function computes the priority based on recency number of
times used, time to build, size, etc.

When an AATBlockQueue fills, blocks get automatically destroyed (the lowest priority blocks, obviously).

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class AATBlockQueue: public JazzBlockKeepr {

	public:
		 AATBlockQueue(jazz_utils::pJazzLogger a_logger = nullptr);
		~AATBlockQueue();

		// Methods for buffer allocation

		bool alloc_keeprs  (int num_items);
		void destroy_keeprs();

		// Methods for JazzBlock allocation

		pJazzQueueItem new_jazz_block (const JazzBlockId64	id64,
									   pJazzBlock			p_as_block,
									   pJazzFilter			p_row_filter  = nullptr,
									   AllAttributes	   *att			  = nullptr,
									   uint64_t				time_to_build = 0);

		pJazzQueueItem new_jazz_block (const JazzBlockId64	id64,
									   int					cell_type,
									   JazzTensorDim	   *dim,
									   AllAttributes	   *att,
									   int					fill_tensor		= JAZZ_FILL_NEW_WITH_NA,
									   bool				   *p_bool_filter	= nullptr,
									   int					stringbuff_size	= 0,
									   const char		   *p_text			= nullptr,
									   char					eoln			= '\n',
									   uint64_t				time_to_build	= 0);

		pJazzQueueItem new_jazz_block (const JazzBlockIdentifier *p_id,
									   pJazzBlock			  	  p_as_block,
									   pJazzFilter			  	  p_row_filter	= nullptr,
									   AllAttributes			 *att			= nullptr,
									   uint64_t					  time_to_build = 0);

		pJazzQueueItem new_jazz_block (const JazzBlockIdentifier *p_id,
									   int						  cell_type,
									   JazzTensorDim			 *dim,
									   AllAttributes			 *att,
									   int						  fill_tensor	  = JAZZ_FILL_NEW_WITH_NA,
									   bool						 *p_bool_filter	  = nullptr,
									   int						  stringbuff_size = 0,
									   const char				 *p_text		  = nullptr,
									   char						  eoln			  = '\n',
									   uint64_t					  time_to_build	  = 0);

		pJazzQueueItem new_keepr_item ();

		void free_jazz_block(pJazzQueueItem p_item);

		pJazzQueueItem highest_priority_item (bool lock_it);
		pJazzQueueItem lowest_priority_item	 (bool lock_it);

		/// A virtual method returning the size of JazzQueueItem that JazzBlockKeepr needs for allocation
		virtual int item_size() { return sizeof(JazzQueueItem); }

		virtual void set_item_priority(pJazzQueueItem p_item);

	private:

		/** Return the highest priority node in the AA subtree without modifying the tree

			\param p_item The root of the AA subtree from which we want the highest priority node.

			\return		  The highest priority node in the AA subtree.

			Note: This does not alter the tree and is thread safe with other reading threads, but incompatible with writing threads.
		*/
		inline pJazzQueueItem highest_priority(pJazzQueueItem p_item)
		{
			if (p_item != nullptr) {
				while (p_item->p_alloc_next != nullptr) p_item = (pJazzQueueItem) p_item->p_alloc_next;
			};

			return p_item;
		};

		/** Return the lowest priority node in the AA subtree without modifying the tree

			\param p_item The root of the AA subtree from which we want the lowest priority node.

			\return		  The lowest priority node in the AA subtree.

			Note: This does not alter the tree and is thread safe with other reading threads, but incompatible with writing threads.
		*/
		inline pJazzQueueItem lowest_priority(pJazzQueueItem p_item)
		{
			if (p_item != nullptr) {
				while (p_item->p_alloc_prev != nullptr) p_item = (pJazzQueueItem) p_item->p_alloc_prev;
			};

			return p_item;
		};

		/** Remove links that skip level in an AA subtree

			\param p_item A tree for which we want to remove links that skip levels.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline void decrease_level(pJazzQueueItem p_item)
		{
			if ((p_item->p_alloc_prev != nullptr) & (p_item->p_alloc_next != nullptr)) {
				int should_be = std::min(reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_prev)->level,
										 reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_next)->level + 1);

				if (should_be < p_item->level) {
					p_item->level = should_be;
					if (should_be < reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_next)->level) {
						reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_next)->level = should_be;
					}
				}
			}
		};

		/** Try to rebalance an AA subtree on its left (prev) side

			\param p_item a node representing an AA tree that needs to be rebalanced.
			\return		  Another node representing the rebalanced AA tree.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline pJazzQueueItem skew(pJazzQueueItem p_item)
		{
			// rotate p_alloc_next if p_alloc_prev child has same level
			if ((p_item->p_alloc_prev != nullptr) & (p_item->level == reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_prev)->level)) {
				pJazzQueueItem p_left = (pJazzQueueItem) p_item->p_alloc_prev;

				p_item->p_alloc_prev = p_left->p_alloc_next;
				p_left->p_alloc_next = p_item;

				return p_left;
			}

			return p_item;
		};

		/** Try to rebalance an AA subtree on its right (next) side

			\param p_item A node representing an AA tree that needs to be rebalanced.
			\return		  Another node representing the rebalanced AA tree.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline pJazzQueueItem split(pJazzQueueItem p_item)
		{
			// rotate p_alloc_prev if there are two p_alloc_next children on same level
			if (  (p_item->p_alloc_next != nullptr)
				& (p_item->p_alloc_next->p_alloc_next != nullptr)
				& (p_item->level == reinterpret_cast<pJazzQueueItem>(p_item->p_alloc_next->p_alloc_next)->level)) {

				pJazzQueueItem p_right = (pJazzQueueItem) p_item->p_alloc_next;

				p_item->p_alloc_next  = p_right->p_alloc_prev;
				p_right->p_alloc_prev = p_item;
				p_right->level++;

				return p_right;
			}

			return p_item;
		};

		double discrete_recency = 0;

		pJazzQueueItem p_queue_root = nullptr;
};


/** This class is an AATBlockQueue with a JazzBlockMap cache that allows searching blocks by JazzBlockIdentifier and JazzBlockId64.
Blocks found can also be re-prioritized to make their automatic destruction less probable when they are used frequently.

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class JazzCache: public AATBlockQueue {

	public:

		/** Find a JazzBlock in a JazzCache by JazzBlockIdentifier (block name)

			\param p_id The JazzBlockIdentifier of the block to be found

			\return A pointer to the JazzBlockKeeprItem containing the block or nullptr if not found.
		*/
		inline pJazzQueueItem find_jazz_block (const JazzBlockIdentifier *p_id)
		{
			JazzBlockId64 id64 = hash_block_id((char *) p_id);

			if (!id64)
				return nullptr;

			return cache[id64];
		}

		/** Find a JazzBlock in a JazzCache by JazzBlockId64 (block name hash)

			\param id64 The JazzBlockId64 of the block to be found

			\return A pointer to the JazzBlockKeeprItem containing the block or nullptr if not found.
		*/
		inline pJazzQueueItem find_jazz_block (JazzBlockId64 id64) { return cache[id64]; }

		void		   free_jazz_block (const JazzBlockIdentifier *p_id);
		void		   free_jazz_block (		JazzBlockId64		 id64);

	private:

		JazzBlockMap cache;
};

} // namespace jazz_containers

#endif

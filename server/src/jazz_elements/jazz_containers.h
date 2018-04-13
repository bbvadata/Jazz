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

#define JAZZ_MAX_BLOCK_ID_LENGTH	   									24		///< Maximum length for a block name
#define JAZZ_REGEX_VALIDATE_BLOCK_ID	"^(/|\\.)[[:alnum:]_]{1,22}\\x00$"		///< Regex validating a JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_LOCAL	   									'.'		///< First char of a LOCAL JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_DISTRIB   									'/'		///< First char of a DISTRIBUTED JazzBlockIdentifier

/// Values for argument fill_tensor of new_jazz_block()
#define JAZZ_FILL_

#define JAZZ_FILL_NEW_DONT_FILL		0	///< Don't initialize at all.
#define JAZZ_FILL_NEW_WITH_ZERO		1	///< Initialize with binary zero.
#define JAZZ_FILL_NEW_WITH_NA		2	///< Initialize with the appropriate NA for the cell_type.
#define JAZZ_FILL_BOOLEAN_FILTER	3	///< Create a boolean (CELL_TYPE_BYTE_BOOLEAN) filter with the values in p_bool_filter.
#define JAZZ_FILL_INTEGER_FILTER	4	///< Create an integer (CELL_TYPE_INTEGER) filter with the values in p_bool_filter.

/// Values for argument set_has_NA of close_jazz_block()
#define JAZZ_SET_HAS_NA_

#define JAZZ_SET_HAS_NA_FALSE		0	///< Set to false without checking (unsafe in case there are and not-NA-aware arithmetic is used)
#define JAZZ_SET_HAS_NA_TRUE		1	///< Set to true without checking (safe even if there are, NA-aware arithemtic is always safe)
#define JAZZ_SET_HAS_NA_AUTO		2	///< Check if there are and set accordinly (slowest option when closing, best later)

/// Values for the keys of the attributes in JazzBlock

#define BLOCK_ATTR_SECTION_SIZE									   10000	///< Maximum number of keys per section
#define BLOCK_ATTR_BASEOF_CONTAINERS							       0	///< Base of attribute keys in this module
#define BLOCK_ATTR_CONTAINERS_EMPTY		BLOCK_ATTR_BASEOF_CONTAINERS + 1	///< No attributes defined, creates a <this:empty string>
#define BLOCK_ATTR_CONTAINERS_FILTER	BLOCK_ATTR_BASEOF_CONTAINERS + 2	///< A new filter: creates a <this:empty string>

#define BLOCK_ATTR_BASEOF_PERSISTENCE	BLOCK_ATTR_BASEOF_CONTAINERS  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_persistence.h
#define BLOCK_ATTR_BASEOF_CLASSES		BLOCK_ATTR_BASEOF_PERSISTENCE + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_classes.h
#define BLOCK_ATTR_BASEOF_PRIMITIVES	BLOCK_ATTR_BASEOF_CLASSES 	  + BLOCK_ATTR_SECTION_SIZE	///< Base of attribute keys in jazz_primitives.h
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
typedef struct JazzTreeItem 	  *pJazzTreeItem;			///< A pointer to a JazzTreeItem
typedef struct JazzQueueItem 	  *pJazzQueueItem;			///< A pointer to a JazzQueueItem


/** All volatile JazzBlock objects are tracked in a double linked list of JazzBlockKeeprItem descendants.
The JazzBlockKeeprItem structure is the minimum to allocate the objects in the list.
*/
struct JazzBlockKeeprItem {
	pJazzBlock			p_jazz_block;						///< A pointer to the JazzBlock
	int		   			size;								///< The size of the JazzBlockKeeprItem descendent
	int		   			keepr_state;						///< A state defining the priority between p_jazz_block, block_id64 and block_id
	pJazzBlockKeeprItem	p_alloc_prev, p_alloc_next;			///< A pair of pointers to keep this (the descendant) in a double linked list
	JazzBlockId64		block_id64;							///< Hash of block_id (or zero, if not set)
	JazzBlockIdentifier	block_id;							///< The block ID ((!block_id[0]), if not set)
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
	float	  priority;										///< A priority value to implement a priority queue
	float	  time_to_build;								///< The time required to compute the block (real or estimated)
	TimePoint last_used;									///< Timestamp of last use
};


/** A map to search a JazzBlockKeeprItem by the block_id64 of its corresponding JazzBlock. This is the essential element
that converts a queue into a cache. A cache is a queue where elements can be searched by id and lower priority items
can be removed to allocate space. priority() is a function, typically over (now - last_used, time_to_build, p_jazz_block->total_bytes).
*/
typedef std::map<JazzBlockId64, const JazzBlockKeeprItem *> JazzBlockMap;


/** A map to track usage of pointers assigned to JazzBlock objects while they are one_shot or volatile.
(For debugging purposes only.)
*/
typedef std::map<void *, int> JazzOneShotAlloc;


pJazzBlock new_jazz_block (pJazzBlock 	  p_as_block,
						   pJazzBlock 	  p_row_filter,
						   AllAttributes *att			= nullptr);


pJazzBlock new_jazz_block (int			  cell_type,
						   JazzTensorDim *dim,
						   AllAttributes *att,
						   int			  fill_tensor	  = JAZZ_FILL_NEW_WITH_NA,
						   bool			 *p_bool_filter	  = nullptr,
						   int			  stringbuff_size = 0,
						   const char	 *p_text		  = nullptr,
						   char			  separator		  = '\n');


/** Set has_NA, the creation time and the hash64 of a JazzBlock based on the content of the tensor

	Despite its name, this function does not actually "close" anything. JazzBlock manipulation is based on "good will",
after calling close_jazz_block() the owner should not change the content. If you do, you should close_jazz_block() again after.

	bool ;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
	TimePoint created;			///< Timestamp when the block was created
	uint64_t hash64;			///< Hash of everything but the header


	close_jazz_block() can be called any number of times on the same block.

	\param p_block The block to be "closed".
*/
inline void close_jazz_block(pJazzBlock p_block, int set_has_NA = JAZZ_SET_HAS_NA_AUTO) {
	switch (set_has_NA) {
	case JAZZ_SET_HAS_NA_FALSE:
		p_block->has_NA = false;
		break;
	case JAZZ_SET_HAS_NA_TRUE:
		p_block->has_NA = true;
		break;
	default:
		p_block->has_NA = p_block->find_NAs_in_tensor();
	}
	p_block->hash64  = jazz_utils::MurmurHash64A(&p_block->tensor, p_block->total_bytes - sizeof(JazzBlockHeader));
	p_block->created = std::chrono::steady_clock::now();
}

void free_jazz_block(pJazzBlock &p_block);


class JazzBlockKeepr {

	inline void hash_block_id();

};


class JazzTree: public JazzBlockKeepr {

};


class AATBlockQueue: public JazzBlockKeepr {

};


class JazzCache: public AATBlockQueue {

};

/*
class AATBlockQueue
{
public:

			AATBlockQueue();
		   ~AATBlockQueue();

	bool	AllocModels			   (int numModels);

	pJazzQueueItem GetFreeModel			   ();
	void	PushModelToPriorityQueue(pJazzQueueItem pM);
	pJazzQueueItem GetHighestPriorityModel ();

	pJazzQueueItem pQueueRoot;

private:

	pJazzQueueItem pBuffBase, pFirstFree;
	int		numAllocM;
};

*/

} // namespace jazz_containers

#endif

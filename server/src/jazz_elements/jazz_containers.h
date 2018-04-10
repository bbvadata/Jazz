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


/**
*/
pJazzBlock new_jazz_block (pJazzBlock p_as_block,
						   pJazzBlock p_row_filter);


/**
*/
pJazzBlock new_jazz_block (int			  cell_type,
						   JazzTensorDim *dim,
						   AllAttributes *att,
						   int			  stringbuff_size);


/**
*/
pJazzBlock new_jazz_block (int			  cell_type,
						   JazzTensorDim *dim,
						   AllAttributes *att,
						   const char	 *p_text,
						   char			  separator = '\n');


/** Set the creation time and the hash64 of a JazzBlock

	Despite its name, this function does not actually "close" anything. JazzBlock manipulation is based on "good will",
after calling close_jazz_block() the owner should not change the content or should close_jazz_block() again after doing so.

	close_jazz_block() can be called any number of times on the same block.

	\param p_block The block to be "closed".
*/
inline void close_jazz_block(pJazzBlock p_block) {
	p_block->hash64  = jazz_utils::MurmurHash64A(&p_block->tensor[0], p_block->total_bytes - sizeof(JazzBlockHeader));
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

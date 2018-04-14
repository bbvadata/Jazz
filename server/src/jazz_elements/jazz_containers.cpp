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


#include "src/jazz_elements/jazz_containers.h"


namespace jazz_containers
{

/** Create a new (one_shot) JazzBlock as a selection (of possibly all) of an existing JazzBlock

	\param p_as_block   An existing block from which everything is copied, possibly with a selection over its rows.
	\param p_row_filter A filter that is applicable to p_as_block. I.e., p_row_filter->can_filter(p_as_block) == true
						If p_row_filter == nullptr then p_as_block is copied into a newly allocated pointer.
						See parameter dim in the new_jazz_block() version that uses dim to understand how selection is applied.
	\param att 			An alternative source of attributes. When this parameter in != nullptr, the new block will get its
						attributes from att instead of copying those in p_as_block->.

	OWNERSHIP: If you create a one shot block using new_jazz_block(), you earn the responsibility to free it with free_jazz_block().
	This is not the normal way to create JazzBlocks, when you use a JazzBlockKeepr descendant, that object will allocate and free
	the JazzBlocks automatically. The same applies to JazzBlocks created in the stack of a bebop program which are also managed
	automatically.

	\return	The new JazzBlock or nullptr if failed. Also logs errors to jazz_containers::log
*/
pJazzBlock new_jazz_block (pJazzBlock  	  p_as_block,
						   pJazzFilter 	  p_row_filter,
						   AllAttributes *att)
{

}


/** Create a new (one_shot) JazzBlock (including a JazzFilter) from scratch

	\param cell_type		The type for the tensor's cell types in [CELL_TYPE_BYTE..CELL_TYPE_DOUBLE]
	\param dim 				This defines both the rank and the dimensions of the tensor. Note that, except for the first position a
							dimension of 0 and 1 is the same dim = {3, 1} is a vector of 3 elements with rank 1, exactly like {3, 0}.
							As a matter of convention, dim should always end with a 0 except when it is JAZZ_MAX_TENSOR_RANK long.
							For the first dimension 1 means one element and 0 means no element. Both have rank 1. The latter is the
							typical result of a selection where no row matches the condition. Blocks never have rank == 0 and zero-element
							blocks have the same rank as the block from which they were selected. When 0 rows are selected from a block
							of dim = {r, s, t} the resulting block is {0, s, t} with size == 0 and rank == 3.
	\param att				The attributes to set when creating the block. They are be inmutable. To change the attributes of a JazzBlock
							use the version of new_jazz_block() with parameter p_as_block
	\param fill_tensor 		How to fill the tensor. When creating anything that is not a JazzFilter, p_bool_filter is ignored and the options
							are: JAZZ_FILL_NEW_DONT_FILL (don't do anything with the tensor), JAZZ_FILL_NEW_WITH_ZERO (fill with binary zero
							no matter what the cell_type is), JAZZ_FILL_NEW_WITH_NA fill with the appropriate NA for the cell_type)
							When creating a filter, p_bool_filter must be a vector of length == size and the filter will be created as
							boolean (when fill_tensor == JAZZ_FILL_BOOLEAN_FILTER) or integer (when fill_tensor == JAZZ_FILL_INTEGER_FILTER)
	\param p_bool_filter	The vector of boolean (each true value means the corresponding row is selected) used when fill_tensor ==
							JAZZ_FILL_BOOLEAN_FILTER and fill_tensor == JAZZ_FILL_INTEGER_FILTER
	\param stringbuff_size 	One of the possible ways to allocate space for strings is declaring this size. When this is non-zero a buffer
							will be allocated with this size plus whatever size is required by the strings in att. new_jazz_block() will
							only allocate the space and do nothing with it. The caller should assign strings to cells with JazzBlock.set_string().
	\param p_text			The other possible way to allocate space for strings is by declaring p_text. Imagine the content of p_text
							as a text file with n = size rows that will be pushed into the tensor and the string buffer. The eoln character
							separates the cells.
	\param eoln		       	A single character that separates the cells in p_text and will not be pushed to the string buffer.

	NOTES: String buffer allocation should not be used to dynamically change attribute values. Attributes are inmutable and should be changed
	only creating a new block with new = new_jazz_block(p_as_block = old, att = new_att).
	String buffer allocation should only be used for cell_type == CELL_TYPE_JAZZ_STRING and either with stringbuff_size or with p_text (and eoln).
	If stringbuff_size is used, JazzBlock.set_string() should be used afterwards. If p_text is used, the tensor is already filled and
	JazzBlock.set_string() **should not** be called after that.

	OWNERSHIP: If you create a one shot block using new_jazz_block(), you earn the responsibility to free it with free_jazz_block().
	This is not the normal way to create JazzBlocks, when you use a JazzBlockKeepr descendant, that object will allocate and free
	the JazzBlocks automatically. The same applies to JazzBlocks created in the stack of a bebop program which are also managed
	automatically.

	\return	The new JazzBlock or nullptr if failed. Also logs errors to jazz_containers::log
*/
pJazzBlock new_jazz_block (int			  cell_type,
						   JazzTensorDim *dim,
						   AllAttributes *att,
						   int			  fill_tensor,
						   bool			 *p_bool_filter,
						   int			  stringbuff_size,
						   const char	 *p_text,
						   char			  eoln)
{

}


/** Free a (one_shot) JazzBlock created with new_jazz_block()

	\param p_block The pointer to the block you previously created with new_jazz_block().

	OWNERSHIP: Never free a pointer you have not created with new_jazz_block(). The normal way to create JazzBlocks is using a JazzBlockKeepr
	descendant, that object will allocate and free	the JazzBlocks automatically. Also in the stack of a bebop program JazzBlocks are also
	managed automatically.
*/
void free_jazz_block(pJazzBlock &p_block)
{

}


JazzBlockKeepr::JazzBlockKeepr()
{
	keepr_item_size = item_size();

}

JazzBlockKeepr::~JazzBlockKeepr()
{

}

/* ----------------------------------------------------------------------------

	GEM : MCTS vs GrQ experiments for my PhD thesis : AATree based priority queue

	Author : Jacques Basaldúa (jacques@dybot.com)

	Version       : 2.0
	Date          : 2012/03/27	(Using Pre 2012 GEM and Pre 2012 ModelSel)
	Last modified :

	(c) Jacques Basaldúa, 2009-2012

-------------------------------------------------------------------------------

Revision history :

	Version		  : 1.0
	Date		  : 2010/01/21


#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include <xstring>
#include <math.h>

using namespace std;

#include "AATBlockQueue.h"

/*	-------------------------------------------------------------------------------------------------

		B a s e   p r o c e d u r a l   A A - t r e e   u t i l s



// pJazzQueueItem skew(pJazzQueueItem pN):	(As in http://en.wikipedia.org/wiki/AA_tree)
// ----------------------

// input: T, a node representing an AA tree that needs to be rebalanced.
// output: Another node representing the rebalanced AA tree.

inline pJazzQueueItem skew(pJazzQueueItem pT)
{
	// rotate p_alloc_next if p_alloc_prev child has same level
	if (pT->p_alloc_prev && pT->level == pT->p_alloc_prev->level)
	{
		pJazzQueueItem pL = pT->p_alloc_prev;
		pT->p_alloc_prev = pL->p_alloc_next;
		pL->p_alloc_next = pT;

		return pL;
	}

	return pT;
};


// pJazzQueueItem split(pJazzQueueItem pN):	(As in http://en.wikipedia.org/wiki/AA_tree)
// -----------------------

// input: N, a node representing an AA tree that needs to be rebalanced.
// output: Another node representing the rebalanced AA tree.

inline pJazzQueueItem split(pJazzQueueItem pT)
{
	// rotate p_alloc_prev if there are two p_alloc_next children on same level
	if (pT->p_alloc_next && pT->p_alloc_next->p_alloc_next && pT->level == pT->p_alloc_next->p_alloc_next->level)
	{
		pJazzQueueItem pR = pT->p_alloc_next;
		pT->p_alloc_next = pR->p_alloc_prev;
		pR->p_alloc_prev = pT;
		pR->level++;

		return pR;
	}

	return pT;
};


// pJazzQueueItem insert(pJazzQueueItem pN, pJazzQueueItem pT):
// -----------------------------------

// input: N, the value to be inserted, and T, the root of the tree to insert it into.
// output: A balanced version T including X.

pJazzQueueItem insert(pJazzQueueItem pN, pJazzQueueItem pT)
{

// Do the normal binary tree insertion procedure.  Set the result of the
// recursive call to the correct child in case a new node was created or the
// root of the subtree changes.

    if (pT == NULL)
	{
		pN->level = 1;
		pN->p_alloc_prev = NULL;
		pN->p_alloc_next = NULL;

		return pN;
	}
    else
	{
		if (pN->priority < pT->priority) pT->p_alloc_prev = insert(pN, pT->p_alloc_prev);
		else							 pT->p_alloc_next = insert(pN, pT->p_alloc_next);
	}

// Perform skew and then split.  The conditionals that determine whether or
// not a rotation will occur or not are inside of the procedures, as given above.

	pT = skew(pT);
	pT = split(pT);

	return pT;
};


// pJazzQueueItem decrease_level(pJazzQueueItem pT):
// --------------------------------

// input: T, a tree for which we want to remove links that skip levels.
// output: T with its level decreased.

inline void decrease_level(pJazzQueueItem pT)
{
	if (pT->p_alloc_prev && pT->p_alloc_next)
	{
		int should_be = min(pT->p_alloc_prev->level, pT->p_alloc_next->level + 1);

		if (should_be < pT->level)
		{
			pT->level = should_be;
			if (should_be < pT->p_alloc_next->level) pT->p_alloc_next->level = should_be;
		}
	}
};


// pJazzQueueItem remove(pJazzQueueItem pN, pJazzQueueItem pT):
// -----------------------------------

// input: N, the node to remove, and T, the root of the tree from which it should be deleted.
// output: T, balanced, without the node N.

pJazzQueueItem remove_hi(pJazzQueueItem pN, pJazzQueueItem pT)
{
	if (pN == pT)
	{
		if (!pT->p_alloc_prev)
		{
			pT = pT->p_alloc_next;
			if (!pT) return NULL;
		}
		else pT = pT->p_alloc_prev;
	}
	else
	{
		if (pN->priority < pT->priority) pT->p_alloc_prev = remove_hi(pN, pT->p_alloc_prev);
		else							 pT->p_alloc_next = remove_hi(pN, pT->p_alloc_next);
	};

// Rebalance the tree.  Decrease the level of all nodes in this level if
// necessary, and then skew and split all nodes in the new level.

    decrease_level(pT);
    pT = skew(pT);
	if (pT->p_alloc_next)
	{
		pT->p_alloc_next = skew(pT->p_alloc_next);
		if (pT->p_alloc_next->p_alloc_next) pT->p_alloc_next->p_alloc_next = skew(pT->p_alloc_next->p_alloc_next);
	};
	pT = split(pT);
	if (pT->p_alloc_next) pT->p_alloc_next = split(pT->p_alloc_next);

    return pT;
};


pJazzQueueItem remove_lo(pJazzQueueItem pN, pJazzQueueItem pT)
{
	if (pN == pT)
	{
		if (!pT->p_alloc_prev)
		{
			pT = pT->p_alloc_next;
			if (!pT) return NULL;
		}
		else pT = pT->p_alloc_prev;
	}
	else
	{
		if (pN->priority <= pT->priority) pT->p_alloc_prev = remove_lo(pN, pT->p_alloc_prev);
		else							  pT->p_alloc_next = remove_lo(pN, pT->p_alloc_next);
	};

// Rebalance the tree.  Decrease the level of all nodes in this level if
// necessary, and then skew and split all nodes in the new level.

    decrease_level(pT);
    pT = skew(pT);
	if (pT->p_alloc_next)
	{
		pT->p_alloc_next = skew(pT->p_alloc_next);
		if (pT->p_alloc_next->p_alloc_next) pT->p_alloc_next->p_alloc_next = skew(pT->p_alloc_next->p_alloc_next);
	};
	pT = split(pT);
	if (pT->p_alloc_next) pT->p_alloc_next = split(pT->p_alloc_next);

    return pT;
};


// pJazzQueueItem HighestPriority(pJazzQueueItem pT):
// -----------------------------------

// input: T, the root of the tree from which we want the highest priority node.
// output: The node N (Tree not modified.)

inline pJazzQueueItem HighestPriority(pJazzQueueItem pT)
{
	if (pT)
	{
		while (pT->p_alloc_next) pT = pT->p_alloc_next;
	};

	return pT;
};

// pJazzQueueItem LowestPriority(pJazzQueueItem pT):
// -----------------------------------

// input: T, the root of the tree from which we want the lowest priority node.
// output: The node N (Tree not modified.)

inline pJazzQueueItem LowestPriority(pJazzQueueItem pT)
{
	if (pT)
	{
		while (pT->p_alloc_prev) pT = pT->p_alloc_prev;
	};

	return pT;
};


ModelBuffer::ModelBuffer()
{
	pBuffBase  = NULL;
	pQueueRoot = NULL;
	pFirstFree = NULL;
	numAllocM  = 0;
};


ModelBuffer::~ModelBuffer()
{
	if (pBuffBase) delete [] pBuffBase;
};


bool ModelBuffer::AllocModels(int numModels)
{
	if (pBuffBase) delete [] pBuffBase;

	pBuffBase  = NULL;
	pQueueRoot = NULL;
	pFirstFree = NULL;
	numAllocM  = 0;

	if (!numModels) return true;

	pBuffBase = new  (nothrow) Model_InGrQS [numModels];

	if (!pBuffBase) return false;

	numAllocM = numModels;

	memset(pBuffBase, 0, sizeof(Model_InGrQS)*numAllocM);

	pQueueRoot = NULL;
	pFirstFree = pBuffBase;

	int i;

	for (i = 0; i < numAllocM - 1; i++) pBuffBase[i].p_alloc_next = &pBuffBase[i + 1];

	return true;
};


pJazzQueueItem ModelBuffer::GetFreeModel()
{
	if (pFirstFree)
	{
		pJazzQueueItem pM = pFirstFree;
		pFirstFree = pM->p_alloc_next;
		return pM;
	};

	pJazzQueueItem pLP = LowestPriority(pQueueRoot);

	pQueueRoot = remove_lo(pLP, pQueueRoot);

	return pLP;
};


void ModelBuffer::PushModelToPriorityQueue(pJazzQueueItem pM)
{
	pQueueRoot = insert(pM, pQueueRoot);
};


pJazzQueueItem ModelBuffer::GetHighestPriorityModel()
{
	pJazzQueueItem pHP = HighestPriority(pQueueRoot);

	if (pHP)
	{
		pQueueRoot = remove_hi(pHP, pQueueRoot);

		pHP->p_alloc_prev = NULL;
		pHP->p_alloc_next = pFirstFree;
		pFirstFree = pHP;
	};

	return pHP;
};

*/

} // namespace jazz_containers


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_containers.ctest"
#endif

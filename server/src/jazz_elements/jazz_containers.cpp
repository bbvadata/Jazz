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

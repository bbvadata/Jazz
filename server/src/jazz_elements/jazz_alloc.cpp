/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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


#include "src/jazz_elements/jazz_alloc.h"


namespace jazz_alloc
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



// pgModel skew(pgModel pN):	(As in http://en.wikipedia.org/wiki/AA_tree)
// ----------------------

// input: T, a node representing an AA tree that needs to be rebalanced.
// output: Another node representing the rebalanced AA tree.

inline pgModel skew(pgModel pT)
{
	// rotate pAllocNext if pAllocPrev child has same level
	if (pT->pAllocPrev && pT->level == pT->pAllocPrev->level)
	{
		pgModel pL = pT->pAllocPrev;
		pT->pAllocPrev = pL->pAllocNext;
		pL->pAllocNext = pT;

		return pL;
	}

	return pT;
};


// pgModel split(pgModel pN):	(As in http://en.wikipedia.org/wiki/AA_tree)
// -----------------------

// input: N, a node representing an AA tree that needs to be rebalanced.
// output: Another node representing the rebalanced AA tree.

inline pgModel split(pgModel pT)
{
	// rotate pAllocPrev if there are two pAllocNext children on same level
	if (pT->pAllocNext && pT->pAllocNext->pAllocNext && pT->level == pT->pAllocNext->pAllocNext->level)
	{
		pgModel pR = pT->pAllocNext;
		pT->pAllocNext = pR->pAllocPrev;
		pR->pAllocPrev = pT;
		pR->level++;

		return pR;
	}

	return pT;
};


// pgModel insert(pgModel pN, pgModel pT):
// -----------------------------------

// input: N, the value to be inserted, and T, the root of the tree to insert it into.
// output: A balanced version T including X.

pgModel insert(pgModel pN, pgModel pT)
{

// Do the normal binary tree insertion procedure.  Set the result of the
// recursive call to the correct child in case a new node was created or the
// root of the subtree changes.

    if (pT == NULL)
	{
		pN->level = 1;
		pN->pAllocPrev = NULL;
		pN->pAllocNext = NULL;

		return pN;
	}
    else
	{
		if (pN->priority < pT->priority) pT->pAllocPrev = insert(pN, pT->pAllocPrev);
		else							 pT->pAllocNext = insert(pN, pT->pAllocNext);
	}

// Perform skew and then split.  The conditionals that determine whether or
// not a rotation will occur or not are inside of the procedures, as given above.

	pT = skew(pT);
	pT = split(pT);

	return pT;
};


// pgModel decrease_level(pgModel pT):
// --------------------------------

// input: T, a tree for which we want to remove links that skip levels.
// output: T with its level decreased.

inline void decrease_level(pgModel pT)
{
	if (pT->pAllocPrev && pT->pAllocNext)
	{
		int should_be = min(pT->pAllocPrev->level, pT->pAllocNext->level + 1);

		if (should_be < pT->level)
		{
			pT->level = should_be;
			if (should_be < pT->pAllocNext->level) pT->pAllocNext->level = should_be;
		}
	}
};


// pgModel remove(pgModel pN, pgModel pT):
// -----------------------------------

// input: N, the node to remove, and T, the root of the tree from which it should be deleted.
// output: T, balanced, without the node N.

pgModel remove_hi(pgModel pN, pgModel pT)
{
	if (pN == pT)
	{
		if (!pT->pAllocPrev)
		{
			pT = pT->pAllocNext;
			if (!pT) return NULL;
		}
		else pT = pT->pAllocPrev;
	}
	else
	{
		if (pN->priority < pT->priority) pT->pAllocPrev = remove_hi(pN, pT->pAllocPrev);
		else							 pT->pAllocNext = remove_hi(pN, pT->pAllocNext);
	};

// Rebalance the tree.  Decrease the level of all nodes in this level if
// necessary, and then skew and split all nodes in the new level.

    decrease_level(pT);
    pT = skew(pT);
	if (pT->pAllocNext)
	{
		pT->pAllocNext = skew(pT->pAllocNext);
		if (pT->pAllocNext->pAllocNext) pT->pAllocNext->pAllocNext = skew(pT->pAllocNext->pAllocNext);
	};
	pT = split(pT);
	if (pT->pAllocNext) pT->pAllocNext = split(pT->pAllocNext);

    return pT;
};


pgModel remove_lo(pgModel pN, pgModel pT)
{
	if (pN == pT)
	{
		if (!pT->pAllocPrev)
		{
			pT = pT->pAllocNext;
			if (!pT) return NULL;
		}
		else pT = pT->pAllocPrev;
	}
	else
	{
		if (pN->priority <= pT->priority) pT->pAllocPrev = remove_lo(pN, pT->pAllocPrev);
		else							  pT->pAllocNext = remove_lo(pN, pT->pAllocNext);
	};

// Rebalance the tree.  Decrease the level of all nodes in this level if
// necessary, and then skew and split all nodes in the new level.

    decrease_level(pT);
    pT = skew(pT);
	if (pT->pAllocNext)
	{
		pT->pAllocNext = skew(pT->pAllocNext);
		if (pT->pAllocNext->pAllocNext) pT->pAllocNext->pAllocNext = skew(pT->pAllocNext->pAllocNext);
	};
	pT = split(pT);
	if (pT->pAllocNext) pT->pAllocNext = split(pT->pAllocNext);

    return pT;
};


// pgModel HighestPriority(pgModel pT):
// -----------------------------------

// input: T, the root of the tree from which we want the highest priority node.
// output: The node N (Tree not modified.)

inline pgModel HighestPriority(pgModel pT)
{
	if (pT)
	{
		while (pT->pAllocNext) pT = pT->pAllocNext;
	};

	return pT;
};

// pgModel LowestPriority(pgModel pT):
// -----------------------------------

// input: T, the root of the tree from which we want the lowest priority node.
// output: The node N (Tree not modified.)

inline pgModel LowestPriority(pgModel pT)
{
	if (pT)
	{
		while (pT->pAllocPrev) pT = pT->pAllocPrev;
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

	for (i = 0; i < numAllocM - 1; i++) pBuffBase[i].pAllocNext = &pBuffBase[i + 1];

	return true;
};


pgModel ModelBuffer::GetFreeModel()
{
	if (pFirstFree)
	{
		pgModel pM = pFirstFree;
		pFirstFree = pM->pAllocNext;
		return pM;
	};

	pgModel pLP = LowestPriority(pQueueRoot);

	pQueueRoot = remove_lo(pLP, pQueueRoot);

	return pLP;
};


void ModelBuffer::PushModelToPriorityQueue(pgModel pM)
{
	pQueueRoot = insert(pM, pQueueRoot);
};


pgModel ModelBuffer::GetHighestPriorityModel()
{
	pgModel pHP = HighestPriority(pQueueRoot);

	if (pHP)
	{
		pQueueRoot = remove_hi(pHP, pQueueRoot);

		pHP->pAllocPrev = NULL;
		pHP->pAllocNext = pFirstFree;
		pFirstFree = pHP;
	};

	return pHP;
};

*/

} // namespace jazz_alloc


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_alloc.ctest"
#endif

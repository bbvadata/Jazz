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


/**< \brief Container classes for JazzBlock objects.

  JazzBlock objects can be:\n

1. One-shot - Owned by the caller of new_jazz_block()\n
2. Volatile - Owned by some JazzBlockKeepr descendant that is not a JazzPersistence (or descendant)\n
3. Persisted - Owned by a JazzPersistence (or descendant), typically a JazzSource


Unlike
in Jazz 0.1.+, there is no support for embedded R (or any other interpreters).


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

#define JAZZ_MAX_BLOCK_ID_LENGTH	   									24		///< Maximum length for a block name
#define JAZZ_REGEX_VALIDATE_BLOCK_ID	"^(/|\\.)[[:alnum:]_]{1,22}\\x00$"		///< Regex validating a JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_LOCAL	   									'.'		///< First char of a LOCAL JazzBlockIdentifier
#define JAZZ_BLOCK_ID_PREFIX_DISTRIB   									'/'		///< First char of a DITRIBUTED JazzBlockIdentifier


/** A readable block identifier. It must be a string matching JAZZ_REGEX_VALIDATE_BLOCK_ID. This name is the key identifying
the JazzBlock in a JazzPersistence, JazzSource or via the API source.block (local) source/block (distributed).
*/
struct JazzBlockIdentifier {
	char key[JAZZ_MAX_BLOCK_ID_LENGTH];
};

/** A binary block identifier. It is a MurmurHash64A of the JazzBlockIdentifier computed with JazzBlockKeepr.hash_block_id
*/
typedef uint64_t JazzBlockId64;

struct JazzBlockKeeprItem {

};

struct JazzTreeItem: JazzBlockKeeprItem {

};

struct JazzQueueItem: JazzBlockKeeprItem {

};

typedef std::map<JazzBlockId64, const JazzBlockKeeprItem *> JazzBlockMap;

class JazzBlockKeepr {

	inline JazzBlockId64 hash_block_id(const JazzBlockIdentifier pBI);

};

class JazzTree: public JazzBlockKeepr {

};

class AATBlockQueue {

};

class JazzCache: public JazzBlockKeepr {

};


// {
// 	int	cell_type;				///< The type for the cells in the tensor. See CELL_TYPE_*
// 	int	rank;					///< The number of dimensions
// 	JazzTensorDim dim_offs;		///< The dimensions of the tensor in terms of offsets (Max. size is 2 Gb.)
// 	int size;					///< The total number of cells in the tensor
// 	int num_attributes;			///< Number of elements in the JazzAttributesMap
// 	int total_bytes;			///< Total size of the block everything included
// 	bool has_NA;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
// 	TimePoint created;			///< Timestamp when the block was created
// 	long long hash64;			///< Hash of everything but the header

// 	int tensor[];				///< A tensor for type cell_type and dimensions set by JazzBlock.set_dimensions()
// };


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


#pragma once

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include <xstring>
#include <math.h>

using namespace std;

#include "GEMtypes.h"


bool   TestLibFunctions	();
bool   TestLibClasses	();


class ModelBuffer
{
public:

			ModelBuffer();
		   ~ModelBuffer();

	bool	AllocModels			   (int numModels);

	pgModel GetFreeModel			   ();
	void	PushModelToPriorityQueue(pgModel pM);
	pgModel GetHighestPriorityModel ();

	pgModel pQueueRoot;

private:

	pgModel pBuffBase, pFirstFree;
	int		numAllocM;
};

*/

} // namespace jazz_containers

#endif

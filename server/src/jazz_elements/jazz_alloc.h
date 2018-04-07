/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATModelQueue class (c) Jacques Basaldúa, 2009-2012 licensed
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


/**< \brief Allocation functions for Jazz.

	This module defines functions to explicitely allocate RAM for JazzDataBlock structures. The module is
functional and understands ownership of pointers between persisted, volatile or one-shot JazzDataBlock
structures. JazzDataObject descendants are memory-wise just JazzDataBlock structures belonging to a class,
so their allocation will also be handled by this module and not via "fancy" C++ object allocation. Unlike
in Jazz 0.1.+, there is no support for embedded R (or any other interpreters).
*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_ALLOC
#define INCLUDED_JAZZ_ELEMENTS_ALLOC

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

} // namespace jazz_alloc

#endif

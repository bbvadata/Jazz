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


#include "src/jazz_elements/jazz_datablocks.h"
#include "src/jazz_elements/jazz_utils.h"
#include "src/jazz_elements/jazz_containers.h"

/**< \brief Jazz class JazzPersistedSource

	This module defines the class JazzPersistedSource to store the data in memory mapped node-local files called "sources".
The persistence is a thread safe key-value store based on LMDB.
*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_PERSISTENCE
#define INCLUDED_JAZZ_ELEMENTS_PERSISTENCE

#include "src/lmdb/lmdb.h"

namespace jazz_persistence
{

using namespace jazz_datablocks;
using namespace jazz_containers;


/**
This is the root class for storing persisted JazzBlocks. JazzBlocks are created with jazz_persistence::JazzPersistence::new_jazz_block()
or (JazzPersistence descendant::)new_jazz_block() and not required to be removed, but can be removed using
(JazzPersistence descendant::)remove_jazz_block(). Unlike volatile JazzBlocks, persisted JazzBlocks are not controlled by a JazzBlockKeeprItem.
Difference between JazzPersistence and JazzSource is the former implements a strict JazzBlockKeepr interface that can be used from c++ to do
things like select information from blocks without assigning or copying them, the latter has a much simpler interface that is exported to
Python and R and provides what a script language programmer would expect at the price of not always benefiting from the memory-mapped
file allocation in lmdb that underlies JazzPersistence.

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class JazzPersistence: public JazzBlockKeepr {

	public:

		 JazzPersistence();
		~JazzPersistence();

		// Methods for JazzBlock allocation

		pJazzBlock new_jazz_block (const JazzBlockIdentifier *p_id,
										 pJazzBlock			  p_as_block,
										 pJazzBlock			  p_row_filter	= nullptr,
										 AllAttributes		 *att			= nullptr);

		pJazzBlock new_jazz_block (const JazzBlockIdentifier *p_id,
										 int				  cell_type,
										 JazzTensorDim		 *dim,
										 AllAttributes		 *att,
										 int				  fill_tensor	  = JAZZ_FILL_NEW_WITH_NA,
										 bool				 *p_bool_filter	  = nullptr,
										 int				  stringbuff_size = 0,
										 const char			 *p_text		  = nullptr,
										 char				  eoln			  = '\n');

		void remove_jazz_block(pJazzBlock p_item);
};


/**
A much simpler interface to create/read/update/delete JazzBlocks in source using and underlying JazzPersistence that can be exported
to R, Python and the REST API directly.

THREAD SAFETY: All public methods in JazzBlockKeepr descendants must be thread safe. In the core objects, thread-safe failure in public methods
is treated as a top priority bug that is intended to be spotted in burn-in tests. Private methods can be unsafe, but the public methods calling
them must be aware of their limitations and use thread-locking when necessary. (Copy this message in all descendants.)
*/
class JazzSource: public JazzPersistence {

	public:

		 JazzSource();
		~JazzSource();
};

}

#endif

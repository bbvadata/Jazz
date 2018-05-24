/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Also includes LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

   Licensed under http://www.OpenLDAP.org/license.html

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

//TODO: Write the module description
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

#define JAZZ_PERSISTENCE_CACHE_READ_ONLY		1	///< Do not cache write operations.
#define JAZZ_PERSISTENCE_CACHE_READ_WRITE		3	///< Cache write operations.

#define JAZZ_SOURCE_MODE_READ_ONLY				1	///< Open a JazzPersistence as read only.
#define JAZZ_SOURCE_MODE_READ_WRITE				3	///< Open a JazzPersistence with read/write access.


using namespace jazz_datablocks;
using namespace jazz_containers;


typedef struct JazzPersistenceItem *pJazzPersistenceItem;		///< A pointer to a JazzPersistenceItem
typedef JazzBlockIdentifier			JazzBlockList[];			///< A pointer to an array of JazzBlockIdentifier

/** The item class for JazzPersistence descendants
*/
struct JazzPersistenceItem: JazzBlockKeeprItem {
};



/**
//TODO: Write the JazzPersistence description
*/
class JazzPersistence: public JazzBlockKeepr {

	public:

		 JazzPersistence(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzPersistence();

		// Methods for buffer allocation

		virtual void destroy_keeprs();

		// Methods for JazzBlock allocation

		pJazzPersistenceItem new_jazz_block (const JazzBlockId64 id64,
											 pJazzBlock			 p_as_block,
											 pJazzFilter		 p_row_filter  = nullptr,
											 AllAttributes		*att		   = nullptr,
											 uint64_t			 time_to_build = 0);

		pJazzPersistenceItem new_jazz_block (const JazzBlockId64 id64,
											 int				 cell_type,
											 int				*dim,
											 AllAttributes		*att			 = nullptr,
											 int				 fill_tensor	 = JAZZ_FILL_NEW_WITH_NA,
											 bool				*p_bool_filter	 = nullptr,
											 int				 stringbuff_size = 0,
											 const char			*p_text			 = nullptr,
											 char				 eoln			 = '\n',
											 uint64_t			 time_to_build	 = 0);

		pJazzPersistenceItem new_jazz_block (const JazzBlockIdentifier *p_id,
											 pJazzBlock					p_as_block,
											 pJazzFilter				p_row_filter  = nullptr,
											 AllAttributes			   *att			  = nullptr,
											 uint64_t					time_to_build = 0);

		pJazzPersistenceItem new_jazz_block (const JazzBlockIdentifier *p_id,
											 int						cell_type,
											 int					   *dim,
											 AllAttributes			   *att				= nullptr,
											 int						fill_tensor		= JAZZ_FILL_NEW_WITH_NA,
											 bool					   *p_bool_filter	= nullptr,
											 int						stringbuff_size = 0,
											 const char				   *p_text		  	= nullptr,
											 char						eoln			= '\n',
											 uint64_t					time_to_build	= 0);

		// Methods for finding JazzBlock by ID (individually)

		pJazzPersistenceItem find_jazz_block (const JazzBlockIdentifier *p_id);
		pJazzPersistenceItem find_jazz_block (JazzBlockId64 			 id64);

		// Methods for removing JazzBlock (individually)

		virtual void free_jazz_block (pJazzPersistenceItem 		 p_item);
		bool 		 free_jazz_block (const JazzBlockIdentifier *p_id);
		bool		 free_jazz_block (JazzBlockId64				 id64);

		/// A virtual method returning the size of JazzPersistenceItem that JazzBlockKeepr needs for allocation
		virtual int item_size() { return sizeof(JazzPersistenceItem); }

		/// A cache definition interface
		bool alloc_cache(int num_items, int cache_mode);

		/// A pipeline definition interface
		bool copy_to_keepr (JazzBlockKeepr keepr,
							JazzBlockList  p_id,
							int			   num_blocks);

		bool copy_from_keepr (JazzBlockKeepr keepr,
							  JazzBlockList  p_id,
							  int			 num_blocks);
};


/**
//TODO: Write the JazzSource description
*/
class JazzSource: public JazzPersistence {

	public:

		 JazzSource();
		~JazzSource();
};

}

#endif

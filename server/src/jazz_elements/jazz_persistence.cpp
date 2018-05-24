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

#include "src/jazz_elements/jazz_persistence.h"

namespace jazz_persistence
{

/**
//TODO: Document JazzSource constructor
*/
JazzSource::JazzSource(jazz_utils::pJazzLogger a_logger)
{
//TODO: Implement JazzSource constructor
}


/**
//TODO: Document ~JazzSource
*/
JazzSource::~JazzSource()
{
//TODO: Implement ~JazzSource
}


/**
//TODO: Document destroy_keeprs
*/
void JazzSource::destroy_keeprs()
{
//TODO: Implement destroy_keeprs
}


/**
//TODO: Document new_jazz_block (1)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockId64 id64,
												pJazzBlock			p_as_block,
												pJazzFilter		    p_row_filter,
												AllAttributes	   *att,
												uint64_t			time_to_build)
{
//TODO: Implement new_jazz_block (1)
}


/**
//TODO: Document new_jazz_block (2)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockId64 id64,
												int				 	cell_type,
												int				   *dim,
												AllAttributes	   *att,
												int				 	fill_tensor,
												bool			   *p_bool_filter,
												int					stringbuff_size,
												const char		   *p_text,
												char				eoln,
												uint64_t			time_to_build)
{
//TODO: Implement new_jazz_block (2)
}


/**
//TODO: Document new_jazz_block (3)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockIdentifier *p_id,
												pJazzBlock				   p_as_block,
												pJazzFilter				   p_row_filter,
												AllAttributes			  *att,
												uint64_t				   time_to_build)
{
//TODO: Implement new_jazz_block (3)
}


/**
//TODO: Document new_jazz_block (4)
*/
pJazzPersistenceItem JazzSource::new_jazz_block(const JazzBlockIdentifier *p_id,
												int						   cell_type,
												int					   	  *dim,
												AllAttributes			  *att,
												int						   fill_tensor,
												bool					  *p_bool_filter,
												int						   stringbuff_size,
												const char				  *p_text,
												char					   eoln,
												uint64_t				   time_to_build)
{
//TODO: Implement new_jazz_block (4)
}


/**
//TODO: Document find_jazz_block (1)
*/
pJazzPersistenceItem JazzSource::find_jazz_block(const JazzBlockIdentifier *p_id)
{
//TODO: Implement find_jazz_block (1)
}


/**
//TODO: Document find_jazz_block (2)
*/
pJazzPersistenceItem JazzSource::find_jazz_block(JazzBlockId64 id64)
{
//TODO: Implement find_jazz_block (2)
}


/**
//TODO: Document free_jazz_block (1)
*/
void JazzSource::free_jazz_block(pJazzPersistenceItem p_item)
{
//TODO: Implement free_jazz_block (1)
}


/**
//TODO: Document free_jazz_block (2)
*/
bool JazzSource::free_jazz_block(const JazzBlockIdentifier *p_id)
{
//TODO: Implement free_jazz_block (2)
}


/**
//TODO: Document free_jazz_block (3)
*/
bool JazzSource::free_jazz_block(JazzBlockId64 id64)
{
//TODO: Implement free_jazz_block (3)
}


/**
//TODO: Document alloc_cache
*/
bool JazzSource::alloc_cache(int num_items, int cache_mode)
{
//TODO: Implement alloc_cache
}


/**
//TODO: Document copy_to_keepr
*/
bool JazzSource::copy_to_keepr(JazzBlockKeepr keepr,
							   JazzBlockList  p_id,
							   int			  num_blocks)
{
//TODO: Implement copy_to_keepr
}


/**
//TODO: Document copy_from_keepr
*/
bool JazzSource::copy_from_keepr(JazzBlockKeepr keepr,
								 JazzBlockList  p_id,
								 int			num_blocks)
{
//TODO: Implement copy_from_keepr
}


/**
//TODO: Document open_jazz_file
*/
int JazzSource::open_jazz_file(const char *file_name)
{
//TODO: Implement open_jazz_file
}


/**
//TODO: Document flush_jazz_file
*/
int JazzSource::flush_jazz_file()
{
//TODO: Implement flush_jazz_file
}


/**
//TODO: Document file_errors
*/
int JazzSource::file_errors()
{
//TODO: Implement file_errors
}


/**
//TODO: Document close_jazz_file
*/
int JazzSource::close_jazz_file()
{
//TODO: Implement close_jazz_file
}


} // namespace jazz_persistence

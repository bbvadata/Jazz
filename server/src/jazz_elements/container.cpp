/* Jazz (c) 2018-2021 kaalam.ai (The Authors of Jazz), using (under the same license):

	1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

	2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

      Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

      This product includes software developed at

      BBVA (https://www.bbva.com/)

	3. LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

	  Licensed under http://www.OpenLDAP.org/license.html


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


// #include <stl_whatever>


#include "src/jazz_elements/container.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Container : I m p l e m e n t a t i o n
--------------------------------------------------- */

Container::Container(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {

	num_keepers = max_num_keepers = 0;
	alloc_bytes = last_alloc_bytes = warn_alloc_bytes = fail_alloc_bytes = 0;
	p_buffer = p_left = p_right = nullptr;
	_lock_ = 0;
}

Container::~Container () {
	destroy_container();
}

/** Verifies variables in config and sets private variables accordingly.
*/
StatusCode Container::start()
{
	if (!get_conf_key("ONE_SHOT_MAX_KEEPERS", max_num_keepers)) {
		log(LOG_ERROR, "Config key ONE_SHOT_MAX_KEEPERS not found in Container::start");
		return SERVICE_ERROR_BAD_CONFIG;
	}

	int i = 0;

	if (!get_conf_key("ONE_SHOT_WARN_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key ONE_SHOT_WARN_BLOCK_KBYTES not found in Container::start");
		return SERVICE_ERROR_BAD_CONFIG;
	}
	warn_alloc_bytes = 1024; warn_alloc_bytes *= i;

	if (!get_conf_key("ONE_SHOT_ERROR_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key ONE_SHOT_ERROR_BLOCK_KBYTES not found in Container::start");
		return SERVICE_ERROR_BAD_CONFIG;
	}
	fail_alloc_bytes = 1024; fail_alloc_bytes *= i;

	return new_container();
}

/** Destroys everything and zeroes allocation.
*/
StatusCode Container::shut_down(bool restarting_service)
{
	return destroy_container();
}


/** Enter (soft lock) a Block for reading. Many readers are allowed simultaneously, but it is incompatible with writing.

	\param p_keeper		The address of the Block's BlockKeeper.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_read(pBlockKeeper p_keeper)
{
	int retry = 0;
	while (true) {
		int32_t lock = p_keeper->_lock_;
		if (lock >= 0) {
			int32_t next = lock + 1;
			if (p_keeper->_lock_.compare_exchange_weak(lock, next))
				return;
		}
		if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
			std::this_thread::yield();
			retry = 0;
		}
	}
}


/** Enter (hard lock) a Block for writing. No readers are allowed during writing.

	\param p_keeper		The address of the Block's BlockKeeper.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_write(pBlockKeeper p_keeper)
{
	int retry = 0;
	while (true) {
		int32_t lock = p_keeper->_lock_;
		if (lock >= 0) {
			int32_t next = lock - LOCK_WEIGHT_OF_WRITE;
			if (p_keeper->_lock_.compare_exchange_weak(lock, next)) {
				while (true) {
					if (p_keeper->_lock_ == -LOCK_WEIGHT_OF_WRITE)
						return;
					if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
						std::this_thread::yield();
						retry = 0;
					}
				}
			}
		}
		if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
			std::this_thread::yield();
			retry = 0;
		}
	}
}

/** Create a new Block (1): Create a Block from scratch.

	\param p_keeper			A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
							BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param cell_type		The tensor cell type in [CELL_TYPE_BYTE..CELL_TYPE_DOUBLE]
	\param dim				This defines both the rank and the dimensions of the tensor. Note that, except for the first position a
							dimension of 0 and 1 is the same dim = {3, 1} is a vector of 3 elements with rank 1, exactly like {3, 0}.
							As a matter of convention, dim should always end with a 0 except when it is MAX_TENSOR_RANK long.
							For the first dimension 1 means one element and 0 means no element. Both have rank 1. The latter is the
							typical result of a selection where no row matches the condition. Blocks never have rank == 0 and zero-element
							blocks have the same rank as the block from which they were selected. When 0 rows are selected from a block
							of dim = {r, s, t} the resulting block is {0, s, t} with size == 0 and rank == 3.
							If dim == nullptr and p_text != nullptr, dim will be set to the number of lines (see eol) in p_text when
							cell_type == CELL_TYPE_STRING.
	\param att				The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
							use the version of new_jazz_block() with parameter p_as_block.
	\param fill_tensor		How to fill the tensor. When creating anything that is not a filter, p_bool_filter is ignored and the options
							are: FILL_NEW_DONT_FILL (don't do anything with the tensor), FILL_NEW_WITH_ZERO (fill with binary zero
							no matter what the cell_type is), FILL_NEW_WITH_NA fill with the appropriate NA for the cell_type)
							When creating a filter, p_bool_filter must be a vector of length == size and the filter will be created as
							boolean (when fill_tensor == CELL_TYPE_BOOLEAN) or integer (when fill_tensor == CELL_TYPE_INTEGER)
	\param p_bool_filter	The vector of boolean (each true value means the corresponding row is selected) used when fill_tensor ==
							CELL_TYPE_BOOLEAN and fill_tensor == CELL_TYPE_INTEGER
	\param stringbuff_size	One of the possible ways to allocate space for strings is declaring this size. When this is non-zero a buffer
							will be allocated with this size plus whatever size is required by the strings in att. new_jazz_block() will
							only allocate the space and do nothing with it. The caller should assign strings with Block.set_string().
	\param p_text			The other possible way to allocate space for strings is by declaring p_text. Imagine the content of p_text
							as a text file with n = size rows that will be pushed into the tensor and the string buffer. The eol character
							separates the cells. (cell_type == CELL_TYPE_STRING & p_text != nullptr) overrides any setting in fill_tensor.
							Also, either dim should be nullptr and set automatically or its resulting size must be the same as the number
							of lines in p_text.
	\param eol				A single character that separates the cells in p_text and will not be pushed to the string buffer.

	NOTES: String buffer allocation should not be used to dynamically change attribute values. Attributes are immutable and should be
	changed	only creating a new block with new = new_jazz_block(p_as_block = old, att = new_att). String buffer allocation should only be
	used for cell_type == CELL_TYPE_STRING and either with stringbuff_size or with p_text (and eol).
	If stringbuff_size is used, Block.set_string() should be used afterwards. If p_text is used, the tensor is already filled and
	Block.set_string() **should not** be called after that.

	OWNERSHIP: Remember: the p_keeper returned on success points inside the Container. Use it as read-only and don't forget to unlock() it
	when done.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								int			  cell_type,
								int			 *dim,
								Attributes	 *att,
								int			  fill_tensor,
								bool		 *p_bool_filter,
								int			  stringbuff_size,
								const char	 *p_text,
								char		  eol)
{
//TODO: Implement new_block(1)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (2): Create a Block by slicing an existing Block.

//TODO: Document new_block(2)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pBlock		  p_as_block,
						   		pBlock		  p_row_filter,
								Attributes	 *att)
{
//TODO: Implement new_block(2)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (3): Create a binary block from text source.

//TODO: Document new_block(3)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								const char	 *p_text,
						   		pBlockHeader  p_as_block,
								Attributes	 *att)
{
//TODO: Implement new_block(3)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (4): Create a long text block by sourcing a binary block as some text serialization.

//TODO: Document new_block(4)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pBlock		  p_block,
						   		int			  format)
{
//TODO: Implement new_block(4)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (5): Create a Kind or a Tuple by merging Items.

//TODO: Document new_block(5)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pItems		  p_items,
						   		int			  build)
{
//TODO: Implement new_block(5)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (6): Create a database or sub-container inside Persisted or Volatile.

//TODO: Document new_block(6)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pLocator	  p_where,
						   		pName		  p_name,
								int			  what)
{
//TODO: Implement new_block(6)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document lock()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::lock (pBlockKeeper *p_keeper,
							pLocator	  p_locator,
							pContainer	  p_sender,
							BlockId64	  block_id)
{
//TODO: Implement lock()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document unlock()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::unlock (pBlockKeeper *p_keeper)
{
//TODO: Implement unlock()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document put()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::put (pLocator	  p_where,
						   pBlock	  p_block,
						   pContainer p_sender,
						   BlockId64  block_id)
{
//TODO: Implement put()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document remove()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::remove (pLocator	 p_what,
							  pContainer p_sender,
							  BlockId64	 block_id)
{
//TODO: Implement remove()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document get(1)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::get (pBlockKeeper *p_keeper,
						   pR_value		 p_rvalue,
						   pContainer	 p_sender,
						   BlockId64	 block_id)
{
//TODO: Implement get(1)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document sleep()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::sleep (pBlockKeeper *p_keeper)
{
//TODO: Implement sleep()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document callback()

	\param aaa		Bla, bla

	\return	Bla
*/
void Container::callback (BlockId64	 block_id,
						  StatusCode result)
{
//TODO: Implement callback()

	return;
}


/** Bla, bla, bla

//TODO: Document new_container()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_container()
{
//TODO: Implement new_container()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document destroy_container()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::destroy_container()
{
//TODO: Implement destroy_container()

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_container.ctest"
#endif

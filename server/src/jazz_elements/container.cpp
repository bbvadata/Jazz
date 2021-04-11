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

Container::Container(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config)
{
	max_num_keepers = 0;
	alloc_bytes = warn_alloc_bytes = fail_alloc_bytes = 0;
	p_buffer = p_alloc = p_free = nullptr;
	_lock_ = 0;
}


Container::~Container () { destroy_container(); }


/** Reads variables from config and sets private variables accordingly.
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
StatusCode Container::shut_down()
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


/** Release the soft lock of Block after reading. This is mandatory for each enter_read() call or it may result in permanent locking.

	\param p_keeper		The address of the Block's BlockKeeper.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_read(pBlockKeeper p_keeper)
{
	while (true) {
		int32_t lock = p_keeper->_lock_;
		int32_t next = lock - 1;
		if (p_keeper->_lock_.compare_exchange_weak(lock, next))
			return;
	}
}


/** Release the hard lock of Block after writing. This is mandatory for each enter_write() call or it may result in permanent locking.

	\param p_keeper		The address of the Block's BlockKeeper.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_write(pBlockKeeper p_keeper)
{
	while (true) {
		int32_t lock = p_keeper->_lock_;
		int32_t next = lock + LOCK_WEIGHT_OF_WRITE;
		if (p_keeper->_lock_.compare_exchange_weak(lock, next))
			return;
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


/** Create a new Block (2): Create a Kind or a Tuple by merging Items.

	\param p_keeper		A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
						BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_item		An array of data blocks (tensors or tuples) merged as a tuple (when build == BUILD_TUPLE) or an array of
						any blocks (tensors, tuples and kinds) whose metadata will be merged into a kind (build == BUILD_KIND)
	\param p_item_name	An array of item names. See how Tuples and Kinds merge below.
	\param build		What should be created, either BUILD_TUPLE or BUILD_KIND.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_as_block.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.


How Tuples and Kinds merge
--------------------------

The way a Kind can include other Kinds is by merging them together in a tree. That increases the ItemHeader.level by one. E.g, a Kind
x is made of tensors (a, b), a Kind y is made of (c, d, e) and f is a tensor. We create a kind (f, x, y). As we merge it together we
will get: (f, x_a, x_b, y_c, y_d, y_e) all of them will have level 1, except f which will be level 0. The naming convention (merging
with an underscore) is what this method does.

Note that: The "level-trick" makes the structure much simpler by just storing the tensors, the tree is encoded in the levels of each tensor
but not necessary to match tuples to kinds. The resulting names must all be different or it will return an error.


	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pItems		  p_item,
								pNames		  p_item_name,
						   		int			  build,
								Attributes	 *att)
{
//TODO: Implement new_block(2)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (3): Create a Block by slicing an existing Block.

	\param p_keeper		A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
						BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_block		The block we want to filter from. The resulting block will be a subset of the rows (selection on the first
						dimension of the tensor). This can be either a tensor or a Tuple. In the case of a Tuple, all the tensors must
						have the same first dimension.
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean of the same length as the tensor in
						p_as_block (or all of them if it is a Tuple) (p_row_filter->filter_type() == FILTER_TYPE_BOOLEAN) or a vector of
						integers (p_row_filter->filter_type() == FILTER_TYPE_INTEGER) in that range.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_as_block.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pBlock		  p_block,
						   		pBlock		  p_row_filter,
								Attributes	 *att)
{
//TODO: Implement new_block(3)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (4): Create a Tuple or a Kind by selecting a list of items from another Tuple or Kind.

	\param p_keeper		A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
						BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_block		The block we want to filter from. It must be either a Kind or a Tuple and the resulting block will be of the
						same type.
	\param p_item_name	The vector of item names to be selected that must exist in p_block.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_as_block.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pBlock		  p_block,
								pNames		  p_item_name,
								Attributes	 *att)
{
//TODO: Implement new_block(4)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (5): Create a binary block from text source.

	\param p_keeper		A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
						BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_source		A pointer to the text source passed by reference. In case of an error, the pointer will be modified to point
						at the conflicting character.
	\param p_as_block	Possibly, a the address of a Block of metadata (either a BlockHeader or a Kind) containing and already parsed
						metadata describing the block. Otherwise, an extra pass will parse the text to build this metadata.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_as_block.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pChar		 &p_source,
						   		pBlock		  p_as_block,
								Attributes	 *att)
{
//TODO: Implement new_block(5)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (6): Create a long text block by sourcing a binary block as some text serialization.

	\param p_keeper		A pointer to a BlockKeeper passed by reference. If successful, the Container will return a pointer to a
						BlockKeeper inside the Container. The caller can only use it read-only and **must** unlock() it when done.
	\param p_block		The binary block we want to source.
	\param format		Either AS_BEBOP, AS_JSON or AS_CPP. The latter is only valid to Kind metadata. Tensors of data in AS_BEBOP and
						AS_JSON are identical. These formats only differ in how they describe metadata.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_as_block.

	\return	SERVICE_NO_ERROR on success (and a valid p_keeper), or some negative value (error). There is no async interface in this method.
*/
StatusCode Container::new_block(pBlockKeeper *p_keeper,
								pBlock		  p_block,
						   		int			  format,
								Attributes	 *att)
{
//TODO: Implement new_block(6)

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
	\param p_sender	This argument, together with block_id, enables the async interface, used by Remote only. (See Async interface below)
	\param block_id	Used by the async interface. (See Async interface below)

	\return	Bla

Async Interface
---------------

The async interface is used by Remote to track delayed operations. The call itself, when no immediate error applies, returns
a SERVICE_ONGOING_ASYNC_OP code and continues via a .callback() call.

The caller must be a Container descendant and pass its own address as p_sender. It must also provide some block_id used to track the
operation via the callback(). When a successfull or error completion code is available, the Container will receive a .callback() call.

A **NOTE for put()**: The callback() will return the completion status for the caller to know (retry, notify, etc.). In either (error or
success) case, there are no further actions to take.

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
	\param p_sender	This argument, together with block_id, enables the async interface, used by Remote only. (See Async interface below)
	\param block_id	Used by the async interface. (See Async interface below)

	\return	Bla

Async Interface
---------------

The async interface is used by Remote to track delayed operations. The call itself, when no immediate error applies, returns
a SERVICE_ONGOING_ASYNC_OP code and continues via a .callback() call.

The caller must be a Container descendant and pass its own address as p_sender. It must also provide some block_id used to track the
operation via the callback(). When a successfull or error completion code is available, the Container will receive a .callback() call.

A **NOTE for remove()**: The callback() will return the completion status for the caller to know (retry, notify, etc.). In either (error or
success) case, there are no further actions to take.

	\return	Bla
*/
StatusCode Container::remove (pLocator	 p_what,
							  pContainer p_sender,
							  BlockId64	 block_id)
{
//TODO: Implement remove()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Get a Block (1) : Complete get() implementing a full R_value with contract and async calls

//TODO: Document get(1)

	\param aaa		Bla, bla
	\param p_sender	This argument, together with block_id, enables the async interface, used by Remote only. (See Async interface below)
	\param block_id	Used by the async interface. (See Async interface below)

	\return	Bla

Async Interface
---------------

The async interface is used by Remote to track delayed operations. The call itself, when no immediate error applies, returns
a SERVICE_ONGOING_ASYNC_OP code and continues via a .callback() call.

The caller must be a Container descendant and pass its own address as p_sender. It must also provide some block_id used to track the
operation via the callback(). When a successfull or error completion code is available, the Container will receive a .callback() call.

A **NOTE for get()**: The returned p_keeper is owned by the Remote. Before the .callback() event, it will have
p_keeper->status == BLOCK_STATUS_ASYNC_WAIT. After an error is returned, the caller should forget the p_keeper and not use it.
After a success is returned, the caller will get: p_keeper->status == BLOCK_STATUS_READY and a valid p_block. In this case, the caller
**must** .unlock() the p_keeper when done with it.
*/
StatusCode Container::get (pBlockKeeper *p_keeper,
						   pR_value		 p_rvalue,
						   pContainer	 p_sender,
						   BlockId64	 block_id)
{
//TODO: Implement get(1)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Get a Block (2) : Simple get() implementing only local, sync, full block "as is" get().

//TODO: Document get(2)

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::get (pBlockKeeper *p_keeper,
						   pLocator		 p_what)
{
//TODO: Implement get(2)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

	The root class Container does not add any base names.

*/
void Container::base_names (BaseNames &base_names){}


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

	return SERVICE_NO_ERROR;
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

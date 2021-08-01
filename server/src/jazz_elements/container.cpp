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


#include "src/jazz_elements/container.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Container : I m p l e m e n t a t i o n
--------------------------------------------------- */

Container::Container(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {

	max_num_keepers = 0;
	alloc_bytes = warn_alloc_bytes = fail_alloc_bytes = 0;
	p_buffer = p_alloc = p_free = nullptr;
	_lock_ = 0;
}


Container::~Container () { destroy_container(); }


/** Reads variables from config and sets private variables accordingly.
*/
StatusCode Container::start() {

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
StatusCode Container::shut_down() {

	return destroy_container();
}


/** Enter (soft lock) a Block for reading. Many readers are allowed simultaneously, but it is incompatible with writing.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_read(pTransaction p_txn) {
	int retry = 0;

	while (true) {
		int32_t lock = p_txn->_lock_;

		if (lock >= 0) {
			int32_t next = lock + 1;

			if (p_txn->_lock_.compare_exchange_weak(lock, next))
				return;
		}

		if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
			std::this_thread::yield();
			retry = 0;
		}
	}
}


/** Enter (hard lock) a Block for writing. No readers are allowed during writing.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_write(pTransaction p_txn) {
	int retry = 0;

	while (true) {
		int32_t lock = p_txn->_lock_;

		if (lock >= 0) {
			int32_t next = lock - LOCK_WEIGHT_OF_WRITE;

			if (p_txn->_lock_.compare_exchange_weak(lock, next)) {
				while (true) {
					if (p_txn->_lock_ == -LOCK_WEIGHT_OF_WRITE)
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

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_read(pTransaction p_txn) {
	while (true) {
		int32_t lock = p_txn->_lock_;
		int32_t next = lock - 1;

		if (p_txn->_lock_.compare_exchange_weak(lock, next))
			return;
	}
}


/** Release the hard lock of Block after writing. This is mandatory for each enter_write() call or it may result in permanent locking.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_write(pTransaction p_txn) {
	while (true) {
		int32_t lock = p_txn->_lock_;
		int32_t next = lock + LOCK_WEIGHT_OF_WRITE;

		if (p_txn->_lock_.compare_exchange_weak(lock, next))
			return;
	}
}


/** Create a new Block (1): Create a Tensor from raw data specifying everything from scratch.

	\param p_txn			A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
							Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
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
	\param att				The attributes to set when creating the block. They are be immutable.
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
	changed	only creating a new block with new = new_jazz_block(p_from = old, att = new_att). String buffer allocation should only be
	used for cell_type == CELL_TYPE_STRING and either with stringbuff_size or with p_text (and eol).
	If stringbuff_size is used, Block.set_string() should be used afterwards. If p_text is used, the tensor is already filled and
	Block.set_string() **should not** be called after that.

	OWNERSHIP: Remember: the p_txn returned on success points inside the Container. Use it as read-only and don't forget to destroy() it
	when done.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								int			  cell_type,
								int			 *dim,
								int			  fill_tensor,
								bool		 *p_bool_filter,
								int			  stringbuff_size,
								const char	 *p_text,
								char		  eol,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	StaticBlockHeader hea;

	hea.cell_type = cell_type;

	int text_length = 0, num_lines = 0;

	if (p_text != nullptr) {
		if (cell_type != CELL_TYPE_STRING || fill_tensor != FILL_WITH_TEXTFILE) {
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		const char *pt = p_text;
		while (pt[0]) {
			if (pt[0] == eol) {
				if (!pt[1])
					break;
				num_lines++;
			}
			pt++;
		}
		text_length = (uintptr_t) pt - (uintptr_t) p_text - num_lines;
		num_lines++;
	}

	if (dim == nullptr) {
		if (p_text == nullptr) {
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		TensorDim i_dim;
		i_dim.dim[0] = num_lines;
		i_dim.dim[1] = 0;
#ifdef DEBUG				// Initialize i_dim for Valgrind.
		i_dim.dim[2] = 0;
		i_dim.dim[3] = 0;
		i_dim.dim[4] = 0;
		i_dim.dim[5] = 0;
#endif
		reinterpret_cast<pBlock>(&hea)->set_dimensions(i_dim.dim);
	} else {
		reinterpret_cast<pBlock>(&hea)->set_dimensions(dim);

		if (num_lines && (num_lines != hea.size)){
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}
	}

	hea.num_attributes = 0;

	hea.total_bytes = (uintptr_t) reinterpret_cast<pBlock>(&hea)->p_string_buffer() - (uintptr_t) (&hea) + sizeof(StringBuffer) + 4;

	if (att	== nullptr) {
		hea.total_bytes += 2*sizeof(int);
		hea.num_attributes++;
	} else {
		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);
			if (len) hea.total_bytes += len + 1;
			hea.num_attributes++;
		}
		hea.total_bytes += 2*hea.num_attributes*sizeof(int);
	}

	hea.total_bytes += stringbuff_size + text_length + num_lines;

	p_txn->p_block = (pBlock) malloc(hea.total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_internal(p_txn);
		return SERVICE_ERROR_NO_MEM;
	}

	memcpy(p_txn->p_block, &hea, sizeof(BlockHeader));

	p_txn->p_block->num_attributes = 0;

	if (att	== nullptr) {
		AttributeMap void_att;
		void_att [BLOCK_ATTRIB_EMPTY] = nullptr;
		p_txn->p_block->set_attributes(&void_att);
	} else {
		p_txn->p_block->set_attributes(att);
	}

#ifdef DEBUG	// Initialize the RAM between the end of the tensor and the base of the attribute key vector for Valgrind.
	{
		char *pt1 = (char *) &p_txn->p_block->tensor + (p_txn->p_block->cell_type & 0xf)*p_txn->p_block->size,
			 *pt2 = (char *) p_txn->p_block->align_128bit((uintptr_t) pt1);

		while (pt1 < pt2) {
			pt1[0] = 0;
			pt1++;
		}
	}
#endif

	if (p_text != nullptr) {
		p_txn->p_block->has_NA = false;
		pStringBuffer psb = p_txn->p_block->p_string_buffer();

		int offset = psb->last_idx;

		char *pt_out = &psb->buffer[offset];

		int row = 1, len = 0;
		const char *pt_in = p_text;

		p_txn->p_block->tensor.cell_int[0] = offset;

		while (pt_in[0]) {
			offset++;
			if (pt_in[0] != eol) {
				pt_out[0] = pt_in[0];
				len++;
			} else {
				if (!len)
					p_txn->p_block->tensor.cell_int[row - 1] = STRING_EMPTY;

				if (!pt_in[1])
					break;

				pt_out[0] = 0;

				p_txn->p_block->tensor.cell_int[row] = offset;

				len = 0;
				row++;
			}
			pt_out++;
			pt_in++;
		}
		if (!len)
			p_txn->p_block->tensor.cell_int[row - 1] = STRING_EMPTY;

		pt_out[0] = 0;

		psb->last_idx			= offset + (pt_in[0] == 0);
		psb->stop_check_4_match = true;							// Block::get_string_offset() does not support match with empty strings.
	} else {
		switch (fill_tensor) {
		case FILL_NEW_DONT_FILL:
			p_txn->p_block->has_NA = p_txn->p_block->cell_type != CELL_TYPE_BYTE;
			break;

		case FILL_NEW_WITH_ZERO:
			memset(&p_txn->p_block->tensor, 0, (p_txn->p_block->cell_type & 0xf)*p_txn->p_block->size);
			p_txn->p_block->has_NA =	  (p_txn->p_block->cell_type == CELL_TYPE_STRING)
									   || (p_txn->p_block->cell_type == CELL_TYPE_TIME);
			break;

		case FILL_NEW_WITH_NA:
			p_txn->p_block->has_NA = true;

			switch (cell_type) {
			case CELL_TYPE_BYTE_BOOLEAN:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_byte[i] = BOOLEAN_NA;
				break;

			case CELL_TYPE_INTEGER:
			case CELL_TYPE_FACTOR:
			case CELL_TYPE_GRADE:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_int[i] = INTEGER_NA;
				break;

			case CELL_TYPE_BOOLEAN:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_uint[i] = BOOLEAN_NA;
				break;

			case CELL_TYPE_SINGLE: {
				u_int una = reinterpret_cast<u_int*>(&SINGLE_NA)[0];

				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_uint[i] = una;
				break; }

			case CELL_TYPE_STRING:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_int[i] = STRING_NA;
				break;

			case CELL_TYPE_LONG_INTEGER:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_longint[i] = LONG_INTEGER_NA;
				break;

			case CELL_TYPE_TIME:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_longint[i] = TIME_POINT_NA;
				break;

			case CELL_TYPE_DOUBLE: {
				uint64_t una = reinterpret_cast<uint64_t*>(&DOUBLE_NA)[0];

				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_ulongint[i] = una;
				break; }

			default:
				destroy_internal(p_txn);
				return SERVICE_ERROR_NEW_BLOCK_ARGS;		// No silent fail, JAZZ_FILL_NEW_WITH_NA is undefined for the type
			}
			break;

		case FILL_BOOLEAN_FILTER:
			p_txn->p_block->has_NA = false;
			if (p_bool_filter == nullptr || p_txn->p_block->filter_type() != FILTER_TYPE_BOOLEAN) {
				destroy_internal(p_txn);
				return SERVICE_ERROR_NEW_BLOCK_ARGS;		// No silent fail, cell_type and rank must match
			}
			memcpy(&p_txn->p_block->tensor, p_bool_filter, p_txn->p_block->size);
			break;

		case FILL_INTEGER_FILTER: {
			p_txn->p_block->has_NA = false;
			if (p_bool_filter == nullptr || p_txn->p_block->filter_type() != FILTER_TYPE_INTEGER) {
				destroy_internal(p_txn);
				return SERVICE_ERROR_NEW_BLOCK_ARGS;		// No silent fail, cell_type and rank must match
			}
			int j = 0;
			for (int i = 0; i < p_txn->p_block->size; i ++) {
				if (p_bool_filter[i]) {
					p_txn->p_block->tensor.cell_int[j] = i;
					j++;
				}
			}
			p_txn->p_block->range.filter.length = j;

#ifdef DEBUG												// Initialize the RAM on top of the filter for Valgrind.
			for (int i = p_txn->p_block->range.filter.length; i < p_txn->p_block->size; i ++)
				p_txn->p_block->tensor.cell_int[i] = 0;
#endif
			break; }

		default:
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;			// No silent fail, fill_tensor is invalid
		}
	}

	p_txn->status = BLOCK_STATUS_READY;

	return SERVICE_NO_ERROR;
}


/** Create a new Block (2): Create a Kind or Tuple from arrays of StaticBlockHeader, names, and, in the case of a tuple, Tensors.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param num_items	The number of items the Kind or Tuple will have.
	\param p_hea		A vector of num_items pointers to StaticBlockHeaders defining the Kind or Tuple.
	\param p_names		An array of num_items Name structures by which the items will go.
	\param p_block		The data, only for tuples. It will normally be the same as p_hea but that will not be checked. The data is
						simply assumed to have the exact shape and type defined in p_hea. If it is nullptr, a Kind will be created,
						otherwise a Tuple will be created and data will be copied from here.
	\param p_dims		For Kinds only, the names of the dimensions. Note that p_hea must have negative values for the dimensions, just
						like when Kinds are built using Kind.new_kind() followed by Kind.add_item()
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction	   &p_txn,
								int					num_items,
								pStaticBlockHeader	p_hea[],
								Name				p_names[],
								pBlock				p_block[],
								AttributeMap	   *p_dims,
								AttributeMap	   *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	StaticBlockHeader hea;
	TensorDim i_dim;
	i_dim.dim[0] = num_items;
	i_dim.dim[1] = 0;

	if (p_block == nullptr)
		hea.cell_type = CELL_TYPE_KIND_ITEM;
	else
		hea.cell_type = CELL_TYPE_TUPLE_ITEM;

	reinterpret_cast<pBlock>(&hea)->set_dimensions(i_dim.dim);

	hea.num_attributes = 0;

	hea.total_bytes = (uintptr_t) reinterpret_cast<pBlock>(&hea)->p_string_buffer() - (uintptr_t) (&hea) + sizeof(StringBuffer) + 4;

	if (att	== nullptr) {
		hea.total_bytes += 2*sizeof(int);
		hea.num_attributes++;
	} else {
		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);

			if (len)
				hea.total_bytes += len + 1;

			hea.num_attributes++;
		}
		hea.total_bytes += 2*hea.num_attributes*sizeof(int);
	}

	for (int i = 0; i < num_items; i++) {
		pChar p_name = (pChar) &p_names[i];

		hea.total_bytes += sizeof(ItemHeader) + strlen(p_name) + 1;

		if (p_block != nullptr)
			hea.total_bytes += p_block[i]->total_bytes + 15;	// 15 == worst case of align 128-bit
	}

	p_txn->p_block = (pBlock) malloc(hea.total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_internal(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

	if (p_block == nullptr) {
		if (!reinterpret_cast<pKind>(p_txn->p_block)->new_kind(num_items, hea.total_bytes, *att)) {
			destroy_internal(p_txn);

			return SERVICE_ERROR_BAD_KIND;
		}
		if (p_dims == nullptr) {
			AttributeMap void_dim = {};

			for (int i = 0; i < num_items; i++) {
				pChar p_name = (pChar) &p_names[i];

				if (!reinterpret_cast<pKind>(p_txn->p_block)->add_item(i, p_name, p_hea[i]->range.dim, p_hea[i]->cell_type, void_dim)) {
					destroy_internal(p_txn);

					return SERVICE_ERROR_BAD_KIND;
				}
			}
		} else {
			for (int i = 0; i < num_items; i++) {
				pChar p_name = (pChar) &p_names[i];

				if (!reinterpret_cast<pKind>(p_txn->p_block)->add_item(i, p_name, p_hea[i]->range.dim, p_hea[i]->cell_type, *p_dims)) {
					destroy_internal(p_txn);

					return SERVICE_ERROR_BAD_KIND;
				}
			}
		}
	} else {
		if (!reinterpret_cast<pTuple>(p_txn->p_block)->new_tuple(num_items, p_block, p_names, hea.total_bytes, *att)) {
			destroy_internal(p_txn);

			return SERVICE_ERROR_BAD_TUPLE;
		}

	}

	p_txn->status = BLOCK_STATUS_READY;

	return SERVICE_NO_ERROR;
}


/** Create a new Block (3): Create a Tensor by selecting rows (filtering) from another Tensor.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param p_from		The block we want to filter from. The resulting block will be a subset of the rows (selection on the first
						dimension of the tensor). This can be either a tensor or a Tuple. In the case of a Tuple, all the tensors must
						have the same first dimension.
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean of the same length as the tensor in
						p_from (or all of them if it is a Tuple) (p_row_filter->filter_type() == FILTER_TYPE_BOOLEAN) or a vector of
						integers (p_row_filter->filter_type() == FILTER_TYPE_INTEGER) in that range.
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from,
						   		pBlock		  p_row_filter,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	if (p_from == nullptr || p_from->size < 0 || p_from->range.dim[0] < 1) {
		destroy_internal(p_txn);
		return SERVICE_ERROR_NEW_BLOCK_ARGS;
	}

	int tensor_diff		= 0,
		old_tensor_size = p_from->size*(p_from->cell_type & 0xff),
		bytes_per_row,
		selected_rows;

	if (p_row_filter != nullptr) {
		int	tensor_rows = p_from->size/p_from->range.dim[0];

		if (!p_row_filter->can_filter(p_from)){
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		if (p_row_filter->cell_type == CELL_TYPE_BYTE_BOOLEAN) {
			selected_rows = 0;
			for (int i = 0; i < p_row_filter->size; i++)
				if (p_row_filter->tensor.cell_bool[i])
					selected_rows++;
		} else {
			selected_rows = p_row_filter->range.filter.length;
		}
		if (p_from->size) {
			bytes_per_row		= old_tensor_size/tensor_rows;
			int new_tensor_size = selected_rows*bytes_per_row;

			old_tensor_size = (uintptr_t) p_from->align_128bit(old_tensor_size);
			new_tensor_size = (uintptr_t) p_from->align_128bit(new_tensor_size);

			tensor_diff = new_tensor_size - old_tensor_size;
		}
	}

	int attrib_diff		   = 0,
		new_num_attributes = 0;

	if (att	!= nullptr) {
		int new_attrib_bytes = 0;

		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);
			if (len)
				new_attrib_bytes += len + 1;
			new_num_attributes++;
		}

		if (!new_num_attributes) {
			destroy_internal(p_txn);
			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		new_attrib_bytes += new_num_attributes*2*sizeof(int);

		int old_attrib_bytes = p_from->num_attributes*2*sizeof(int);

		if (p_from->cell_type != CELL_TYPE_STRING) {
			pStringBuffer psb = p_from->p_string_buffer();
			old_attrib_bytes += std::max(0, psb->buffer_size - 4);
		}

		attrib_diff = new_attrib_bytes - old_attrib_bytes;
	}

	int total_bytes = p_from->total_bytes + tensor_diff + attrib_diff;

	p_txn->p_block = (pBlock) malloc(total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_internal(p_txn);
		return SERVICE_ERROR_NO_MEM;
	}

	memcpy(p_txn->p_block, p_from, sizeof(BlockHeader));

	p_txn->p_block->total_bytes = total_bytes;

	if (tensor_diff) {
		p_txn->p_block->size = selected_rows*p_from->range.dim[0];

		u_char *p_dest = &p_txn->p_block->tensor.cell_byte[0],
			   *p_src  = &p_from->tensor.cell_byte[0];

		if (p_row_filter->cell_type == CELL_TYPE_BYTE_BOOLEAN) {
			for (int i = 0; i < p_row_filter->size; i++) {
				if (p_row_filter->tensor.cell_bool[i]) {
					memcpy(p_dest, p_src, bytes_per_row);
					p_dest = p_dest + bytes_per_row;
				}
				p_src = p_src + bytes_per_row;
			}
		} else {
			for (int i = 0; i < p_row_filter->range.filter.length; i++) {
				memcpy(p_dest, p_src + p_row_filter->tensor.cell_int[i]*bytes_per_row, bytes_per_row);
				p_dest = p_dest + bytes_per_row;
			}
		}
	} else {
		memcpy(&p_txn->p_block->tensor, &p_from->tensor, old_tensor_size);
	}

	if (att	!= nullptr)	{
		if (p_from->cell_type != CELL_TYPE_STRING) {
			p_txn->p_block->num_attributes = 0;
			p_txn->p_block->set_attributes(att);
		} else {
			p_txn->p_block->num_attributes = new_num_attributes;

			pStringBuffer p_nsb = p_txn->p_block->p_string_buffer(), p_osb = p_from->p_string_buffer();

			p_txn->p_block->init_string_buffer();

			memcpy(&p_nsb->buffer, &p_osb->buffer, p_osb->buffer_size);

			p_nsb->last_idx = p_osb->last_idx;

			int i = 0;
			int *ptk = p_txn->p_block->p_attribute_keys();

			for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
				ptk[i] = it->first;
				ptk[i + new_num_attributes] = p_txn->p_block->get_string_offset(p_nsb, it->second);

				i++;
			}
		}
	} else {
		memcpy(p_txn->p_block->p_attribute_keys(), p_from->p_attribute_keys(), p_from->num_attributes*2*sizeof(int));

		pStringBuffer p_nsb = p_txn->p_block->p_string_buffer(), p_osb = p_from->p_string_buffer();

		memcpy(p_nsb, p_osb, p_osb->buffer_size + sizeof(StringBuffer));
	}

	p_txn->status = BLOCK_STATUS_READY;
	return SERVICE_NO_ERROR;
}


/** Create a new Block (4): Create a Tensor by selecting an item from a Tuple.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param p_from	The Tuple from which the item is selected.
	\param name		The name of the item to be selected.
	\param att		The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
					use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

The Tuple already has a method .block() that does this, the difference is the .block() method returns a pointer inside the Tuple
while this makes a copy to a new block, possibly with attributes.
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pTuple		  p_from,
						   		pChar		  name,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement new Block (4)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (5): Create a Tensor, Kind or Tuple from a Text block kept as a Tensor of CELL_TYPE_BYTE of rank == 1.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param p_from_text	The block containing a valid serialization of Tensor, Kind or Tuple.
	\param cell_type	The expected output type.
	\param p_as_kind	For Tuples only, the definition of the exact types of each item. The serialization contains names, but not types
						to make it JSON-like. E.g., "[temperature:[[20,21][22,22], sensation:["cold", "mild"]]" This can be skipped
						by passing a nullptr. In that case, the items will be guessed only as CELL_TYPE_INTEGER, CELL_TYPE_DOUBLE or
						CELL_TYPE_STRING. The serialization of a Kind includes types and does not use this.
						E.g. "[temperature:(INTEGER[num_places,2]),sensation:(STRING[num_places])]"
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from_text,
						   		int			  cell_type,
								pKind		  p_as_kind,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement new Block (5)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (6): Create a Tensor of CELL_TYPE_BYTE of rank == 1 with a text serialization of a Tensor, Kind or Tuple.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param p_from_raw	The block to be serialized
	\param p_fmt		An optional numerical precision format specifier. In the case of Tuples, if this is used all items with numerical
						tensors will use it. (This may imply converting integer to double depending on the specifier.)
	\param att			The attributes to set when creating the block. They are be immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from_raw,
						   		pChar		  p_fmt,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement new Block (6)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Create a new Block (7): Create an empty Index block.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy() it when done.
	\param cell_type	The type of index (from CELL_TYPE_INDEX_II to CELL_TYPE_INDEX_SS)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike all the other blocks, Tensor, Kind and Tuple, this returns a header with an std::map. Thefore, it is dynamically allocated,
by just using it. As such, it is not movable and cannot be used in any transactions other that Channels serializing it as a
Tuple(key, value). When no longer needed, it has to be destroy()-ed just like the other Blocks created with new_block() and the Container
will take care of freeing the std::map before destroying the transaction.

*/
StatusCode Container::new_block(pTransaction &p_txn, int cell_type) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement new Block (7)

	return SERVICE_NOT_IMPLEMENTED;
}


/** Dealloc the Block in the p_tnx->p_block (if not null) and free the Transaction inside a Container.

	\param p_txn	A pointer to a valid Transaction passed by reference. Once finished, p_txn is set to nullptr to avoid reusing.

NOTE: Different Container descendants, may do different things with blocks inside the transaction. In the case of this one-shot
allocation, the p_block will be freed and the p_route ignored (since this Container does not allocate anything here).
Persisted or Volatile Blocks will not be destroyed when a Transaction referring to them is destroyed. If the descentant allocates
a p_route, it should also free it when the Transaction is destroyed.
*/
void Container::destroy (pTransaction &p_txn) {

	if (p_txn->p_owner == nullptr) {
		log_printf(LOG_ERROR, "Transaction %p has no p_owner", p_txn);

		return;
	}
	if (p_txn->p_owner != this) {
		p_txn->p_owner->destroy(p_txn);

		return;
	}

	enter_write		 (p_txn);
	destroy_internal (p_txn);
}


/** Block retrieving interface: A general API to be inherited (and possibly extended)

	\param p_what	Some string with a locator that the Container can handle.
	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy()-ed when the caller is done.
*/
StatusCode Container::get (pChar p_what, pTransaction &p_txn) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Block storing interface: A general API to be inherited (and possibly extended)

	\param p_block	A block to be stored. Notice it is a block, not a Transaction. If necessary, the Container will make a copy, write to
					disc, PUT it via http, etc. The container does not own the pointer in any way.
	\param p_where	Some string with a locator that the Container can handle.

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Container::put (pBlock p_block, pChar p_where) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Block deletion interface: Erase a block at a locator inside the Container

	\param p_what Some string with a locator that the Container can handle.

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Container::remove (pChar p_what) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Block copying interface: A general API to be inherited (and possibly extended)

	\param p_what	Some string with a locator that the Container can handle.
	\param p_where	Some string with a locator that the Container can handle.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy()-ed when the caller is done.
*/
StatusCode Container::copy (pChar  p_what, pChar  p_where) {
	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

	The root class Container does not add any base names.

*/
void Container::base_names (BaseNames &base_names) {}


/** Creates the buffers for new_transaction()/free_keeper()

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some error.
*/
StatusCode Container::new_container() {
	if (p_buffer != nullptr || max_num_keepers <= 0)
#if defined CATCH_TEST
		destroy_container();
#else
		return SERVICE_ERROR_STARTING;
#endif

	alloc_bytes = 0;

	alloc_warning_issued = false;

	_lock_ = 0;

	p_buffer = (pStoredTransaction) malloc(max_num_keepers*sizeof(StoredTransaction));

	if (p_buffer == nullptr)
		return SERVICE_ERROR_NO_MEM;

	p_alloc = nullptr;
	p_free  = &p_buffer[max_num_keepers - 1];

	p_free->p_next = nullptr;

	pStoredTransaction pt = p_free;
	for (int i = 1; i < max_num_keepers; i ++) {
		p_free--;

		p_free->p_next = pt--;
	}

	return SERVICE_NO_ERROR;
}


/** Destroys everything: all keepers and the buffer itself

	\return	SERVICE_NO_ERROR.
*/
StatusCode Container::destroy_container() {
	if (p_buffer != nullptr) {
		while (p_alloc != nullptr) {
			pTransaction pt = p_alloc;
			destroy_internal(pt);
		}

		free (p_buffer);
	}
	alloc_bytes = 0;
	p_buffer = p_alloc = p_free = nullptr;
	_lock_ = 0;

	return SERVICE_NO_ERROR;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_container.ctest"
#endif

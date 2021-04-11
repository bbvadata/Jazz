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


#include "src/jazz_elements/tuple.h"


namespace jazz_elements
{

/** Check the internal validity of a Tuple (item structure, dimensions, etc.)

	\return KIND_TYPE_NOTAKIND on error or KIND_TYPE_TUPLE if every check passes ok.
*/
int Tuple::audit()
{

	return KIND_TYPE_NOTAKIND;
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
StatusCode Container::new_block(pBlockKeeper &p_keeper,
								pItems		  p_item,
								pNames		  p_item_name,
						   		int			  build,
								Attributes	 *att)
{
//TODO: Implement new_block(2)

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
StatusCode Container::new_block(pBlockKeeper &p_keeper,
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
StatusCode Container::new_block(pBlockKeeper &p_keeper,
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
StatusCode Container::new_block(pBlockKeeper &p_keeper,
								pBlock		  p_block,
						   		int			  format,
								Attributes	 *att)
{
//TODO: Implement new_block(6)

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_tuple.ctest"
#endif

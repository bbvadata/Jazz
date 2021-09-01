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


#include "src/jazz_elements/channel.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_VOLATILE
#define INCLUDED_JAZZ_ELEMENTS_VOLATILE


namespace jazz_elements
{

/** \brief A pointer to a Transaction-descendant wrapper over a Block for Volatile blocks.
*/
typedef struct VolatileTransaction *pVolatileTransaction;


/** \brief VolatileTransaction: A Transaction-descendant wrapper over a Block for Volatile blocks.

This structure supports all the types in Volatile (deque, queue, tree).
*/
struct VolatileTransaction: Transaction {
	union {
		pVolatileTransaction p_prev;					///< Pointer to the previous node in a deque ...
		pVolatileTransaction p_parent;					///< ... or parent node in a tree.
	};
	pVolatileTransaction p_next;						///< Pointer to the next node in a deque or next sibling in a tree.
	union {
		pVolatileTransaction p_child;					///< Pointer to the first child in a tree ...
		double priority;								///< ... or priority value in a queue.
	};
	union {
		int	level;										///< Level in the AA tree (used for auto-balancing) ...
		int num_wins;									///< ... or MCTS tree node number of wins.
	};
	union {
		int	times_used;									///< Times the block has been reassigned in the queue ...
		int num_visits;									///< ... or MCTS tree node number of visits.
	};
	uint64_t id_hash;									///< Node locator hash required to find the ID of a related (next, ...) node.
};


/** \brief Volatile: A Service to manage data objects in RAM.

Node Method Reference
---------------------

Some bases in Volatile, like **deque** which is just a key/value store where each entity is a new keyspace. Keys are keys and values
are blocks of any kind. Other bases have a syntax that allows operating with entities or nodes, like pop()-ing the highest priority
node in a **queue**, accessing to nodes in a tree using the tree pointers or keys to node ids, etc.

The reference for this is this docstring. All entities and keys can be removed with remove(), each time keys are returned, they are
inside a block of just one string, see the methods of each base for more.

Methods in deque
----------------

A deque is a key-value store. It is created empty via new_entity() and you can just get(), header(), put(), remove() or copy(). In order
to  access all the blocks in a deque, the key of the first block will be returned by get()ting //deque/entity/~first. And the keys of
any node can be obtained by //deque/entity/key~next and //deque/entity/key~prev.

Methods in index
----------------

Index both exposes and serializes Index type blocks. An entity inside index is **one single** Index. When get()ting them by key, you get
the value stored in the Index. To create a new one, just new_entity() //index/name/~ss (ii, is, si or ss). To populate one just
put() to //index/name/~load with a Tuple of the appropriate Kind. To save one, just get() //index/name/~save.

Methods in queue
----------------

A priority queue is implemented as self balanced binary trees. Each time you push a block, you must put() to a key with a priority by
putting to //queue/name/key~0.977 (where 0.977 can be serialized to a double). The queue is created by new_entity() //queue/name/~5000
(where 5000 is the mandatory maximum number of nodes). When the queue fills, lower priority nodes are discarded. You can also get()
to //queue/name/~pop_highest, //queue/name/~peek_highest, //queue/name/~pop_lowest, //queue/name/~peek_lowest. And you can get()
nodes by key as in a deque (if they haven't been pop()ed or been discarded).

Methods in tree
---------------

When a tree is created empty, new_entity() //tree/name/, the first node pushed must have key == **root** and is the only node without
a parent. Any node is created by put()ing to //tree/name/key~parentname (where parentname must exist). All nodes support querying keys to
their parent, siblings and first child via: get() //tree/name/key~parent, //tree/name/key~next, //tree/name/key~child.
*/
class Volatile : public Container {

	public:

		Volatile(pLogger	 a_logger,
				 pConfigFile a_config);

		StatusCode start	();
		StatusCode shut_down();

		// The easy interface (Requires explicit pulling because of the native interface using the same names.)

		using Container::get;
		using Container::header;
		using Container::put;
		using Container::new_entity;
		using Container::remove;
		using Container::copy;

		virtual StatusCode new_transaction(pTransaction &p_txn);
		virtual void destroy_transaction  (pTransaction &p_txn);

		// The "native" interface

		virtual StatusCode get		 (pTransaction		&p_txn,
									  Locator			&what);
		virtual StatusCode get		 (pTransaction		&p_txn,
									  Locator			&what,
									  pBlock			 p_row_filter);
		virtual StatusCode get		 (pTransaction		&p_txn,
							  		  Locator			&what,
							  		  pChar				 name);
		virtual StatusCode header	 (StaticBlockHeader	&hea,
									  Locator			&what);
		virtual StatusCode header	 (pTransaction		&p_txn,
									  Locator			&what);
		virtual StatusCode put		 (Locator			&where,
									  pBlock			 p_block,
									  int				 mode = WRITE_ALWAYS_COMPLETE);
		virtual StatusCode new_entity(Locator			&where);
		virtual StatusCode remove	 (Locator			&where);
		virtual StatusCode copy		 (Locator			&where,
									  Locator			&what);

		// Support for container names in the API .base_names()

		void base_names(BaseNames &base_names);

#ifndef CATCH_TEST
	protected:
#endif

#ifndef CATCH_TEST
	private:
#endif

		StatusCode new_volatile();
		StatusCode destroy_volatile();
};
typedef Volatile *pVolatile;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_VOLATILE

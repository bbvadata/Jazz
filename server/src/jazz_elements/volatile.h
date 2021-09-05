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

#define BASE_DEQUE_10BIT		0x0a4		//< First 10 bits of base "deque"
#define BASE_INDEX_10BIT		0x1c9		//< First 10 bits of base "index"
#define BASE_QUEUE_10BIT		0x2b1		//< First 10 bits of base "queue"
#define BASE_TREE_10BIT			0x254		//< First 10 bits of base "tree"

#define COMMAND_JUST_THE_KEY	0x000		//< No command in the key
#define COMMAND_CHILD_10BIT		0x103		//< First 10 bits of command "ch{ild}"
#define COMMAND_FIRST_10BIT		0x126		//< First 10 bits of command "fi{rst}"
#define COMMAND_GET_10BIT		0x0a7		//< First 10 bits of command "ge{t}"
#define COMMAND_HIGH_10BIT		0x128		//< First 10 bits of command "hi{ghest}"
#define COMMAND_II_10BIT		0x129		//< First 10 bits of command "ii"
#define COMMAND_INSERT_10BIT	0x1c9		//< First 10 bits of command "in{sert}"
#define COMMAND_IS_10BIT		0x269		//< First 10 bits of command "is"
#define COMMAND_LAST_10BIT		0x02c		//< First 10 bits of command "la{st}"
#define COMMAND_LOW_10BIT		0x1ec		//< First 10 bits of command "lo{west}"
#define COMMAND_NEXT_10BIT		0x0ae		//< First 10 bits of command "ne{xt}"
#define COMMAND_PARENT_10BIT	0x030		//< First 10 bits of command "pa{rent}"
#define COMMAND_PFIRST_10BIT	0x0d0		//< First 10 bits of command "pf{irst}"
#define COMMAND_PLAST_10BIT		0x190		//< First 10 bits of command "pl{ast}"
#define COMMAND_PREV_10BIT		0x250		//< First 10 bits of command "pr{ev}"
#define COMMAND_PUT_10BIT		0x2b0		//< First 10 bits of command "pu{t}"
#define COMMAND_SI_10BIT		0x133		//< First 10 bits of command "si"
#define COMMAND_SS_10BIT		0x273		//< First 10 bits of command "ss"
#define COMMAND_XHIGH_10BIT		0x118		//< First 10 bits of command "xh{ighest}"
#define COMMAND_XLOW_10BIT		0x198		//< First 10 bits of command "xl{owest}"
#define COMMAND_PARENT_KEY		0x3ff		//< In a put call with a key, the command whatever it is should be considered a parent key.
#define COMMAND_SIZE			0x400		//< For numbers, defining a queue size, this is added to avoid overlap.


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
	uint64_t key_hash;									///< Node locator hash required to find the ID of a related (next, ...) node.
};


/** \brief EntityKeyHash: A record containing separate hashes for entity and key.

*/
struct EntityKeyHash {
	uint64_t ent_hash, key_hash;

	bool operator==(const EntityKeyHash &o) const {
		return ent_hash == o.ent_hash && key_hash == o.key_hash;
	}

	bool operator<(const EntityKeyHash &o) const {
		return ent_hash < o.ent_hash || (ent_hash == o.ent_hash && key_hash < o.key_hash);
	}
};


/** \brief HashVolXctMap: A map from hashes to pointers to VolatileTransaction.

This map allows locating entity root VolatileTransactions for creating and destroying entities.
*/
typedef std::map<uint64_t, pVolatileTransaction> HashVolXctMap;


/** \brief EntKeyVolXctMap: A map from (entity,key) hashes to pointers to VolatileTransaction.

This map allows locating any nodes.
*/
typedef std::map<EntityKeyHash, pVolatileTransaction> EntKeyVolXctMap;


/** \brief HashNameMap: A map from hashes to pointers to VolatileTransaction.

This map allows doing the reverse conversion to a hash() function finding out the hashed names.
*/
typedef std::map<uint64_t, Name> HashNameMap;


/// A pointer to an std::string
typedef std::string* pString;


/** \brief Volatile: A Service to manage data objects in RAM.

Node Method Reference
---------------------

Some bases in Volatile, like **deque** which is just a key/value store where each entity is a new keyspace. Keys are keys and values
are blocks of any kind. Other bases have a syntax that allows operating with entities or nodes, like pop()-ing the highest priority
node in a **queue**, accessing to nodes in a tree using the tree pointers or keys to node ids, etc.

The reference for this is this docstring. All entities and keys can be removed with remove(), each time keys are returned, they are
inside a block of just one string, see the methods of each base for more. Methods have commands that follow a ~ char and are only two
letters long, allthough they can be written in full to improve readability. E.g. ~fi is the same as ~first and, we write it ~fi{rst}
below, but, of course, it cannot be written with the brackets.

Methods in deque
----------------

A deque is a key-value store. It is created empty via new_entity() and you can just get(), header(), put(), remove() or copy(). In order
to  access all the blocks in a deque, the key of the first block will be returned by get()ting //deque/entity/~fi{rst}. And the keys of
any node can be obtained by //deque/entity/key~ne{xt} and //deque/entity/key~pr{ev}. Also, //deque/entity/~la{st} returns the last element.
Aditionally, //deque/entity/~pf{irst} and //deque/entity/~pl{ast} return the corresponding nodes while removing them. For put() calls
//deque/entity/~fi{rst} and //deque/entity/~la{st} can also be given and the nodes will be created without keys.

Methods in index
----------------

Index both exposes and serializes Index type blocks. An entity inside index is **one single** Index. When get()ting them by key, you get
the value stored in the Index. To create a new one, just new_entity() //index/name/~ss (ii, is, si or ss). To populate one just
put() to //index/name/~pu{t} with a Tuple of the appropriate Kind. To save one, just get() //index/name/~ge{t}.

Methods in queue
----------------

A priority queue is implemented as self balanced binary trees. Each time you push a block, you must put() to a key with a priority by
putting to //queue/name/key~0.977 (where 0.977 can be serialized to a double). The queue is created by new_entity() //queue/name/~5000
(where 5000 is a mandatory maximum number of nodes). When the queue fills, lower priority nodes are discarded. You can also get()
to //queue/name/~xh{ighest} (extracting it), //queue/name/~hi{ghest} (leaving it), equivalently: //queue/name/~xl{owest},
//queue/name/~lo{west}. And you can get() nodes by key as in a deque (if they haven't been pop()ed or been discarded).
Aditionally, you can put() to //deque/entity/~in{sert} and the node will be inserted without a key.

Methods in tree
---------------

When a tree is created empty, new_entity() //tree/name/, the first node pushed must have just a key and is the only node without
a parent. Any node is created by put()ing to //tree/name/key~parentname (where parentname must exist). All nodes support querying keys to
their parent, siblings and first child via: get() //tree/name/key~pa{rent}, //tree/name/key~ne{xt}, //tree/name/key~ch{ild}.
The root node can be retrieved with get() //tree/entity/~first.
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

		/** Internal non-copy version of get() form 1.

			\param p_txn	A Transaction **inside the Container** that will be returned for anything except an index.
			\param p_str	A pointer to a std::string.c_str() **inside the Container** for index.
			\param pop_ent	The hash of the entity from which the returned block is expected to be pop()-ed.
			\param what		Some Locator to the block, just like what get() expects.

			\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

		This is the kernel of get(). The difference is it does not create block to be used by other services, it returns the internal
		VolatileTransaction without copying neither the transaction nor the block.
		*/
		inline StatusCode internal_get(pTransaction &p_txn, pString &p_str, uint64_t &pop_ent, Locator &what) {

			HashVolXctMap *p_ent_map;
			int base;

			pop_ent = 0;

			switch (base = TenBitsAtAddress(what.base)) {
			case BASE_DEQUE_10BIT:
				p_ent_map = &deque_ent;
				break;

			case BASE_INDEX_10BIT:
				p_ent_map = &index_ent;
				break;

			case BASE_QUEUE_10BIT:
				p_ent_map = &queue_ent;
				break;

			case BASE_TREE_10BIT:
				p_ent_map = &tree_ent;
				break;

			default:
				return SERVICE_ERROR_WRONG_BASE;
			}

			EntityKeyHash ek;
			ek.ent_hash = hash(what.entity);

			HashVolXctMap::iterator it_ent = p_ent_map->find(ek.ent_hash);

			if (it_ent == p_ent_map->end())
				return SERVICE_ERROR_BLOCK_NOT_FOUND;

			pVolatileTransaction p_root = it_ent->second;

			Name key, parent;
			int	 command;

			if (!parse_command(key, command, parent, what.key, false))
				return SERVICE_ERROR_PARSING_COMMAND;

			switch (command) {
			case COMMAND_JUST_THE_KEY:
  				switch (base) {
				case BASE_DEQUE_10BIT: {
					ek.key_hash = hash(key);
					EntKeyVolXctMap::iterator it;

					if ((it = deque_key.find(ek)) == deque_key.end())
						return SERVICE_ERROR_BLOCK_NOT_FOUND;

					p_txn = it->second;
					p_str = nullptr; }

					return SERVICE_NO_ERROR;

				case BASE_QUEUE_10BIT: {
					ek.key_hash = hash(key);
					EntKeyVolXctMap::iterator it;

					if ((it = queue_key.find(ek)) == queue_key.end())
						return SERVICE_ERROR_BLOCK_NOT_FOUND;

					p_txn = it->second;
					p_str = nullptr; }

					return SERVICE_NO_ERROR;

				case BASE_TREE_10BIT: {
					ek.key_hash = hash(key);
					EntKeyVolXctMap::iterator it;

					if ((it = tree_key.find(ek)) == tree_key.end())
						return SERVICE_ERROR_BLOCK_NOT_FOUND;

					p_txn = it->second;
					p_str = nullptr; }

					return SERVICE_NO_ERROR;

				default: {
					Index::iterator it;

					if ((it = p_root->p_hea->index.find(key)) == p_root->p_hea->index.end())
						return SERVICE_ERROR_BLOCK_NOT_FOUND;

					p_txn = nullptr;
					p_str = &it->second; }

					return SERVICE_NO_ERROR;
				}

			case COMMAND_CHILD_10BIT: {
				if (base != BASE_TREE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				ek.key_hash = hash(key);
				EntKeyVolXctMap::iterator it;

				if ((it = tree_key.find(ek)) == tree_key.end())
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				p_txn = it->second->p_child;
				p_str = nullptr; }

				return SERVICE_NO_ERROR;

			case COMMAND_PARENT_10BIT: {
				if (base != BASE_TREE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				ek.key_hash = hash(key);
				EntKeyVolXctMap::iterator it;

				if ((it = tree_key.find(ek)) == tree_key.end())
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				p_txn = it->second->p_parent;
				p_str = nullptr; }

				return SERVICE_NO_ERROR;

			case COMMAND_NEXT_10BIT: {
				if (base != BASE_TREE_10BIT && base != BASE_DEQUE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				ek.key_hash = hash(key);
				EntKeyVolXctMap::iterator it;

				if ((it = tree_key.find(ek)) == tree_key.end())
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				p_txn = it->second->p_next;
				p_str = nullptr; }

				return SERVICE_NO_ERROR;

			case COMMAND_PREV_10BIT: {
				if (base != BASE_DEQUE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				ek.key_hash = hash(key);
				EntKeyVolXctMap::iterator it;

				if ((it = tree_key.find(ek)) == tree_key.end())
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				p_txn = it->second->p_prev;
				p_str = nullptr; }

				return SERVICE_NO_ERROR;

			case COMMAND_HIGH_10BIT:
			case COMMAND_XHIGH_10BIT: {
				if (base != BASE_QUEUE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (p_root == nullptr)
					return SERVICE_ERROR_EMPTY_ENTITY;

				p_txn = highest_priority(p_root);
				p_str = nullptr;

				if (command == COMMAND_XHIGH_10BIT)
					pop_ent = ek.ent_hash; }

				return SERVICE_NO_ERROR;

			case COMMAND_LOW_10BIT:
			case COMMAND_XLOW_10BIT: {
				if (base != BASE_QUEUE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (p_root == nullptr)
					return SERVICE_ERROR_EMPTY_ENTITY;

				p_txn = lowest_priority(p_root);
				p_str = nullptr;

				if (command == COMMAND_XLOW_10BIT)
					pop_ent = ek.ent_hash; }

				return SERVICE_NO_ERROR;

			case COMMAND_FIRST_10BIT:
			case COMMAND_PFIRST_10BIT: {
				if (base != BASE_DEQUE_10BIT && (base != BASE_TREE_10BIT || command == COMMAND_PFIRST_10BIT))
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (p_root == nullptr)
					return SERVICE_ERROR_EMPTY_ENTITY;

				p_txn = p_root;
				p_str = nullptr;

				if (command == COMMAND_PFIRST_10BIT)
					pop_ent = ek.ent_hash; }

				return SERVICE_NO_ERROR;

			case COMMAND_LAST_10BIT:
			case COMMAND_PLAST_10BIT: {
				if (base != BASE_DEQUE_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (p_root == nullptr)
					return SERVICE_ERROR_EMPTY_ENTITY;

				p_txn = p_root->p_prev;
				p_str = nullptr;

				if (command == COMMAND_PLAST_10BIT)
					pop_ent = ek.ent_hash; }

				return SERVICE_NO_ERROR;

			case COMMAND_GET_10BIT:
				if (base != BASE_INDEX_10BIT)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				p_txn = nullptr;
				p_str = nullptr;
				pop_ent = ek.ent_hash;

				return SERVICE_NO_ERROR;
			}

			return SERVICE_ERROR_PARSING_COMMAND;
		}

		/** Parses a key to find the command and a new key that can be hashed, possibly a key of a parent.

			\param key_out The clean key returned without the command.
			\param command The command as an integer in COMMAND_CHILD_10BIT..COMMAND_XLOW_10BIT or COMMAND_SIZE + a size
			\param parent  A parent node, when command == COMMAND_PARENT_KEY (In put //tree/ent/aaa~next, "next" is a parent id,
						   not a command.)
			\param key_in  The original key to be parsed.
			\param is_put  We are parsing a put call (if true, the command maybe a parent node id).

			\return		   True on success, all outputs (key_out, command, parent) are defined on success and undefined on failure.

		NOTE: See the reference of the class Volatile for an explanation on commands.
		*/
		inline bool parse_command(Name &key_out, int &command, Name &parent, Name key_in, bool is_put) {
			if (key_in[0] == '~') {
				key_out[0] = 0;
				parent [0] = 0;

				switch (command = TenBitsAtAddress(&key_in[1])) {
				case COMMAND_FIRST_10BIT:
				case COMMAND_LAST_10BIT:
					return true;

				case COMMAND_PUT_10BIT:
				case COMMAND_INSERT_10BIT:
					return is_put;

				case COMMAND_II_10BIT:
				case COMMAND_IS_10BIT:
				case COMMAND_SI_10BIT:
				case COMMAND_SS_10BIT:
				case COMMAND_PFIRST_10BIT:
				case COMMAND_PLAST_10BIT:
				case COMMAND_HIGH_10BIT:
				case COMMAND_LOW_10BIT:
				case COMMAND_GET_10BIT:
				case COMMAND_XHIGH_10BIT:
				case COMMAND_XLOW_10BIT:
					return !is_put;

				default:
					if (!is_put || key_in[1] < '0' || key_in[1] > '9')
						return false;

					int size, r_len;

					if (sscanf(&key_in[1], "%d%n", &size, &r_len) != 1 || size <= 0 || key_in[r_len + 1] != 0)
						return false;

					command = COMMAND_SIZE + size;

					return true;
				}
			}
			strcpy(key_out, key_in);
			pChar pc = strchr(key_out, '~');

			if (pc == nullptr) {
				command	  = COMMAND_JUST_THE_KEY;
				parent[0] = 0;

				return true;
			}
			(*pc++) = 0;

			if (*pc == 0)
				return false;

			if (is_put) {
				if (!valid_name(pc))
					return false;

				strcpy(parent, pc);
				command = COMMAND_PARENT_KEY;

				return true;
			}
			parent[0] = 0;

			switch (command = TenBitsAtAddress(pc)) {
			case COMMAND_CHILD_10BIT:
			case COMMAND_NEXT_10BIT:
			case COMMAND_PARENT_10BIT:
			case COMMAND_PREV_10BIT:
				return true;
			}

			return false;
		}

		/** Fills a name with zero after the string and returns the hash of the complete NAME_SIZE-long array.

			\param name The name to be filled and hashed.

			\return		The hash of the whole array.
		*/
		inline uint64_t hash(Name &name) {

			bool fill = false;

			for (int i = 0; i < NAME_SIZE; i++) {
				if (name[i] != 0) {
					if (fill)
						name[i] = 0;
				} else
					fill = true;
			}
			return MurmurHash64A(&name, NAME_SIZE);
		}


		/** Check the relative position (left or right) between an item and a tree for inserting a deleting.

			\param p_item The item that will be inserted or deleted.
			\param p_tree The AA subtree compared with the item,.

			Note: This should be the only way to break ties when p_item->priority == p_tree->priority.
		*/
		inline bool to_left(pVolatileTransaction p_item, pVolatileTransaction p_tree) {

			return p_item->priority == p_tree->priority ? (uintptr_t) p_item < (uintptr_t) p_tree : p_item->priority < p_tree->priority;
		}


		/** Free all the blocks in the sub-tree calling destroy_transaction() recursively.

			\param p_txn The root of the AA subtree from which we the free all the blocks.

			Note: This does not dealloc the tree and should only be used inside destroy_keeprs(). It is NOT thread safe.
		*/
		inline void recursive_destroy_queue(pVolatileTransaction &p_txn) {

			if (p_txn != nullptr) {
				recursive_destroy_queue(p_txn->p_prev);
				recursive_destroy_queue(p_txn->p_next);

				pTransaction p_txn2 = p_txn;	// Ugly, this should be: destroy_transaction(p_txn)
				destroy_transaction(p_txn2);
				p_txn = nullptr;
			}
		};


		/** Return the highest priority node in the AA subtree without modifying the tree

			\param p_item The root of the AA subtree from which we want the highest priority node.

			\return		  The highest priority node in the AA subtree.

			Note: This does not alter the tree and is thread safe with other reading threads, but incompatible with writing threads.
		*/
		inline pVolatileTransaction highest_priority(pVolatileTransaction p_item) {

			if (p_item != nullptr) {
				while (p_item->p_next != nullptr)
					p_item = p_item->p_next;
			};

			return p_item;
		};


		/** Return the lowest priority node in the AA subtree without modifying the tree

			\param p_item The root of the AA subtree from which we want the lowest priority node.

			\return		  The lowest priority node in the AA subtree.

			Note: This does not alter the tree and is thread safe with other reading threads, but incompatible with writing threads.
		*/
		inline pVolatileTransaction lowest_priority(pVolatileTransaction p_item) {

			if (p_item != nullptr) {
				while (p_item->p_prev != nullptr)
					p_item = p_item->p_prev;
			};

			return p_item;
		};


		/** Rebalance the tree after remove().

			\param	p_tree The tree resulting of (a recursive step) in remove

			\return The balanced tree

		 Decrease the level of all nodes in this level if necessary, and then skew and split all nodes in the new level.
		*/
		inline pVolatileTransaction rebalance(pVolatileTransaction p_tree) {

			decrease_level(p_tree);

			p_tree = skew(p_tree);
			if (p_tree->p_next != nullptr) {
				p_tree->p_next = skew(p_tree->p_next);

				if (p_tree->p_next->p_next != nullptr)
					p_tree->p_next->p_next = skew(p_tree->p_next->p_next);
			};
			p_tree = split(p_tree);

			if (p_tree->p_next != nullptr)
				p_tree->p_next = split(p_tree->p_next);

			return p_tree;
		}


		/** Check if a node belongs to a tree

			\param p_item The node checked if part of the AA subtree
			\param p_tree The the AA subtree that may contain p_item

			\return		  True if the node was found in the tree
		*/
		inline bool is_in_tree(pVolatileTransaction p_item, pVolatileTransaction p_tree) {

			if (p_tree == nullptr || p_item == nullptr)
				return false;

			if (p_item == p_tree)
				return true;

			if (to_left(p_item, p_tree))
				return is_in_tree(p_item, p_tree->p_prev);

			return is_in_tree(p_item, p_tree->p_next);
		};


		/** Implements the "deep case" of AA tree removal.

			\param p_kill	The node that we are following towards HPLoT found by recursion
			\param p_parent The parent of p_kill (required to rebalance the tree after every recursive step)
			\param p_tree	The (never changing) root of the subtree (we want to remove it)
			\param p_deep	A variable to store the HPLoT (found in the deepest level, applied in the shallowest)

			\return			The rebalanced HPLoT converted in the new subtree root

			Note: Against what is stated in AA tree literature, it is not always easy to convert an arbitrary node that has to be
			removed into a leaf. (Many applications only remove high or low priority node and that does not apply then.) When we need
			to remove a node high in the tree (far away for leaves) skewing the tree to make its predecessor the new root is feasible,
			but that still does not solve the problem of removing it. The safest option is removing the HPLoT (Highest Priority to the
			Left of Tree) which at least will have no successor, rebalancing the tree after removal and inserting it back as the root
			in replacement of the previous root. That is what is function does.
		*/
		inline pVolatileTransaction remove_go_deep(pVolatileTransaction p_kill,
												   pVolatileTransaction p_parent,
												   pVolatileTransaction p_tree,
												   pVolatileTransaction &p_deep) {
			if (p_kill->p_next != nullptr)
				p_kill->p_next = remove_go_deep(p_kill->p_next, p_kill, p_tree, p_deep);

			else {
				if (p_parent == p_tree) {
					p_tree->p_prev = p_kill->p_prev;	// Disconnect p_kill
					p_kill->level		 = p_tree->level;
					p_kill->p_next = p_tree->p_next;
					p_kill->p_prev = p_tree->p_prev;	// p_kill is the new p_tree

					return rebalance(p_kill);

				} else {
					p_deep = p_kill;					// Save p_kill for the end

					return p_kill->p_prev;				// Disconnect p_kill
				}
			}
			if (p_parent != p_tree)
				return rebalance(p_kill);

			else {
				p_deep->level		 = p_tree->level;
				p_deep->p_next = p_tree->p_next;
				p_deep->p_prev = rebalance(p_kill);

				return rebalance(p_deep);
			}
		}


		/** Remove a node in an AA subtree

			\param p_item The node to be removed
			\param p_tree The tree from which it should be removed

			\return		  The balanced p_tree without the node p_item

			Note: This is NOT thread safe and should only be used inside public methods providing the safety mechanisms.
		*/
		inline pVolatileTransaction remove(pVolatileTransaction p_item, pVolatileTransaction p_tree) {

			if (p_tree == nullptr || p_item == nullptr)
				return p_tree;

			if (p_item == p_tree) {
				if (p_tree->p_prev == nullptr)
					return p_tree->p_next;

				else {
					if (p_tree->p_next == nullptr)
						return p_tree->p_prev;

					else {
						pVolatileTransaction p_deep = nullptr;
						return remove_go_deep(p_tree->p_prev, p_tree, p_tree, p_deep);
					}
				}
			} else {
				if (to_left(p_item, p_tree))
					p_tree->p_prev = remove(p_item, p_tree->p_prev);

				else
					p_tree->p_next = remove(p_item, p_tree->p_next);
			}

			return rebalance(p_tree);
		};


		/** Remove links that skip level in an AA subtree

			\param p_item A tree for which we want to remove links that skip levels.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline void decrease_level(pVolatileTransaction p_item) {

			if (p_item->p_prev == nullptr) {
				if (p_item->p_next == nullptr)
					p_item->level = 1;

				else {
					int should_be = p_item->p_next->level + 1;

					if (should_be < p_item->level) {
						p_item->level = should_be;

						if (should_be < p_item->p_next->level)
							p_item->p_next->level = should_be;
					}
				}
			} else {
				if (p_item->p_next == nullptr) {
					int should_be = p_item->p_prev->level + 1;

					if (should_be < p_item->level)
						p_item->level = should_be;

				} else {
					int should_be = std::min(p_item->p_prev->level, p_item->p_next->level) + 1;

					if (should_be < p_item->level) {
						p_item->level = should_be;

						if (should_be < p_item->p_next->level)
							p_item->p_next->level = should_be;
					}
				}
			}
		};


		/** Try to rebalance an AA subtree on its left (prev) side

			\param p_item a node representing an AA tree that needs to be rebalanced.
			\return		  Another node representing the rebalanced AA tree.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline pVolatileTransaction skew(pVolatileTransaction p_item) {
			// rotate p_next if p_prev child has same level

			if (p_item->p_prev != nullptr && p_item->level == p_item->p_prev->level) {
				pVolatileTransaction p_left = p_item->p_prev;

				p_item->p_prev = p_left->p_next;
				p_left->p_next = p_item;

				return p_left;
			}

			return p_item;
		};


		/** Try to rebalance an AA subtree on its right (next) side

			\param p_item A node representing an AA tree that needs to be rebalanced.
			\return		  Another node representing the rebalanced AA tree.

			Note: This does alter the tree and requires exclusive access to the AA.
		*/
		inline pVolatileTransaction split(pVolatileTransaction p_item) {
			// rotate p_prev if there are two p_next children on same level

			if (   p_item->p_next != nullptr
				&& p_item->p_next->p_next != nullptr
				&& p_item->level == p_item->p_next->p_next->level) {

				pVolatileTransaction p_right = p_item->p_next;

				p_item->p_next  = p_right->p_prev;
				p_right->p_prev = p_item;
				if (p_item->p_prev == nullptr && p_item->p_next == nullptr && p_item->level > 1) {
					p_item->level = 1;
/* The following condition looks like it could be problematic (since the previous one is already an undocumented deviation from the
canonical method description). It has never been observed (in intense unit testing) and should only be considered if problems happen.
It may very well be impossible, who knows. Just keep it as a remark, unless someone smarter proves it unnecessary. */
					// if (   p_right->level != 2
					// 	|| (   p_right->p_next->p_next != nullptr
					// 		&& p_right->p_next->p_next->level >= 2))
					// 		p_right->level++;	// This is actually a breakpoint possibility, not a solution!!

				} else
					p_right->level++;

				return p_right;
			}

			return p_item;
		};


		/** Insert a node in its correct place according to priority in an AA subtree

			\param p_new  The node to be inserted
			\param p_tree The root of the subtree where p_new will be inserted
			\return		  A balanced version of p_tree including p_new

			Note: This is NOT thread safe and should only be used inside public methods providing the safety mechanisms.
		*/
		inline pVolatileTransaction insert(pVolatileTransaction p_new, pVolatileTransaction p_tree) {
			// Do the normal binary tree insertion procedure. Set the result of the
			// recursive call to the correct child in case a new node was created or the
			// root of the subtree changes.

			if (p_tree == nullptr) {
				p_new->level = 1;
				p_new->p_prev = nullptr;
				p_new->p_next = nullptr;

				return p_new;

			} else {

				if (to_left(p_new, p_tree))
					p_tree->p_prev = insert(p_new, p_tree->p_prev);

				else
					p_tree->p_next = insert(p_new, p_tree->p_next);
			}

			// Perform skew and then split.	The conditionals that determine whether or
			// not a rotation will occur or not are inside of the procedures, as given above.

			p_tree = skew(p_tree);
			p_tree = split(p_tree);

			return p_tree;
		};

	HashNameMap		name {};
	HashVolXctMap	deque_ent {}, queue_ent {}, tree_ent {}, index_ent {};
	EntKeyVolXctMap deque_key {}, queue_key {}, tree_key {};
};
typedef Volatile *pVolatile;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_VOLATILE

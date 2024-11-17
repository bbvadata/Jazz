/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_bebop/base_api.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_SPACE
#define INCLUDED_JAZZ_BEBOP_SPACE


namespace jazz_bebop
{

typedef uint64_t RowNumber;		///< A row number in a Space
typedef Name	 ColumnName;	///< A column name in a Space

#define SPACE_NOT_A_ROW				0xffffFFFFffffFFFE	///< A row number that is not valid used for initialization.
#define SPACE_ROW_STOP_ITERATOR		0xffffFFFFffffFFFF	///< A row number returned by the iterator when it is done.


class Space;					///< Forward definition of Space
typedef Space *pSpace;			///< A pointer to a Space


/** \brief RowSelection: An iterator over the rows of a Space.

	This is an abstract class that provides the interface of an iterator over rows. The where() method of a Space returns a RowSelection
	descendant that is compatible with this interface.

	\see Space

*/
class RowSelection {

	/** \brief The constructor for a RowSelection.

		\param query	A query string that is understood by the descendant. In Bop, this is the content of a WHERE clause with a syntax
					    that depends on how the Space is indexed (Time, categorical, Embedding storage, ...).
		\param p_space	The Space that created the object and is being queried.
	*/
	RowSelection(pChar query, pSpace p_space) {}

	/** \brief Restart the iterator.

		\return True if the iterator was successfully restarted, false if it is not possible.
	*/
	virtual bool restart() {
		return false;
	}

	/** \brief Get the next row.

		\return The next row number or SPACE_ROW_STOP_ITERATOR if the iteration is exhausted.
	*/
	virtual RowNumber next() {
		return SPACE_ROW_STOP_ITERATOR;
	}
};
typedef RowSelection *pRowSelection;	///< A pointer to a RowSelection


/** \brief ColSelection: A selection of columns from a Space.

	The interface is consistent with RowSelection, but unlike RowSelection the parent class is probably all you need.

	\see Space

*/
class ColSelection {

	/** \brief The constructor for a ColSelection.

		\param query	By default, a list of comma separated column names. A descendant may define a different interface.
						In Bop, this is the content of a SELECT clause.
		\param p_space	This is not used in the parent class, but provided to a hypothetical descendant. (May be removed in the future.)
	*/
	ColSelection(pChar query, pSpace p_space);

	virtual bool restart();

	virtual pName next();
};
typedef ColSelection *pColSelection;	///< A pointer to a ColSelection


/** \brief Space: The abstract space.

	This is the abstract parent of DataSpace and SemSpace. A Space is an abstraction over many blocks that provides:

	- An abstraction in the form of rows and columns.
	- Sharding and replication across a cluster.
	- A mechanism to load, update, invalidate blocks. This supports continuous update like in time series.
	- A mechanism to load and update its own metadata in a persisted way.

	The class Space is mostly empty. It provides the parent virtual interface and the parents of all the auxiliary classes used
	to access data.

	\see DataSpace, SemSpace

*/
class Space : public Container {

	public:

	// Container interface

		Space(pBaseAPI api, pName name);
	   ~Space();

		StatusCode start	();
		StatusCode shut_down();

	// Persistence interface

//TODO: Define how Space stores its metadata.

	// Internal interface for ColSelection to have access to data by rows.

//TODO: Define internal interface for ColSelection to have access to data by rows.


	// Space interface

	/** \brief Get a RowSelection from a query.

		\param query	A query string that is understood by the descendant. In Bop, this is the content of a WHERE clause with a syntax
						that depends on how the Space is indexed (Time, categorical, Embedding storage, ...).

		\return A RowSelection object that can be used to iterate over the rows that match the query.
	*/
	virtual pRowSelection where(pChar query) {
		return nullptr;
	}

	/** \brief Get a ColSelection from a query.

		\param query By default, a list of comma separated column names. Spaces that use descendants of ColSelection may define a
					 different interface.
		\return A ColSelection object that can be used to iterate over the selected columns.
	*/
	virtual pColSelection select(pChar query) {
		return nullptr;
	}

//TODO: Complete the Space interface.

	protected:

		pBaseAPI p_api;		///< A pointer to the BaseAPI that provides access to containers.
};

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_SPACE

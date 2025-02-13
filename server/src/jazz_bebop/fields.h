/* Jazz (c) 2018-2025 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_bebop/snippet.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_FIELD
#define INCLUDED_JAZZ_BEBOP_FIELD


namespace jazz_bebop
{

/** \brief Fields: A Space for Snippets.

A Fields is Space interface to the base `index` of the Volatile Container. The entity is the Fields and the key is the name of the snippet.

A field is a namespace inside Fields.
*/
class Fields : public Space {

	public:

		Fields(pBaseAPI api, pName name);

		virtual StatusCode start();

		virtual pChar const id();

		// Space interface

		virtual StatusCode load_meta();
		virtual StatusCode save_meta();
		virtual RowNumber num_rows();
		virtual void* get_index_data(RowNumber row);
		virtual int num_cols();
		virtual pName col_name(int col);
		virtual int col_index(pName name);
		virtual pLocator locator(RowNumber row, int col, int &index);
		virtual pRowSelection where(pChar query);
		virtual StatusCode get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols = nullptr, pCaster cast = nullptr);

	private:

		StatusCode load_or_create_space();

		Name		storage_ent;	///< The name of the storage entity (Typically an lmdb database with the metadata of all SemSpaces).
		pVolatile	p_volatile;		///< The Volatile container
		pPersisted	p_persisted;	///< The Persisted container
};
typedef Fields *pFields;			///< A pointer to a Fields

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_FIELD

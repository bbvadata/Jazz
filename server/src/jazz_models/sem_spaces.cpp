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


#include "src/jazz_models/sem_spaces.h"


namespace jazz_models
{

/*	-----------------------------------------------
	 SemSpaces : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the SemSpaces.

	\param api	 A pointer to a BaseAPI that provides access to containers.
*/
SemSpaces::SemSpaces(pBaseAPI api) : Fields(api) {}


/** Starts the SemSpaces service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode SemSpaces::start() {

	int ret = Fields::start();

	if (ret != SERVICE_NO_ERROR)
		return ret;

	std::string s;

	if (!get_conf_key("SEMSPACE_STORAGE_ENTITY", s)) {
		log(LOG_ERROR, "Config key SEMSPACE_STORAGE_ENTITY not found in SemSpaces::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
		log(LOG_ERROR, "Config key SEMSPACE_STORAGE_ENTITY is not a valid base in SemSpaces::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	strcpy(storage_ent, s.c_str());

	return load_or_create_space();
}


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const SemSpaces::id() {
    static char arr[] = "SemSpaces from Jazz-" JAZZ_VERSION;
    return arr;
}


StatusCode SemSpaces::load_meta() {

//TODO: Implement SemSpaces::load_meta

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode SemSpaces::save_meta() {

//TODO: Implement SemSpaces::save_meta

	return SERVICE_NOT_IMPLEMENTED;
}


RowNumber SemSpaces::num_rows() {

//TODO: Implement SemSpaces::num_rows

	return SPACE_NOT_A_ROW;
}


void* SemSpaces::get_index_data(RowNumber row) {

//TODO: Implement SemSpaces::get_index_data

	return nullptr;
}


int SemSpaces::num_cols() {

//TODO: Implement SemSpaces::num_cols

	return 0;
}


pName SemSpaces::col_name(int col) {

//TODO: Implement SemSpaces::col_name

	return nullptr;
}


int SemSpaces::col_index(pName name) {

//TODO: Implement SemSpaces::col_index

	return -1;
}


pLocator SemSpaces::locator(RowNumber row, int col, int &index) {

//TODO: Implement SemSpaces::locator

	return nullptr;
}


pRowSelection SemSpaces::where(pChar query) {

//TODO: Implement SemSpaces::where

	return nullptr;
}


StatusCode SemSpaces::get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols, pCaster cast) {

//TODO: Implement SemSpaces::get_row

	return SERVICE_NOT_IMPLEMENTED;
}


/** Load or create the space.

	This is called once when start() has successfully completed.
	It will load from persistence (and not write into persistence) if def.load_on_start.
	Otherwise, if the table metadata exists, it will fail. To override a table, you must use the SemSpaces-ETL interface.-
	Otherwise, it will create the table using the definition in `def`.

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode SemSpaces::load_or_create_space() {

//TODO: Implement SemSpaces::load_or_create_space

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_models

#if defined CATCH_TEST
#include "src/jazz_models/tests/test_sem_spaces.ctest"
#endif

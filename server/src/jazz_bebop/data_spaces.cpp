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


#include "src/jazz_bebop/data_spaces.h"


namespace jazz_bebop
{

/*	-----------------------------------------------
	 DataSpaces : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the DataSpaces.

	\param api	 A pointer to a BaseAPI that provides access to containers.
*/
DataSpaces::DataSpaces(pBaseAPI api) : Service(api->p_log, api->p_conf) {

	p_api = api;
}


/** Starts the DataSpaces service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode DataSpaces::start() {

	std::string s;

	if (!get_conf_key("DATASPACES_STORAGE_ENTITY", s)) {
		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY not found in DataSpaces::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY is not a valid base in DataSpaces::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	strcpy(storage_ent, s.c_str());

	return load_or_create_space();
}


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const DataSpaces::id() {
    static char arr[] = "DataSpaces from Jazz-" JAZZ_VERSION;
    return arr;
}


StatusCode DataSpaces::load_meta() {

//TODO: Implement DataSpaces::load_meta

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode DataSpaces::save_meta() {

//TODO: Implement DataSpaces::save_meta

	return SERVICE_NOT_IMPLEMENTED;
}


RowNumber DataSpaces::num_rows() {

//TODO: Implement DataSpaces::num_rows

	return SPACE_NOT_A_ROW;
}


void* DataSpaces::get_index_data(RowNumber row) {

//TODO: Implement DataSpaces::get_index_data

	return nullptr;
}


int DataSpaces::num_cols() {

//TODO: Implement DataSpaces::num_cols

	return 0;
}


pName DataSpaces::col_name(int col) {

//TODO: Implement DataSpaces::col_name

	return nullptr;
}


int DataSpaces::col_index(pName name) {

//TODO: Implement DataSpaces::col_index

	return -1;
}


pLocator DataSpaces::locator(RowNumber row, int col, int &index) {

//TODO: Implement DataSpaces::locator

	return nullptr;
}


pRowSelection DataSpaces::where(pChar query) {

//TODO: Implement DataSpaces::where

	return nullptr;
}


StatusCode DataSpaces::get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols, pCaster cast) {

//TODO: Implement DataSpaces::get_row

	return SERVICE_NOT_IMPLEMENTED;
}


/** Load or create the space.

	This is called once when start() has successfully completed.
	It will load from persistence (and not write into persistence) if def.load_on_start.
	Otherwise, if the table metadata exists, it will fail. To override a table, you must use the DataSpaces-ETL interface.-
	Otherwise, it will create the table using the definition in `def`.

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode DataSpaces::load_or_create_space() {

//TODO: Implement DataSpaces::load_or_create_space

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_data_spaces.ctest"
#endif

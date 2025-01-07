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


#include "src/jazz_models/sem_space.h"


namespace jazz_models
{

/*	-----------------------------------------------
	 SemSpace : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the SemSpace.

	\param api	 A pointer to a BaseAPI that provides access to containers.
	\param name	 The name of the SemSpace.
	\param p_def The definition of the SemSpace. The content is copied on construction.
*/
SemSpace::SemSpace(pBaseAPI api, pName name, pSemSpaceDefinition p_def) : Space(api, name) {

	def.load_on_start = true;		// Forces loading when p_def is nullptr.

	if (p_def != nullptr)
		def = *p_def;
}


/** Starts the SemSpace service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode SemSpace::start() {

	int ret = Space::start();

	if (ret != SERVICE_NO_ERROR)
		return ret;

	std::string s;

	if (!get_conf_key("SEMSPACE_STORAGE_ENTITY", s)) {
		log(LOG_ERROR, "Config key SEMSPACE_STORAGE_ENTITY not found in SemSpace::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
		log(LOG_ERROR, "Config key SEMSPACE_STORAGE_ENTITY is not a valid base in SemSpace::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	strcpy(storage_ent, s.c_str());

	return load_or_create_space();
}


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const SemSpace::id() {
    static char arr[] = "SemSpace from Jazz-" JAZZ_VERSION;
    return arr;
}


StatusCode SemSpace::load_meta() {

//TODO: Implement SemSpace::load_meta

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode SemSpace::save_meta() {

//TODO: Implement SemSpace::save_meta

	return SERVICE_NOT_IMPLEMENTED;
}


RowNumber SemSpace::num_rows() {

//TODO: Implement SemSpace::num_rows

	return SPACE_NOT_A_ROW;
}


void* SemSpace::get_index_data(RowNumber row) {

//TODO: Implement SemSpace::get_index_data

	return nullptr;
}


int SemSpace::num_cols() {

//TODO: Implement SemSpace::num_cols

	return 0;
}


pName SemSpace::col_name(int col) {

//TODO: Implement SemSpace::col_name

	return nullptr;
}


int SemSpace::col_index(pName name) {

//TODO: Implement SemSpace::col_index

	return -1;
}


pLocator SemSpace::locator(RowNumber row, int col, int &index) {

//TODO: Implement SemSpace::locator

	return nullptr;
}


pRowSelection SemSpace::where(pChar query) {

//TODO: Implement SemSpace::where

	return nullptr;
}


StatusCode SemSpace::get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols, pCaster cast) {

//TODO: Implement SemSpace::get_row

	return SERVICE_NOT_IMPLEMENTED;
}


/** Load or create the space.

	This is called once when start() has successfully completed.
	It will load from persistence (and not write into persistence) if def.load_on_start.
	Otherwise, if the table metadata exists, it will fail. To override a table, you must use the SemSpace-ETL interface.-
	Otherwise, it will create the table using the definition in `def`.

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode SemSpace::load_or_create_space() {

//TODO: Implement SemSpace::load_or_create_space

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_models

#if defined CATCH_TEST
#include "src/jazz_models/tests/test_sem_space.ctest"
#endif

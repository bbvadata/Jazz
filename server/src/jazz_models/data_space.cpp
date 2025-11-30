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


#include "src/jazz_models/data_space.h"


namespace jazz_models
{

// TODO: Take whatever is still valid from this old version.

// ///> The name of the DataSpaces class required by Spaces.
// Name NAME_CLASS_DATASPACES = "DataSpaces";

// /*	-----------------------------------------------
// 	 DataSpaces : I m p l e m e n t a t i o n
// --------------------------------------------------- */

// /** \brief Bop: Start the DataSpaces.

// 	\param api	 A pointer to a BaseAPI that provides access to containers.
// */
// DataSpaces::DataSpaces(pBaseAPI api) : Space(api, &NAME_CLASS_DATASPACES) {

// 	p_api = api;
// }


// /** Starts the DataSpaces service

// 	\return SERVICE_NO_ERROR if successful, an error code otherwise.
// */
// StatusCode DataSpaces::start() {

// 	std::string s;

// 	if (!get_conf_key("DATASPACES_STORAGE_ENTITY", s)) {
// 		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY not found in DataSpaces::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
// 		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY is not a valid base in DataSpaces::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	strcpy(storage_ent, s.c_str());

// 	return true;
// }


// /** Return object ID.

// 	\return A string identifying the object that is especially useful to track uplifts and versions.
// */
// pChar const DataSpaces::id() {
//     static char arr[] = "DataSpaces from Jazz-" JAZZ_VERSION;
//     return arr;
// }

} // namespace jazz_models

#if defined CATCH_TEST
#include "src/jazz_models/tests/test_data_space.ctest"
#endif

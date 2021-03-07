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


#include "src/jazz_elements/container.h"


namespace jazz_elements
{

/*	-----------------------------------------------
	 Container : I m p l e m e n t a t i o n
--------------------------------------------------- */

Container::Container(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {

	num_keepers = max_num_keepers = 0;
	alloc_bytes = last_alloc_bytes = warn_alloc_bytes = fail_alloc_bytes = 0;
	p_buffer = p_left = p_right = nullptr;
}

Container::~Container () {
	destroy_container();
}

/** Verifies variables in config and sets private variables accordingly.
*/
StatusCode Container::start()
{
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
StatusCode Container::shut_down(bool restarting_service)
{
	return destroy_container();
}


/** Bla, bla, bla

//TODO: Document sleep()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::sleep (pBlockKeeper *p_keeper)
{
//TODO: Implement sleep()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document callback()

	\param aaa		Bla, bla

	\return	Bla
*/
void Container::callback (BlockId64	 block_id,
						  StatusCode result)
{
//TODO: Implement callback()

	return;
}


/** Bla, bla, bla

//TODO: Document new_container()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::new_container()
{
//TODO: Implement new_container()

	return SERVICE_NOT_IMPLEMENTED;
}


/** Bla, bla, bla

//TODO: Document destroy_container()

	\param aaa		Bla, bla

	\return	Bla
*/
StatusCode Container::destroy_container()
{
//TODO: Implement destroy_container()

	return SERVICE_NOT_IMPLEMENTED;
}

} // namespace jazz_elements

#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_container.ctest"
#endif

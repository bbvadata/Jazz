/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

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

#include "src/jazz_functional/jazz_filesystem.h"


#ifndef INCLUDED_JAZZ_FUNCTIONAL_COLUMN
#define INCLUDED_JAZZ_FUNCTIONAL_COLUMN


/**< \brief TODO

//TODO: Write module description for jazz_column when implemented.
*/
namespace jazz_column
{

using namespace jazz_containers;


class DenseMapping {

	public:
		 DenseMapping();
		~DenseMapping();

//TODO: Define DenseMapping
//TODO: Implement DenseMapping
//TODO: Document DenseMapping
//TODO: Test DenseMapping

};


class SparseMapping: public DenseMapping {

	public:
		 SparseMapping();
		~SparseMapping();

//TODO: Define SparseMapping
//TODO: Implement SparseMapping
//TODO: Document SparseMapping
//TODO: Test SparseMapping

};


/**
//TODO: Write the JazzColumn description
*/
class JazzColumn: public JazzTree {

	public:
		 JazzColumn(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzColumn();

//TODO: Document interface for module jazz_column.
//TODO: Implement interface for module jazz_column.

};

}

#endif

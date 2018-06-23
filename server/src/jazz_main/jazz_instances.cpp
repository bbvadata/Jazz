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

#include "src/jazz_main/jazz_instances.h"

namespace jazz_instances
{

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

JazzConfigFile J_CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
JazzLogger	   J_LOGGER(J_CONFIG, "LOGGER_PATH");
// JazzCluster	   J_CLUSTER;
// Bebop		   J_BOP;
// JazzHttpServer J_HTTP_SERVER;
// rAPI		   J_R_API;
// pyAPI		   J_PYTHON_API;

} // namespace jazz_instances

#endif

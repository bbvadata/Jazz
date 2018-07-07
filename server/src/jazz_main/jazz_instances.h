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

#include "src/jazz_main/jazz_restapi.h"


#ifndef INCLUDED_JAZZ_MAIN_INSTANCES
#define INCLUDED_JAZZ_MAIN_INSTANCES

/**< \brief Contains one instance of each major class implemented as a global variable

The full list of Jazz global variables is.
  - J_CONFIG a JazzConfigFile
  - J_LOGGER a JazzLogger
  - J_CLUSTER a JazzCluster
  - J_BOP a Bebop
  - J_HTTP_SERVER a JazzHttpServer
  - J_R_API a rAPI
  - J_PYTHON_API a pyAPI

It also contains a signal handler function for SIGTERM and a pointer to a MHD_Daemon controlling the http server daemon.
*/
namespace jazz_instances
{

using namespace jazz_utils;
using namespace jazz_cluster;
using namespace jazz_bebop;
using namespace jazz_restapi;

#define JAZZ_DEFAULT_CONFIG_PATH "config/jazz_config.ini"

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

extern JazzConfigFile J_CONFIG;
extern JazzLogger	  J_LOGGER;
extern JazzCluster	  J_CLUSTER;
extern Bebop		  J_BOP;
extern JazzHttpServer J_HTTP_SERVER;
extern rAPI			  J_R_API;
extern pyAPI		  J_PYTHON_API;

extern pMHD_Daemon    Jazz_MHD_Daemon;

void signalHandler_SIGTERM(int signum);

}

#endif

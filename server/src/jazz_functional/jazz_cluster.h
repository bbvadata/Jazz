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

#include "src/include/jazz.h"


#ifndef INCLUDED_JAZZ_FUNCTIONAL_CLUSTER
#define INCLUDED_JAZZ_FUNCTIONAL_CLUSTER


/**< \brief TODO

//TODO: Write module description for jazz_cluster when implemented.
*/
namespace jazz_cluster
{

#define MAX_ALIAS_NAMELENGTH	  32	///< Max length of a node alias
#define MAX_HOST_NAMELENGTH		  32	///< Max length of a host name or IP
#define MAX_CLUSTER_NODES		  32	///< Number of nodes.

/** A node as declared in the configuration file.
*/
struct JazzNode
{
	char  alias		 [MAX_ALIAS_NAMELENGTH];	///< Name used internally, also for reporting.
	char  host_or_ip [MAX_HOST_NAMELENGTH];		///< Host or IP. Must be a valid hostname.
	int	  port;									///< Http port
};

//TODO: Document interface for module jazz_cluster.

//TODO: Implement interface for module jazz_cluster.

class JazzCluster	: public JazzObject {

	public:
		 JazzCluster(jazz_utils::pJazzConfigFile a_config,
					 jazz_utils::pJazzLogger	 a_logger = nullptr);
		~JazzCluster();

		API_ErrorCode StartService (jazz_utils::pJazzConfigFile p_config 		   = nullptr);
		API_ErrorCode ShutDown     (bool 						restarting_service = false);

		/** Wrapper method logging events through a JazzLogger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param message	A message.

			See JazzLogger for details.
		*/
		inline void log (int loglevel, const char *message) { if (p_logger != nullptr) p_logger->log(loglevel, message); }

		/** Wrapper method logging events through a JazzLogger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param fmt		The printf-style format string.
			\param ...		The list of parameters as a variadic list of parameters.

			See JazzLogger for details.
		*/
		inline void log_printf (int loglevel, const char *fmt, ...) {
			if (p_logger != nullptr) {
				va_list args;
				va_start(args, fmt);
				p_logger->log_printf(loglevel, fmt, args);
				va_end(args);
			}
		}

	private:

		jazz_utils::pJazzConfigFile p_config;
		jazz_utils::pJazzLogger		p_logger;
};

}

#endif

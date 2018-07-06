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

#include <signal.h>

#include "src/jazz_main/jazz_restapi.h"

namespace jazz_restapi
{

using namespace std;

//TODO: Implement module jazz_restapi.

/**
//TODO: Document JazzHttpServer()
*/
JazzHttpServer::JazzHttpServer(jazz_utils::pJazzLogger a_logger) : JazzAPI(a_logger)
{
#ifdef DEBUG
	log(LOG_DEBUG, "Entering JazzHttpServer::JazzHttpServer");
#endif

//TODO: Implement JazzHttpServer
}


/**
//TODO: Document ~JazzHttpServer()
*/
JazzHttpServer::~JazzHttpServer()
{
//TODO: Implement ~JazzHttpServer

#ifdef DEBUG
	log(LOG_DEBUG, "Leaving JazzHttpServer::~JazzHttpServer");
#endif
}


/** Start the Jazz server.

\param p_config A pointer to a JazzConfigFile object containing the server configuration.

\return         On failure, EXIT_FAILURE. On success, the thread forks and only the parent process returns EXIT_SUCCESS, the child does not return.
The application is stopped when callback signalHandler_SIGTERM exits with EXIT_SUCCESS if shutting all services was successful or with EXIT_FAILURE
if not.

Starting logic:

 1. This first loads a configuration, returns EXIT_FAILURE if that fails.

	If conf is not void, it uses this file as the configuration source,
	else it tries config/jazz_config.ini

 2. Initializes the logger, returns EXIT_FAILURE if that fails.

 3. Loads the cluster configuration, returns EXIT_FAILURE if that fails.

 4. Finds a port using the variable JAZZ_NODE_WHO_AM_I, returns EXIT_FAILURE if that fails.

 5. Configure the server, including variables: flags, MHD_AcceptPolicyCallback and MHD_AccessHandlerCallback from the configuration.

 6. Registers all services configuration variables JazzHTTPSERVER.MHD_DISABLE_BLOCKS..MHD_DISABLE_RAMQ.

	if anything fails:
		calls jServices.stop_all() and returns EXIT_FAILURE

 7. Calls jServices.start_all()

	if anyone fails:
		calls jServices.stop_all() and returns EXIT_FAILURE

 8. Registers the signal handlers for SIGTERM

	if that fails:
		calls jServices.stop_all() and returns EXIT_FAILURE

 9. Forks

	if that fails:
		calls jServices.stop_all() and returns EXIT_FAILURE

	(The parent exits with EXIT_SUCCESS, the child continues)

 10. Calls MHD_start_daemon()

	if that fails:
		The child calls jServices.stop_all() and returns EXIT_FAILURE

	Then calls setsid() This creates a new session if the calling process is not a process group leader. The calling process is the leader of the
	new session, the process group leader of the new process group, and has no controlling terminal.

	And sleeps forever! (The child.)

//TODO: Update this documentation server_start()

*/
int JazzHttpServer::server_start(jazz_utils::pJazzConfigFile p_config,
						 		 SignalHandler				 p_sig_handler)
{
// 1. Get all the MHD server config settings from p_config

	int http_port;

	if (!p_config->get_key("HTTP_PORT", http_port)) {
		cout << "Failed to find server port in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed to find server port in configuration.");

		return EXIT_FAILURE;
	}

	int ok, debug, ssl, ipv6, pedantic, supp_date, tcp_fastopen;

	ok =   p_config->get_key("MHD_DEBUG", debug)
		 & p_config->get_key("MHD_SSL", ssl)
		 & p_config->get_key("MHD_IPv6", ipv6)
		 & p_config->get_key("MHD_PEDANTIC_CHECKS", pedantic)
		 & p_config->get_key("MHD_SUPPRESS_DATE", supp_date)
		 & p_config->get_key("MHD_USE_TCP_FASTOPEN", tcp_fastopen);

	if ((!ok ) | ((debug | ssl | ipv6 | pedantic | supp_date | tcp_fastopen) & 0xfffffffe)) {
		cout << "Failed parsing flags block in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed config in flags block.");

		return EXIT_FAILURE;
	}
	unsigned int server_flags =   debug*MHD_USE_DEBUG | ssl*MHD_USE_SSL | ipv6*MHD_USE_IPv6
				 				| pedantic*MHD_USE_PEDANTIC_CHECKS | supp_date*MHD_SUPPRESS_DATE_NO_CLOCK | tcp_fastopen*MHD_USE_TCP_FASTOPEN
								| MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;		// Only threading model for Jazz.

	MHD_OptionItem server_options[9];		// The variadic parameter MHD_OPTION_ARRAY, server_options, MHD_OPTION_END in MHD_start_daemon()

	int cml, cmi, ocl, oct, pic, tps, tss, lar;

	ok =   p_config->get_key("MHD_CONN_MEMORY_LIMIT", cml)
		 & p_config->get_key("MHD_CONN_MEMORY_INCR", cmi)
		 & p_config->get_key("MHD_CONN_LIMIT", ocl)
		 & p_config->get_key("MHD_CONN_TIMEOUT", oct)
		 & p_config->get_key("MHD_PER_IP_CONN_LIMIT", pic)
		 & p_config->get_key("MHD_THREAD_POOL_SIZE", tps)
		 & p_config->get_key("MHD_THREAD_STACK_SIZE", tss)
		 & p_config->get_key("MHD_LISTEN_ADDR_REUSE", lar);

	if (!ok) {
		cout << "Failed parsing integers block in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed config in integers block.");

		return EXIT_FAILURE;
	}

	MHD_OptionItem *pop = server_options;

	if (cml) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_LIMIT,	   cml, NULL}; pop++;
	if (cmi) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_INCREMENT, cmi, NULL}; pop++;
	if (ocl) pop[0] = {MHD_OPTION_CONNECTION_LIMIT,			   ocl, NULL}; pop++;
	if (oct) pop[0] = {MHD_OPTION_CONNECTION_TIMEOUT,		   oct, NULL}; pop++;
	if (pic) pop[0] = {MHD_OPTION_PER_IP_CONNECTION_LIMIT,	   pic, NULL}; pop++;
	if (tps) pop[0] = {MHD_OPTION_THREAD_POOL_SIZE,			   tps, NULL}; pop++;
	if (tss) pop[0] = {MHD_OPTION_THREAD_STACK_SIZE,		   tss, NULL}; pop++;
	if (lar) pop[0] = {MHD_OPTION_LISTENING_ADDRESS_REUSE,	   lar, NULL}; pop++;

	pop[0] = {MHD_OPTION_END, 0, NULL};

// 2. Register the signal handlers for SIGTERM

	int sig_ok = signal(SIGTERM, p_sig_handler) != SIG_ERR;

	if (!sig_ok) {
		cout << "Failed to register signal handlers." << endl;

		log(LOG_ERROR, "Failed to register signal handlers.");

		return EXIT_FAILURE;
	}
/*
// 9. Forks

	pid_t pid = fork();
	if (pid < 0)
	{
		jServices.stop_all();

		cout << "Failed to fork." << endl;

		jCommons.log(LOG_ERROR, "Failed to fork.");

		return EXIT_FAILURE;
	}
	if (pid > 0) return EXIT_SUCCESS; // This is parent process, exit now.

//10. Calls MHD_start_daemon()

	cout << "Starting server on port : " << port << endl;

	unsigned int			  flags;
	MHD_AcceptPolicyCallback  apc;
	MHD_AccessHandlerCallback dh;
	MHD_OptionItem *		  pops;

	jCommons.get_server_start_params(flags, apc, dh, pops);

	jzzdaemon = MHD_start_daemon (flags, port, apc, NULL, dh, NULL, MHD_OPTION_ARRAY, pops, MHD_OPTION_END);

	if (jzzdaemon == NULL)
	{
		jServices.stop_all();

		cout << "Failed to start the server." << endl;

		jCommons.log(LOG_ERROR, "Failed to start the server.");
	}

// Creates a new session if the calling process is not a process group leader. The calling process is the leader of the new session,
// the process group leader of the new process group, and has no controlling terminal.

	setsid();

#ifdef DEBUG
	cout << endl << "DEBUG MODE: -- Press any key to stop the server. ---" << endl;
	getchar();
	cout << endl << "Stopping ..." << endl;
	kill(getpid(), SIGTERM);
	sleep(1);
	cout << endl << "Failed :-(" << endl;
#endif

	while(true) sleep(60);
*/
}

} // namespace jazz_restapi


#if defined CATCH_TEST
#include "src/jazz_main/tests/test_restapi.ctest"
#endif

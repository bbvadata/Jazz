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


#include <signal.h>

#include "microhttpd.h"

#include "src/jazz_main/server.h"


namespace jazz_main
{

using namespace std;

/*	-----------------------------------------------
	 HttpServer : I m p l e m e n t a t i o n
--------------------------------------------------- */

HttpServer::HttpServer(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}


/** Start the Jazz server.

\param p_sig_handler	A function (of type pSignalHandler) that will be called when the process receives a SIGTERM signal.
\param p_daemon			Returns by reference the pointer that will be used to control the MHD_Daemon.

\return			On failure, EXIT_FAILURE. On success, the thread forks and only the parent process returns EXIT_SUCCESS, the child does
not return. The application is stopped when callback signalHandler_SIGTERM exits with EXIT_SUCCESS if shutting all services was successful
or with EXIT_FAILURE if not. On failure, the caller is responsible of stopping all started services (see jazz_main.cpp).

Starting logic:

 1. Get all the MHD server config settings via get_conf_key()

	The default config file is JAZZ_DEFAULT_CONFIG_PATH but that can be changed via command line argument (see jazz_main.cpp).

 2. Registers the signal handlers for SIGTERM. (See argument p_sig_handler)

 3. Forks (== The parent exits with EXIT_SUCCESS, the child continues to call MHD_start_daemon().)

 4. Calls MHD_start_daemon()

	Then calls setsid() This creates a new session if the calling process is not a process group leader. The calling process is the leader of the
	new session, the process group leader of the new process group, and has no controlling terminal.

	And sleeps forever! (Remember, it is the child of the original caller who exited with EXIT_SUCCESS.)
*/
StatusCode HttpServer::start(pSignalHandler p_sig_handler, pMHD_Daemon &p_daemon)
{
// 1. Get all the MHD server config settings via get_conf_key()

	int http_port;

	if (!get_conf_key("HTTP_PORT", http_port)) {
		cout << "Failed to find server port in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed to find server port in configuration.");

		return EXIT_FAILURE;
	}

	int ok, debug, ssl, ipv6, pedantic, supp_date, tcp_fastopen;

	ok =   get_conf_key("MHD_DEBUG", debug)
		 & get_conf_key("MHD_SSL", ssl)
		 & get_conf_key("MHD_IPv6", ipv6)
		 & get_conf_key("MHD_PEDANTIC_CHECKS", pedantic)
		 & get_conf_key("MHD_SUPPRESS_DATE", supp_date)
		 & get_conf_key("MHD_USE_TCP_FASTOPEN", tcp_fastopen);

	if ((!ok ) | ((debug | ssl | ipv6 | pedantic | supp_date | tcp_fastopen) & 0xfffffffe)) {
		cout << "Failed parsing flags block in configuration." << endl;

		log(LOG_ERROR, "JazzHttpServer::server_start() failed config in flags block.");

		return EXIT_FAILURE;
	}
	unsigned int server_flags =	  debug*MHD_USE_DEBUG | ssl*MHD_USE_SSL | ipv6*MHD_USE_IPv6 | pedantic*MHD_USE_PEDANTIC_CHECKS
								| supp_date*MHD_SUPPRESS_DATE_NO_CLOCK | tcp_fastopen*MHD_USE_TCP_FASTOPEN
								| MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;							// Only threading model for Jazz.

	MHD_OptionItem server_options[9];	// The variadic parameter MHD_OPTION_ARRAY, server_options, MHD_OPTION_END in MHD_start_daemon()

	int cml, cmi, ocl, oct, pic, tps, tss, lar;

	ok =   get_conf_key("MHD_CONN_MEMORY_LIMIT", cml)
		 & get_conf_key("MHD_CONN_MEMORY_INCR", cmi)
		 & get_conf_key("MHD_CONN_LIMIT", ocl)
		 & get_conf_key("MHD_CONN_TIMEOUT", oct)
		 & get_conf_key("MHD_PER_IP_CONN_LIMIT", pic)
		 & get_conf_key("MHD_THREAD_POOL_SIZE", tps)
		 & get_conf_key("MHD_THREAD_STACK_SIZE", tss)
		 & get_conf_key("MHD_LISTEN_ADDR_REUSE", lar);

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

// 3. Forks

	pid_t pid = fork();
	if (pid < 0) {
		cout << "Failed to fork." << endl;

		log(LOG_ERROR, "Failed to fork.");

		return EXIT_FAILURE;
	}
	if (pid > 0) return EXIT_SUCCESS; // This is the parent process, exit now.

// 4. Calls MHD_start_daemon()

	cout << "Starting server on port : " << http_port << endl;

	p_daemon = MHD_start_daemon (server_flags, http_port, NULL, NULL, http_request_callback, NULL,
								 MHD_OPTION_ARRAY, server_options, MHD_OPTION_END);

	if (p_daemon == NULL) {
		cout << "Failed to start the server." << endl;

		log(LOG_ERROR, "Failed to start the server.");
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
}


/** Server closing method.

This overrides the Service shut_down() just to set the return to SERVICE_NO_ERROR.

The true closing mechanism is a: MHD_stop_daemon (Jazz_MHD_Daemon); done by the signalHandler_SIGTERM() callback that captures SIGTERM.
*/
StatusCode HttpServer::shut_down(bool restarting_service)
{
	return SERVICE_NO_ERROR;
}

} // namespace jazz_main

#if defined CATCH_TEST
#include "src/jazz_main/tests/test_server.ctest"
#endif

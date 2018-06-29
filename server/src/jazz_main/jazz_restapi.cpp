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

namespace jazz_restapi
{

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

\param conf
\return on failure, EXIT_FAILURE. On success, the thread forks and only the parent process returns EXIT_SUCCESS, the child does not return. The
application is stopped when callback signalHandler_SIGTERM exits with EXIT_SUCCESS if shutting all services was successful or with EXIT_FAILURE
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
int JazzHttpServer::server_start()
{
//TODO: Implement server_start

	return EXIT_FAILURE;
/*
// 1. This first loads a configuration, returns EXIT_FAILURE if that fails.

	bool cnf_ok;

	if (strcmp("",	conf))
	{
		cnf_ok = jCommons.load_config_file(conf);

		cout << "Loading configuration \"" << conf << "\" " << okfail(cnf_ok);
	}
	else
	{
		cnf_ok = normal_verbose_load_configuration();
	}

	if (!cnf_ok)
	{
		cout << "No valid configuration loaded." << endl;

		return EXIT_FAILURE;
	}

// 2. Initializes the logger, returns EXIT_FAILURE if that fails.

	if (!jCommons.logger_init())
	{
		cout << "Logger failed to initialize." << endl;

		return EXIT_FAILURE;
	}

// 3. Loads the cluster configuration, returns EXIT_FAILURE if that fails.

	if (!jCommons.load_cluster_conf())
	{
		cout << "Failed to load the cluster configuration." << endl;

		jCommons.log(LOG_ERROR, "Failed to load the cluster configuration.");

		return EXIT_FAILURE;
	}

// 4. Finds a port using the variable JAZZ_NODE_WHO_AM_I, returns EXIT_FAILURE if that fails.

	int port;
	string me;
	if (!jCommons.get_config_key("JazzCLUSTER.JAZZ_NODE_WHO_AM_I", me) || !jCommons.get_cluster_port(me.c_str(), port))
	{
		cout << "Failed to find server port in configuration." << endl;

		jCommons.log(LOG_ERROR, "Failed to find server port in configuration.");

		return EXIT_FAILURE;
	}

// 5. Configure the server, including variables: flags, MHD_AcceptPolicyCallback and MHD_AccessHandlerCallback from the configuration.

	if (!jCommons.configure_MHD_server())
	{
		cout << "Failed to configure the http server." << endl;

		jCommons.log(LOG_ERROR, "Failed to configure the http server.");

		return EXIT_FAILURE;
	}

// 6. Registers all services configuration variables JazzHTTPSERVER.MHD_DISABLE_BLOCKS..MHD_DISABLE_RAMQ.

	register_service(&jBLOCKC,	  NOT_CONDITIONAL, NULL);

	register_service(&jAPI, NOT_CONDITIONAL, NULL);

// 7. Calls jServices.start_all()

	if (!jServices.start_all())
	{
		jServices.stop_all();

		cout << "Failed to start all services." << endl;

		jCommons.log(LOG_ERROR, "Failed to start all services.");

		return EXIT_FAILURE;
	}

// 8. Registers the signal handlers for SIGTERM

	int sig_ok = signal(SIGTERM, signalHandler_SIGTERM) != SIG_ERR;

	if (!sig_ok)
	{
		jServices.stop_all();

		cout << "Failed to register signal handlers." << endl;

		jCommons.log(LOG_ERROR, "Failed to register signal handlers.");

		return EXIT_FAILURE;
	}

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

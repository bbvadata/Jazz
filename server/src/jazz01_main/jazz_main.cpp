/* BBVA - Jazz: A lightweight analytical web server for data-driven applications.
   ------------

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

#define CATCH_CONFIG_MAIN		//This tells Catch to provide a main() - has no effect when CATCH_TEST is not defined

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <signal.h>

using namespace std;

/**< \brief	 The application entry point.

	At the highest level, see the description of main()
	For the server starting details, see the description of main_server_start()
*/

#include "src/include/jazz01_commons.h"
#include "src/jazz_main/jazz_instances.h"
#include "src/jazz_main/jazz_api.h"

/*~ end of automatic header ~*/


#define CMND_HELP		0	///< Command 'help' as a numerical constant (see parse_arg())
#define CMND_START		1	///< Command 'start' as a numerical constant (see parse_arg())
#define CMND_STOP		2	///< Command 'stop' as a numerical constant (see parse_arg())
#define CMND_STATUS		3	///< Command 'status' as a numerical constant (see parse_arg())

#define NOT_CONDITIONAL 0	///< 'fun' value for register_service(): Register always.
#define IF_PERIMETER	1	///< 'fun' value for register_service(): Register if key is MODE_INTERNAL_PERIMETER | MODE_EXTERNAL_PERIMETER.
#define IF_ZERO			2	///< 'fun' value for register_service(): Register if zero, meaning 'not disabled'.


/** Display the Jazz logo message automatically appending JAZZ_VERSION.
 */
void hello()
{
	cout << "\x20 888888" << endl
		 << "\x20 \x20 `88b" << endl
		 << " \x20 \x20 888" << endl
		 << " \x20 \x20 888\x20 8888b.\x20 88888888 88888888" << endl
		 << " \x20 \x20 888 \x20 \x20 `88b\x20 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 .d888888 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 88P 888\x20 888\x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 `Y888888 88888888 88888888" << endl
		 << " \x20 .d88P" << endl
		 << " .d88P'" << endl
		 << "888P'\x20 \x20 \x20 by BBVA Data & Analytics" << endl
		 << " \x20 \x20 \x20 \x20 \x20 \x20 \x20 version: " << JAZZ_VERSION << " (" << LINUX_PLATFORM << ")" << endl << endl;
}


/** Explain usage of the command line interface to stdout.
 */
void help()
{
	cout << "\x20 usage: jazz <config> start | stop | status" << endl << endl

		 << " <config>: A configuration file for the server in case of command start." << endl
		 << "\x20 \x20 \x20 \x20 \x20 \x20 by default, Jazz will try to load: config/jazz_config.ini" << endl
		 << "\x20 start\x20 : Start the jazz server." << endl
		 << "\x20 stop \x20 : Stop the jazz server." << endl
		 << "\x20 status : Just check if server is running." << endl;
}


/** Parse the command line argument (start, stop, ...) into a numeric constant, CMND_START, CMND_STOP, ...

	\param arg The argument as typed.
	\return the numeric constant and CMND_HELP when not known.
*/
int parse_arg(const char *arg)
{
	if (!strcmp("start",  arg)) return CMND_START;
	if (!strcmp("stop",	  arg)) return CMND_STOP;
	if (!strcmp("status", arg)) return CMND_STATUS;

	return CMND_HELP;
}


/** A verbose configuration load made a function to avoid its repetition.
*/
bool normal_verbose_load_configuration()
{
	string cfn;

	cfn = "config/jazz_config.ini";

	return jCommons.load_config_file(cfn.c_str());
}


/** The server's MHD_Daemon created by MHD_start_daemon() and needed for MHD_stop_daemon()
*/
struct MHD_Daemon * jzzdaemon;


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum)
{
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ..." << endl;

	MHD_stop_daemon (jzzdaemon);

	jCommons.logger_close();

	cout << "Stopping all services ..." << endl;

	bool clok = jServices.stop_all();

	cout << "Stopped all services : " << okfail(clok) << endl;

	if (!clok)
	{
		jCommons.log(LOG_ERROR, "Failed stopping all services.");

		exit (EXIT_FAILURE);
	}

	exit (EXIT_SUCCESS);
}


/** Register a service conditionally depending on some configuration key an some relation.

	\param serv	The service
	\param fun	The relation. NOT_CONDITIONAL = always, IF_PERIMETER = if key is MODE_INTERNAL_PERIMETER | MODE_EXTERNAL_PERIMETER,
				IF_ZERO = if not disabled
	\param key	The key
*/
void register_service(jazzService* serv, int fun, char * key)
{
	int val;

	if (fun != NOT_CONDITIONAL)
	{
		if (!jCommons.get_config_key(key, val))
		{
			jCommons.log_printf(LOG_ERROR, "Failed to find config key: %s", key);

			exit(EXIT_FAILURE);
		}
	}

	if (   (fun == NOT_CONDITIONAL)
		|| (fun == IF_PERIMETER && (val == MODE_INTERNAL_PERIMETER || val == MODE_EXTERNAL_PERIMETER))
		|| (fun == IF_ZERO && !val))
	{
		if (!jServices.register_service(serv))
		{
			jCommons.log(LOG_ERROR, "Failed to register service.");

			exit(EXIT_FAILURE);
		}
	}
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

 */
int main_server_start(const char *conf)
{
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
}


#if !defined CATCH_TEST

/** Entry point.

\param argc (Linux) number of arguments, counting the name of the executable.
\param argv (Linux) each argument, argv[0] being the name of the executable.

\return EXIT_FAILURE or EXIT_SUCCESS

When the command is "start":
	if running already, message + EXIT_FAILURE
	if configuration declared, but does no exist, message + EXIT_FAILURE
	else try to start the server calling main_server_start()

When the command is "stop":
	if not running already, message + EXIT_FAILURE
	else send a SIGTERM to the server and wait to check if it closes
		if the server closes before 20 seconds, message + EXIT_SUCCESS
		else message + EXIT_FAILURE

When the command is "status":
	if not running already, message + EXIT_FAILURE
	else message + EXIT_SUCCESS

When the command is anything else, too many or too few:
	show help + EXIT_FAILURE

 */
int main(int argc, char* argv[])
{
	int cmnd = (argc < 2 || argc > 3) ? CMND_HELP : parse_arg(argv[argc - 1]);

	if (cmnd == CMND_HELP || argc == 3 && cmnd != CMND_START)
	{
		hello();
		help();

		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	string proc_name ("./djazz");
#else
	string proc_name ("./jazz");
#endif

	pid_t jzzPID = proc_find(proc_name.c_str());

	if (!jzzPID) jzzPID = proc_find("/etc/jazz-server/jazz");

	if (!jzzPID)
	{
		if (cmnd != CMND_START)
		{
			cout << "The process \"" << proc_name << "\" is not running." << endl;

			exit(EXIT_FAILURE);
		}

		string conf ("");
		if (argc == 3)
		{
			conf = argv[1];

			if (!exists(conf.c_str()))
			{
				cout << "The file " << conf << " does not exist." << endl;

				exit(EXIT_FAILURE);
			}
		}

		hello();

		exit(main_server_start(conf.c_str()));
	}
	else
	{
		if (cmnd == CMND_START)
		{
			cout << "The process \"" << proc_name << "\" is already running with pid = " << jzzPID << "." << endl;

			exit(EXIT_FAILURE);
		}

		if (cmnd == CMND_STOP)
		{
			kill(jzzPID, SIGTERM);

			cout << "Break signal was sent to process \"" << proc_name << "\" running with pid = " << jzzPID << "." << endl << endl;
			for (int t = 0; t < 200; t++)
			{
				usleep (100000);
				if (!proc_find(proc_name.c_str()))
				{
					cout << "The process \"" << proc_name << "\" was stopped." << endl;

					exit(EXIT_SUCCESS);
				}
			}

			cout << "Waiting for \"" << proc_name << "\" to stop timed out." << endl;

			exit(EXIT_FAILURE);
		}

//		if (cmnd == CMND_STATUS)
//		{
			cout << "The process \"" << proc_name << "\" is running with pid = " << jzzPID << "." << endl;

			exit(EXIT_SUCCESS);
//		}
	}
};

#endif

/*	-----------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST
TEST_CASE("Test for jzzMAIN")
{
	// No real tests here. This line is added to avoid a warning when running ./switch_tests.R

	REQUIRE(sizeof(jzzdaemon) == 8);
}
#endif

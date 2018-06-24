/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

   2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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


#define CATCH_CONFIG_MAIN		//This tells Catch2 to provide a main() - has no effect when CATCH_TEST is not defined


#include "src/jazz_main/jazz_main.h"


#if !defined CATCH_TEST


/** Display the Jazz logo message automatically appending JAZZ_VERSION.
 */
void show_credits()
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
		 << "888P'" << endl << endl
		 << " \x20 \x20 (c)" << JAZZ_YEARS << " - kaalam.ai - The Authors of Jazz" << endl << endl
		 << " \x20 \x20This product includes software developed at BBVA" << endl
		 << " \x20 \x20 Licensed under the Apache License, Version 2.0" << endl
		 << " \x20 \x20 \x20 http://www.apache.org/licenses/LICENSE-2.0" << endl
		 << " \x20 \x20 \x20 \x20 \x20version: " << JAZZ_VERSION << " (" << LINUX_PLATFORM << ")" << endl
#ifdef DEBUG
		 << endl
		 << " \x20 \x20 \x20 \x20 \x20 \x20 *** D E B U G - B U I L D ***" << endl
#endif
		 << endl;
}


/** Explain usage of the command line interface to stdout.
 */
void show_usage()
{
	cout << "\x20 usage: jazz <config> start | stop | status" << endl << endl

		 << " <config>: A configuration file for the server in case of command start." << endl
		 << "\x20 \x20 \x20 \x20 \x20\x20 by default, Jazz will try to load: " << JAZZ_DEFAULT_CONFIG_PATH << endl
		 << "\x20 start\x20 : Start the jazz server." << endl
		 << "\x20 stop \x20 : Stop the jazz server." << endl
		 << "\x20 status : Just check if server is running." << endl;
}


/** Parse the command line argument (start, stop, ...) into a numeric constant, CMND_START, CMND_STOP, ...

	\param arg The argument as typed.
	\return the numeric constant and CMND_HELP when not known.
*/
int parse_command(const char *arg)
{
	if (!strcmp("start",  arg)) return CMND_START;
	if (!strcmp("stop",	  arg)) return CMND_STOP;
	if (!strcmp("status", arg)) return CMND_STATUS;

	return CMND_HELP;
}


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
	int cmnd = (argc < 2 || argc > 3) ? CMND_HELP : parse_command(argv[argc - 1]);

	if (cmnd == CMND_HELP || argc == 3 && cmnd != CMND_START) {
		show_credits();
		show_usage();

		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	string proc_name ("./djazz");
#else
	string proc_name ("./jazz");
#endif

	pid_t jzzPID = jazz_utils::FindProcessIdByName(proc_name.c_str());

	if (!jzzPID) jzzPID = jazz_utils::FindProcessIdByName("/etc/jazz-server/jazz");

	if (!jzzPID) {
		if (cmnd != CMND_START) {
			cout << "The process \"" << proc_name << "\" is not running." << endl;

			exit(EXIT_FAILURE);
		}

		if (argc == 3) {
			if (!jazz_utils::FileExists(argv[1])) {
				cout << "The file " << argv[1] << " does not exist." << endl;

				exit(EXIT_FAILURE);
			}
			if (!jazz_instances::J_CONFIG.load_config(argv[1])) {
				cout << "The configuration file " << argv[1] << " could not be parsed." << endl;

				exit(EXIT_FAILURE);
			}
			cout << endl
				 << "**NOTE:** The configuration file " << argv[1] << " has been loaded." << endl
				 << "---------" << endl << endl;
		}

		show_credits();

		exit(jazz_instances::J_HTTP_SERVER.server_start());

	} else {

		if (cmnd == CMND_START) {
			cout << "The process \"" << proc_name << "\" is already running with pid = " << jzzPID << "." << endl;

			exit(EXIT_FAILURE);
		}

		if (cmnd == CMND_STOP) {
			kill(jzzPID, SIGTERM);

			cout << "Break signal was sent to process \"" << proc_name << "\" running with pid = " << jzzPID << "." << endl << endl;
			for (int t = 0; t < 200; t++) {
				usleep (100000);
				if (!jazz_utils::FindProcessIdByName(proc_name.c_str())) {
					cout << "The process \"" << proc_name << "\" was stopped." << endl;

					exit(EXIT_SUCCESS);
				}
			}

			cout << "Waiting for \"" << proc_name << "\" to stop timed out." << endl;

			exit(EXIT_FAILURE);
		}

// (cmnd == CMND_STATUS)

		cout << "The process \"" << proc_name << "\" is running with pid = " << jzzPID << "." << endl;

		exit(EXIT_SUCCESS);
	}
};

#endif

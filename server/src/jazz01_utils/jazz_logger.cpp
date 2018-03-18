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

#include <chrono>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

using namespace std;

/**< \brief	In charge of providing logging functionality to the rest of the modules and directing it to persistence.

	This does not define a class, but defines methods declared in jazzCommons.

	See help of method log() for details.
*/

#include "src/include/jazz01_commons.h"
#include "src/jazz01_blocks/jazz_blocks.h"

/*~ end of automatic header ~*/

/** Create a log event with a static message.

\param loglevel The trace level. One of:

  LOG_DEBUG 1  -  Just for checking, normally not logged, should not exist when NDEBUG. In that case, it becomes a LOG_WARN to force removing it.
  LOG_INFO	2  -  A good, non trivial, non frequent event to discard trouble. E.g., "Jazz successfully installed host xxx", "backup completed ok."
  LOG_MISS	3  -  A function returned an error status. This may still be normal. E.g., "configuration key xxx cannot be converted to integer."
  LOG_WARN	4  -  A warning. More serious than the previous. Should not happen. It is desirable to treat the existence of a warning as a bug.
  LOG_ERROR 5  -  Something known to be a requisite is failing. The program or task halts due to this.

\param message A message that can be up to 256 - LEFTAUTO characters (see source for details).

	See the configuration file for options on how the output can be directed.

	If loglevel >= LOG_WARN and compiled for DEBUG, the output also goes to stderr.
*/
void jazzCommons::log(int loglevel, const char *message)
{
	chrono::steady_clock::time_point now = chrono::steady_clock::now();

	int64_t elapsed = chrono::duration_cast<chrono::microseconds>(now - log_big_bang).count();
	double sec = elapsed/1000000.0;

#ifdef NDEBUG
	if (loglevel == LOG_DEBUG) loglevel == LOG_WARN;	// Should not exist in case of NDEBUG. It becomes a LOG_WARN to force removing it.
#endif

	char buffer [256];

#define LEFTAUTO	28

	sprintf (buffer, "%12.6f : %02d : %5zu : ", sec, loglevel, syscall(SYS_gettid));	// This fills LEFTAUTO char
	while (strlen(buffer) > LEFTAUTO)
	{
		int j = strlen(buffer);
		for (int i = 1; i < j; i++) buffer[i] = buffer[i + 1];
		buffer[0] = '+';
	}
	strncpy(&buffer[LEFTAUTO], message, 256 - LEFTAUTO);					// This fills in the range [LEFTAUTO..254]
	buffer[255] = '\0';														// Last pos gets the terminator

#ifdef DEBUG
	if (loglevel >= LOG_WARN) cerr << buffer << endl;
#endif

	if (log_2_local)
	{
		ifstream is;
		filebuf * fb = is.rdbuf();

		fb->open (logfilename, ios::out | ios::app);

		fb->sputn(buffer, strlen(buffer));
		fb->sputc('\n');

		fb->close();
	}

	if (!log_block_size)
	{
		cerr << buffer << endl;

		return;
	}

//TODO: Implement logging to the persistence blocks.

};


/** Initialize the logger by loading its configuration.
*/
bool jazzCommons::logger_init ()
{
	string fnam;

	bool lok =	 get_config_key("JazzLOGGER.LOGGER_LINUX_LOCAL", fnam)
			   & get_config_key("JazzLOGGER.LOGGER_BLOCK_SIZE", log_block_size)
			   & get_config_key("JazzLOGGER.LOGGER_BLOCK_KEEPBLOCKS", log_keep_blocks)
			   & get_config_key("JazzLOGGER.LOGGER_BLOCK_TIMEOUT", log_block_timeout);

	if (!lok) return false;

	log_big_bang = chrono::steady_clock::now();

	log_2_local = fnam.compare("VOID");

	if (log_2_local)
	{
		int i = sizeof(logfilename) - 1;
		strncpy(logfilename, fnam.c_str(), i);
		logfilename[i] = '\0';

		log(LOG_INFO, "+-\x2D-----\x2D-------\x2D-------------------\x2D-----\x2D-+");
		log(LOG_INFO, "| \x20 --- \x20 N E W \x20 E X E C U T I O N \x20 --- \x20 |");
		log(LOG_INFO, "+-\x2D-----\x2D-------\x2D-------------------\x2D-----\x2D-+");
	}

	return true;
}


/** Create a log event with a printf style string including a variadic list of parameters.

\param loglevel The trace level. (see jazzCommons::log())
\param fmt		The printf-style format string.
\param ...		The list of parameters.

	It is a wrapper function calling jazzCommons::log(). (See jazzCommons::log() for details.)
*/
void jazzCommons::log_printf (int loglevel, const char *fmt, ...)
{
	char buffer[256];

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	return log(loglevel, buffer);
}


/** Put the logger in safe mode avoiding it trying to write to persistence.

	This is part of shutdown to provide some logging support after the persistence service closes.
*/
void jazzCommons::logger_close()
{
	log_block_size = 0;
}

/*	--------------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST

SCENARIO("Scenario jzzLOGGER ...")
{
	jCommons.log_printf(LOG_INFO, "log_printf(2+2=%d, name=%s, hex=%08x, pi=%4.2f)", 4, "me", 0xbadf00d, 3.141592);
}

#endif

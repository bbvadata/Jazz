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

#include <chrono>
#include <iostream>
#include <fstream>
#include <map>

#include <string.h>

#include <unistd.h>
#include <dirent.h>

#include <sys/syscall.h>
#include <stdarg.h>

#include "src/jazz_elements/jazz_datablocks.h"


#ifndef INCLUDED_JAZZ_ELEMENTS_UTILS
#define INCLUDED_JAZZ_ELEMENTS_UTILS


#define TENBITS_LUT_SIZE 1024	///< The size of a table indexable by all possible output values of TenBitsAtAddress()


/** The trace levels for argument loglevel in JazzLogger.log()
*/
#define LOG__

	/// Just for checking, normally not logged, should not exist in case of NDEBUG. In that case, it becomes a LOG_WARN to force removing it.
#define LOG_DEBUG			  1

	/// A good, non trivial, non frequent event to discard trouble. E.g., "Jazz successfully installed on host xxx", "backup completed."
#define LOG_INFO			  2

	/// A function returned an error status. This may still be normal. E.g., "configuration key xxx cannot be converted to integer."
#define LOG_MISS			  3

	/// A warning. More serious than the previous. Should not happen. It is desirable to treat the existence of a warning as a bug.
#define LOG_WARN			  4

	/// Something known to be a requisite is failing. The program or task halts due to this.
#define LOG_ERROR			  5

	/// Maximum length for file names in JazzConfigFile and JazzLogger.
#define MAX_FILENAME_LENGTH	256


/**< \brief Miscellaneous utility functions for Jazz.

	This module defines many unrelated functions needed by Jazz. The only rule is: functions and classes
without global variables.
*/
namespace jazz_utils
{

bool		 FileExists			   (const char* file_name);
int			 CountBytesFromUtf8	   (char *buff, int length);
char		*ExpandEscapeSequences (char *buff);
pid_t		 FindProcessIdByName   (const char *name);
uint64_t	 MurmurHash64A		   (const void *key, int len);
std::string	 CleanConfigArgument   (std::string s);


/** Get ten bits taking the least significant 5 of the first two characters of a string.
	Warning: No pointer validation or length check. Never use on nullptr or "".
*/
inline int TenBitsAtAddress (const char* str) {
	return ((str[1] & 0x1f) << 5) | (str[0] & 0x1F);
}


/** Return the time elapsed in microseconds as a 64 bit integer since a primordial event.

	\param big_bang The primordial event
	\return			The time elapsed in microseconds
*/
inline int64_t elapsed_us(jazz_datablocks::TimePoint big_bang) {
	jazz_datablocks::TimePoint now = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::microseconds>(now - big_bang).count();
}


typedef class JazzConfigFile *pJazzConfigFile;
typedef class JazzLogger	 *pJazzLogger;


/** A configuration file as a key/value store.

	The configuration is loaded when constructing the object and available for reading as int, double or string via get_key().

	The input file format removes anything to the right of a // as a remark. It uses a single equal (=) character to separate the key
	from the value. It also performs a very simple form of quote replacement using CleanConfigArgument().
*/
class JazzConfigFile {

	public:

		JazzConfigFile(const char *input_file_name);

		int	 num_keys ();

		bool get_key  (const char *key, int &value);
		bool get_key  (const char *key, double &value);
		bool get_key  (const char *key, std::string &value);

		void debug_put(const std::string key, const std::string val);

	private:

		std::map<std::string, std::string> config;
};


/** A simple logger.

	This objects logs events one line per event. It prefixes the time since the logger was created, the trace level and the thread id as
in "   0.224036 : 02 :	2872 : jzzAPI started.".  A printf style version supports printing variables using variadic arguments.
*/
class JazzLogger {

	public:

		 JazzLogger(const char			 *output_file_name);
		 JazzLogger(const JazzConfigFile  config,
					const char			 *config_key);
		~JazzLogger();

		int	 get_output_file_name (char *buff, int buff_size);

		void log		(int loglevel, const char *message);
		void log_printf	(int loglevel, const char *fmt, ...);
		void log_printf	(int loglevel, const char *fmt, va_list args);

	private:

		void InitLogger();

		char file_name [MAX_FILENAME_LENGTH];
		std::ifstream f_stream;
		std::filebuf *f_buff;
		jazz_datablocks::TimePoint big_bang;
};


} // namespace jazz_utils

#endif

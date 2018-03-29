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


/**< \brief Miscelaneous utility functions for Jazz.

	This module defines many unrelated functions needed by Jazz. The only rule is: functions, not classes and
without global variables. For example, the logger does not belong here, since it is a configurable object, but
some elements of the logger that are just pure functions are here.
*/


#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#define TENBITS_LUT_SIZE 1024	///< The size of a table indexable by all possible output values of TenBitsAtAddress()


#ifndef INCLUDED_JAZZ_ELEMENTS_UTILS
#define INCLUDED_JAZZ_ELEMENTS_UTILS


/** The trace levels for argument loglevel in JazzLogger.log()
*/
#define LOG__

	/// Just for checking, normally not logged, should not exist in case of NDEBUG. In that case, it becomes a LOG_WARN to force removing it.
#define LOG_DEBUG		1

	/// A good, non trivial, non frequent event to discard trouble. E.g., "Jazz successfully installed on host xxx", "backup completed."
#define LOG_INFO		2

	/// A function returned an error status. This may still be normal. E.g., "configuration key xxx cannot be converted to integer."
#define LOG_MISS		3

	/// A warning. More serious than the previous. Should not happen. It is desirable to treat the existence of a warning as a bug.
#define LOG_WARN		4

	/// Something known to be a requisite is failing. The program or task halts due to this.
#define LOG_ERROR		5


namespace jazz_utils
{

int			 CountBytesFromUtf8	   (char *buff, int len);
char		*ExpandEscapeSequences (char *buff);
pid_t		 FindProcessIdByName   (const char *name);
uint64_t	 MurmurHash64A		   (const void *key, int len);
std::string  CleanConfigArgument   (std::string s);


/** Get ten bits taking the least significant 5 of the first two characters of a string.
	Warning: No pointer validation or length check. Never use on nullptr or "".
*/
inline int TenBitsAtAddress (const char* str)
{
	return ((str[1] & 0x1f) << 5) | (str[0] & 0x1F);
}


class JazzConfigFile {

	public:

		 JazzConfigFile(const char *input_file_name);

		int  num_keys ();

		bool get_key  (const char *key, int &value);
		bool get_key  (const char *key, double &value);
		bool get_key  (const char *key, std::string &value);

		void debug_put(const std::string key, const std::string val);

	private:

		std::map<std::string, std::string> config;
};


typedef std::chrono::steady_clock::time_point TimePoint;

class JazzLogger {

	public:

		 JazzLogger(const char *output_file_name);
		~JazzLogger();

		void log		(int loglevel, const char *message);
		void log_printf	(int loglevel, const char *fmt, ...);

	private:

		TimePoint big_bang = std::chrono::steady_clock::now();	/// Clock zero for the logger
};


}

#endif

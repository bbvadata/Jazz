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


#include <chrono>
#include <iostream>
#include <fstream>
#include <map>

#include <string.h>

#include <unistd.h>
#include <dirent.h>

#include <sys/syscall.h>
#include <stdarg.h>


#include "src/jazz_elements/types.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_UTILS
#define INCLUDED_JAZZ_ELEMENTS_UTILS


namespace jazz_elements
{

/* Miscellaneous utility functions for Jazz.

	This module defines many unrelated functions needed by Jazz. The only rule is: functions and classes
without global variables.
*/

#define TENBITS_LUT_SIZE 1024	///< The size of a table indexable by all possible output values of TenBitsAtAddress()


/** The trace levels for argument loglevel in Logger.log()
*/

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

/// Maximum length for file names in ConfigFile and Logger.
#define MAX_FILENAME_LENGTH	256

/** Constants for StatusCode values
*/
#define SERVICE_NO_ERROR				  0		///< No errors found processing the API call.
#define SERVICE_NOT_IMPLEMENTED			 -1		///< Returned by the Service class (only descendents should be called.).
#define SERVICE_ERROR_BAD_CONFIG		 -2		///< Generic error related with configuration parsing.
#define SERVICE_ERROR_STARTING			 -3		///< Generic error related with starting a service.
#define SERVICE_ERROR_NO_MEM			 -4		///< Specific error where configured allocation RAM failed.
#define SERVICE_ERROR_LOADING_STAT		 -5		///< Error happened while loading static resources.
#define SERVICE_ERROR_NEW_BLOCK_ARGS	 -6		///< Error with arguments to a new_block() call.
#define SERVICE_ERROR_WRONG_TYPE		 -7		///< Error with block type (used by new_tuple()).
#define SERVICE_ERROR_WRONG_NAME		 -8		///< Error with block/item names (used by new_tuple()).
#define SERVICE_ERROR_WRONG_ARGUMENTS	 -9		///< Error with arguments any call.
#define SERVICE_ERROR_BAD_BLOCK			-10		///< A general error when Blocks cannot be serialized for whatever reason.
#define SERVICE_ERROR_BAD_NEW_KIND		-11		///< Error in new_block (form 2) with Kind.new_kind()
#define SERVICE_ERROR_BAD_KIND_ADD		-12		///< Error in new_block (form 2) with Kind.add_item()
#define SERVICE_ERROR_WRITE_FORBIDDEN	-13		///< Conflict with a block existing and the argument mode in a put()
#define SERVICE_ERROR_WRITE_FAILED		-14		///< put() failed
#define SERVICE_ERROR_REMOVE_FAILED		-15		///< remove() failed (either on a key or an entity)
#define SERVICE_ERROR_BLOCK_NOT_FOUND	-16		///< get() failed (either on a key or an entity)
#define SERVICE_ERROR_CREATE_FAILED		-17		///< new_entity() failed
#define SERVICE_ERROR_PARSING_NAMES		-18		///< Error with format in Container.as_locator().


/** Default path to config file
*/
#define JAZZ_DEFAULT_CONFIG_PATH "config/jazz_config.ini"

/// Type returned by the Service API
typedef int StatusCode;


bool		 FileExists			   (const char* file_name);
char		*ExpandEscapeSequences (char *buff);
pid_t		 FindProcessIdByName   (const char *name);
uint64_t	 MurmurHash64A		   (const void *key, int len);
std::string	 CleanConfigArgument   (std::string s);


/// A lookup table for all the possible results of a TenBitsAtAddress() call.
typedef int TenBitsLUT[TENBITS_LUT_SIZE];


/** \brief Get ten bits taking the least significant 5 of the first two characters of a string.

	Warning: No pointer validation or length check. Never use on nullptr or "".
*/
inline int TenBitsAtAddress (const char* str) {
	return ((str[1] & 0x1f) << 5) | (str[0] & 0x1F);
}


/** \brief Return server running time in microseconds as a 64 bit integer.

	\param big_bang The primordial event (== Jazz server start ;)
	\return			The time elapsed in microseconds
*/
inline int64_t elapsed_mu_sec(jazz_elements::TimePoint big_bang) {
	jazz_elements::TimePoint now = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::microseconds>(now - big_bang).count();
}


/** \brief Check if a name is valid (Without using the regex).

	\param p_name The name to be checked
	\return		  True if valid
*/
inline bool valid_name(pChar p_name) {
	char ch = p_name++[0];

	if (!ch)
		return false;

	if (ch < 'A' || ch > 'z' || (ch > 'Z' && ch < 'a'))
		return false;

	for (int i = 1; i < NAME_SIZE; i++) {
		char ch = p_name++[0];

		if (!ch)
			return true;

		if (ch < '0' || ch > 'z' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < '_') || ch == 0x60)
			return false;
	}
	return false;
}


/** \brief A configuration file as a key/value store.

	The configuration is loaded when constructing the object and available for reading as int, double or string via get_key().

	The input file format removes anything to the right of a // as a remark. It uses a single equal (=) character to separate the key
	from the value. It also performs a very simple form of quote replacement using CleanConfigArgument().
*/
class ConfigFile {

	public:

		ConfigFile(const char *input_file_name);

		bool load_config (const char *input_file_name);

		int	 num_keys ();

		bool get_key  (const char *key, int &value);
		bool get_key  (const char *key, double &value);
		bool get_key  (const char *key, std::string &value);

		void debug_put(const std::string key, const std::string val);

	private:

		std::map<std::string, std::string> config;
};
typedef ConfigFile *pConfigFile;


/** \brief A simple logger.

	This objects logs events one line per event. It prefixes the time since the logger was created, the trace level and the thread id as
in "   0.224036 : 02 :	2872 : jzzAPI started.".  A printf style version supports printing variables using variadic arguments.
*/
class Logger {

	public:

		 Logger(const char		 *output_file_name);
		 Logger(	  ConfigFile  config,
				const char		 *config_key);
		~Logger();

		int	 get_output_file_name (char *buff, int buff_size);

		void log		(int loglevel, const char *message);
		void log_printf	(int loglevel, const char *fmt, ...);
		void log_printf	(int loglevel, const char *fmt, va_list args);

#if defined CATCH_TEST
		bool SkipLogOnce;
#endif

	private:

		void InitLogger();

		char file_name [MAX_FILENAME_LENGTH];
		std::ifstream f_stream;
		std::filebuf *f_buff;
		jazz_elements::TimePoint big_bang;
};
typedef Logger *pLogger;


/** \brief A Jazz Service is a globally instanced configurable object that may allocate RAM.

	A service is anything that requires configuration and a logger. Only a service can own (alloc from the system) RAM, anything else
	allocates RAM from a service. Only Services (and some callback functions) are instantiated in Jazz instances and there is only one
	of each. The abstract class just defines the API.

	A Service should NOT read the configuration before its .start() method is called as it may change between construction (at loading)
	and .start() (at running).
*/
class Service {

	public:

		 Service(pLogger	 a_logger,
			     pConfigFile a_config);

		/// A simple start()/shut_down() interface (Restart is: shut_down(TRUE):start())
		virtual StatusCode start	 ();
		virtual StatusCode shut_down ();

		/** Wrapper method logging events through a Logger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param message	A message.

			See Logger for details.
		*/
		inline void log (int loglevel, const char *message) { if (p_log != nullptr) p_log->log(loglevel, message); }

		/** Wrapper method logging events through a Logger when the logger was passed to the constructor of this class.

			\param loglevel The trace level.
			\param fmt		The printf-style format string.
			\param ...		The list of parameters as a variadic list of parameters.

			See Logger for details.
		*/
		inline void log_printf (int loglevel, const char *fmt, ...) {
			if (p_log != nullptr) {
				va_list args;
				va_start(args, fmt);
				p_log->log_printf(loglevel, fmt, args);
				va_end(args);
			}
		}

		/** Wrapper method to get configuration values when the ConfigFile was passed to the constructor of this class.

			\param key	 The configuration key to be searched.
			\param value Value to be returned only when the function returns true.
			\return		 True when the key exists and can be returned with the specific (overloaded) type.

			See ConfigFile for details.
		*/
		bool get_conf_key (const char *key, int &value) {
			if (p_conf != nullptr) return p_conf->get_key(key, value); else return false; }

		bool get_conf_key (const char *key, double &value) {
			if (p_conf != nullptr) return p_conf->get_key(key, value); else return false; }

		bool get_conf_key (const char *key, std::string &value) {
			if (p_conf != nullptr) return p_conf->get_key(key, value); else return false; }

	private:

		pLogger		p_log;
		pConfigFile	p_conf;
};
typedef Service *pService;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_UTILS

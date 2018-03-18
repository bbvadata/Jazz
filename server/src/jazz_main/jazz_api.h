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

using namespace std;

/*! \brief API class implementing the non-security related functionality of the Jazz server.

This module implements both the callback function (which can be a direct callback or called through jzzPERIMETRAL) and a class that parses API calls.

The callback function calls the appropriate instanced services and handles the http responses on successful url parsing or tries a jazzWebsource
call with the url if not. If that call is successful it provides the resource, if not it delivers a 404 (with a possible 404 resource).
*/

#include "src/include/jazz01_commons.h"
#include "src/jazz_misc/jazz_websource.h"
#include "src/jazz_main/jazz_instances.h"

/*~ end of automatic header ~*/

#ifndef jzz_IG_JZZAPI
#define jzz_IG_JZZAPI

/** Http methods
*/
#define HTTP_NOTUSED	0
#define HTTP_OPTIONS	1
#define HTTP_HEAD		2
#define HTTP_GET		3
#define HTTP_PUT		4
#define HTTP_DELETE		5

#define MAX_INSTRUMENT_LENGTH	  32	///< The maximum number of char for an instrument or a function name.
#define MAX_PARAM_LENGTH		  64	///< The maximum length of a parameter string.
#define GET_FUN_BUFFER_SIZE		2048	///< Size of the response buffer for GET functions.

struct instumentName {char name[MAX_INSTRUMENT_LENGTH];};	///< Name for instruments. Instruments, like persistedKey use [a-zA-Z0-9_]+
typedef instumentName apifunctionName;						///< Name for functions is the same type as for instruments.

struct apifunctionParam {char param[MAX_PARAM_LENGTH];};	///< To contain parsed parameters to a function. Uses [a-zA-Z0-9_\-.,;&]+

typedef persistedKey meshName;								///< Name for meshes.


struct parsedURLhea
{
	bool	isInstrumental;	///< It is a query on meshes/instruments. Otherwise a block API query.
	bool	hasAKey;		///< The (block API) query includes a block name.
	bool	hasAFunction;	///< The query includes a function.
	bool	deleteSource;	///< The function requires deleting sources => exclusive access.
};

struct parsedURL	: parsedURLhea
{
	int				 source;		///< The index to the source.
	union {
		persistedKey key;			///< The key in case of a block (isInstrum = false) query.
		meshName	 mesh;			///< The mesh in case of a mesh/instrument (isInstrum = true) query.
	};
	instumentName	 instrument;	///< The instrument name.
	apifunctionName	 function;		///< The function name.
	apifunctionParam parameters;	///< The functions parameters.
};


class jzzAPI: public jazzWebSource {

	public:

		 jzzAPI();
		~jzzAPI();

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();

		// sample this API call for statistics with an enter_api_call()/leave_api_call() pair

		bool sample_api_call();

		// parsing method

		bool parse_url(const char * url, int method, parsedURL &pars);

		// deliver http error pages

		int	 return_error_message (struct MHD_Connection * connection, int http_status);

		// execution method for the www API

		bool exec_www_get			(const char * url, struct MHD_Response * &response);

		// execution methods for the block API

		int	 exec_block_get			(parsedURL &pars, struct MHD_Response * &response);
		int	 exec_block_get_function(parsedURL &pars, struct MHD_Response * &response);
		bool exec_block_put			(parsedURL &pars, const char * upload, size_t size, bool reset);
		bool exec_block_put_function(parsedURL &pars, const char * upload, size_t size);
		int	 exec_block_kill		(parsedURL &pars);

		// execution methods for the instrumental API

		int	 exec_instr_get			(parsedURL &pars, struct MHD_Response * &response);
		int	 exec_instr_get_function(parsedURL &pars, struct MHD_Response * &response);
		bool exec_instr_put			(parsedURL &pars, const char * upload, size_t size);
		bool exec_instr_put_function(parsedURL &pars, const char * upload, size_t size);
		int	 exec_instr_kill		(parsedURL &pars);

#ifndef CATCH_TEST
	private:
#endif

		typedef jazzWebSource super;

		bool char_to_key_relaxed_end	(const char * pch, persistedKey		&key);
		bool char_to_instrum_relaxed_end(const char * pch, instumentName	&inst);
		bool char_to_param_strict_end	(const char * pch, apifunctionParam &param);

		int sample_n = 0, sample_i = 0;
};

extern jzzAPI jAPI;

#endif

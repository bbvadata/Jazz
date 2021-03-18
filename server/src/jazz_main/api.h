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


#include <map>

#include "src/include/jazz_agency.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_API
#define INCLUDED_JAZZ_MAIN_API


#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"


/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_agency
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_agency;

#define JAZZ_DEFAULT_CONFIG_PATH "config/jazz_config.ini"

/** Http methods
*/
#define HTTP_NOTUSED	0
#define HTTP_OPTIONS	1
#define HTTP_HEAD		2
#define HTTP_GET		3
#define HTTP_PUT		4
#define HTTP_DELETE		5

/// Parser limits

#define MAX_RECURSION_DEPTH	16		///< Max number of nested block references in a query.
#define EIGHT_BIT_LONG		256		///< Length of a CharLUT.


/// Parser return codes

#define PARSE_OK					0		///< Success.
#define GET_OK						0		///< Success.
#define PARSE_ERROR_INVALID_CHAR	1		///< Error unexpected char.
#define PARSE_ERROR_INVALID_SHAPE	2		///< Sequence of [] is inconsistent with a tensor.
#define PARSE_ERROR_ENCODING		3
#define PARSE_ERROR_NOT_A_STRING	4


/// Parsing states

#define MAX_NUM_PSTATES					20		///< Maximum number of non error states the parser can be in
#define MAX_TRANSITION_REGEX_LEN		32		///< Length of regex for state transitions. Used only in constants for LUT construction.
#define NUM_STATE_TRANSITIONS			100		///< Maximum number of state transitions in the parsing grammar. Applies to const only.

#define PSTATE_INITIAL					0		///< Parser state: Fist char of source string being parsed
#define PSTATE_CONST_INT				1		///< Parser state: Parsing integers
#define PSTATE_CONST_REAL				2		///< Parser state: Parsing real numbers
#define PSTATE_CONST_STR				3		///< Parser state: Parsing strings
#define PSTATE_CONST_STR_ENC			4		///< Parser state: Parsing url-encoding inside a string
#define PSTATE_CONST_NAME				5		///< Parser state: Parsing a name
#define PSTATE_CONST_SEP_INT			6		///< Parser state: Reached a cell separator while parsing integers
#define PSTATE_CONST_SEP_REAL			7		///< Parser state: Reached a cell separator while parsing real numbers
#define PSTATE_CONST_SEP_STR			8		///< Parser state: Reached a cell separator while parsing string
#define PSTATE_CONST_END_INT			9		///< Parser state: Reached end of part or expression while parsing integers
#define PSTATE_CONST_END_REAL			10		///< Parser state: Reached end of part or expression while parsing real numbers
#define PSTATE_CONST_END_STR			11		///< Parser state: Reached end of part or expression while parsing string

#define PSTATE_INVALID_CHAR				255		///< Parser state: The MOST GENERIC parsing error: char goes to invalid state.


/** A lookup table for all the possible values of a char mapped into an 8-bit state.
*/
struct NextStateLUT {
	char next[EIGHT_BIT_LONG];
};


/** A lookup table for all the possible values of a char mapped into a a bool.
*/
typedef bool CharGroupLUT[EIGHT_BIT_LONG];


/** A vector of NextStateLUT containing next states for all states and char combinations.
*/
struct StateSwitch {
	NextStateLUT	next[MAX_NUM_PSTATES];
};


/** A way to build constants defining the transtition from one state to the next via a regex.
*/
struct StateTransition {
	int  from;
	int	 to;
	char rex[MAX_TRANSITION_REGEX_LEN];
};


/** A vector of StateTransition.l This only runs once, when contruction the API object, initializes the LUTs from a sequence of
StateTransition constants in the source of api.cpp.
*/
typedef StateTransition StateTransitions[NUM_STATE_TRANSITIONS];


/** A map to convert urls to block names (in Persisted //static/).
*/
typedef std::map<std::string, std::string> Url2Name;


/** A pointer to char.
*/
typedef char *pChar;


/** A buffer to keep the state while parsing/executing a query
*/
struct APIParseBuffer {
	pContainer p_owner;							///< The "owner" of the block == the Container that can .get() the top-level call.
	int		   stack_size;						///< The stack size. Avoids having to memset() the whole APIParseBuffer.

	L_value l_value[MAX_RECURSION_DEPTH];		///< A stack of L_value used during parsing/executing
	R_value r_value[MAX_RECURSION_DEPTH];		///< A stack of R_value used during parsing/executing
};

typedef struct MHD_Response *pMHD_Response;

// ConfigFile and Logger shared by all services

extern ConfigFile  CONFIG;
extern Logger	   LOGGER;


extern int http_request_callback   (void *cls,
									struct MHD_Connection *connection,
									const char *url,
									const char *method,
									const char *version,
									const char *upload_data,
									size_t *upload_data_size,
									void **con_cls);


/** \brief Api: A Service to manage the REST API.

This service parses and executes http queries. It is aware and redistributes to all the appropriate services. It is called directly by
the http callback http_request_callback().

*/
class Api : public Container {

	public:

		Api (pLogger	 a_logger,
			 pConfigFile a_config,
			 pVolatile	 a_volatile,
			 pRemote	 a_remote,
			 pPersisted	 a_persisted,
			 pBebop		 a_bebop,
			 pAgency	 a_agency);

		StatusCode start	 ();
		StatusCode shut_down ();

		// parsing methods

		StatusCode parse	   (const char	   *url,
								int				method,
								APIParseBuffer &pars,
								bool			execution = true);

		StatusCode get_static  (const char	   *url,
								pMHD_Response  &response,
								bool			execution = true);

		// deliver http error pages

		int	return_error_message (struct MHD_Connection *connection,
								  int					 http_status);

		// Specific execution methods

		bool upload	   (APIParseBuffer &parse_buff,
						const char	   *upload,
						size_t			size,
						bool			continue_upload);
		bool remove	   (APIParseBuffer &parse_buff);
		bool http_get  (APIParseBuffer &parse_buff,
						pMHD_Response  &response);

		StatusCode get (pBlockKeeper   *p_keeper,
						pLocator		p_what);

		void base_names(BaseNames	   &base_names);

#ifndef CATCH_TEST
	private:
#endif

		StatusCode _load_statics (const char *path);

		StatusCode _parse_recurse  (pChar	&p_url,
									int		 method,
									L_value &l_value,
									R_value &r_value,
									bool	 execution,
									int		 rec_level);

		StatusCode _parse_exec_stage(pChar	 &p_url,
									 int	  method,
									 L_value &l_value,
									 R_value &r_value);

		StatusCode _parse_const_meta(pChar 		 &p_url,
									 BlockHeader &hea);

		StatusCode _parse_const_data(pChar 		  &p_url,
									 BlockHeader  &hea,
									 pBlockKeeper *p_keeper);
		pVolatile	p_volatile;
		pRemote		p_remote;
		pPersisted	p_persisted;
		pBebop		p_bebop;
		pAgency		p_agency;
		BaseNames	base;
		Url2Name	url_name;
		int			remove_statics;
};

extern Api	API;			// The API interface

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_API

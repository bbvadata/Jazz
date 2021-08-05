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


#include <microhttpd.h>

#if MHD_VERSION < 0x00097000
/*! \brief Replacement type for the future (libmicrohttpd-dev 0.9.72-2)

In a version somewhere between > 0.9.66-1 and <= 0.9.72-2 libmicrohttpd changed the return type of int MHD_YES and MHD_NO to a:

enum MHD_Result {MHD_NO = 0, MHD_YES = 1};

*/
typedef int MHD_Result;
#endif

/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_agency
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_agency;

/// Http methods

#define HTTP_NOTUSED					  0		///< Rogue value to fill the LUTs
#define HTTP_OPTIONS					  1		///< http predicate OPTIONS
#define HTTP_HEAD						  2		///< http predicate HEAD
#define HTTP_GET						  3		///< http predicate GET
#define HTTP_PUT						  4		///< http predicate PUT
#define HTTP_DELETE						  5		///< http predicate DELETE

/// Parser limits

#define SAFE_URL_LENGTH				   2048		///< Maximum safe assumption of URL length for both parsing and forwarding.
#define MAX_RECURSION_DEPTH				 16		///< Max number of nested block references in a query.

/// Parser return codes

#define PARSE_OK						  0		///< Success.
#define GET_OK							  0		///< Success.
#define PARSE_NOT_IMPLEMENTED			 -1		// TODO: remove this const
#define PARSE_ERROR_INVALID_CHAR		  1		///< Error unexpected char.
#define PARSE_ERROR_INVALID_SHAPE		  2		///< Sequence of [] is inconsistent with a tensor.
#define PARSE_ERROR_ENCODING			  3		///< Wrong utf8 decoding (length does not match expected length).
#define PARSE_ERROR_TOO_DEEP			  4		///< Number of shape dimensions exceed MAX_TENSOR_RANK in a constant.
#define PARSE_BRACKET_MISMATCH			  5		///< Number of [ in constant does not match number of ].

// Codes with no source in the StateSwitch (The parser, if necessary, will change the state (E.g., a tensor inside a tuple.))

#define PSTATE_CONST_END_INT			200		///< Parser state: Reached end of part or expression while parsing integers
#define PSTATE_CONST_END_REAL			201		///< Parser state: Reached end of part or expression while parsing real numbers
#define PSTATE_CONST_END_STR			202		///< Parser state: Reached end of part or expression while parsing string
#define PSTATE_CONST_END_KIND			203		///< Parser state: Reached end of part or expression while parsing string

#define PSTATE_INVALID_CHAR				255		///< Parser state: The MOST GENERIC parsing error: char goes to invalid state.


/** A map to convert urls to block names (in Persisted //static/).
*/
typedef std::map<std::string, std::string> Url2Name;


// ------------------------------ Remove all this ------------------------------------------------------------------------------------------
//TODO: remove Locator, pLocator, ContractStep, L_value and R_value
#define MAX_NESTED_CONTAINERS			2					///< (max) sub-container names in a locator (base is resolved to a pointer).

/** API buffers limit the size of http API calls, but also anything like lists of item names, dimension names, types, blocktypes, etc.
Since Blocks do allocate RAM, when they communicate these kind of text operations, they expect the caller to assign a buffer of
ANSWER_LENGTH chars. Of course, data serialization does not have any limits it is done by containers creating new blocks.
*/
#define ANSWER_LENGTH			4096		///< Size of an Answer buffer

/** \brief A string returned by some methods in jazz_elements and also by some API calls.

Some methods of Kinds an Tuples returning dimension names, etc. expect their buffers to fit the size of an Answer.
*/
struct Answer {
	char text[ANSWER_LENGTH];	///< A message, metadata, lists of items, columns, etc.
};

typedef Answer *pAnswer;

/** A Locator is used by all Containers using block names == all except the root deque. It locates a block (existing or new) and is the
base of both lvalues and rvalues.
*/
struct Locator {
	Name			container[MAX_NESTED_CONTAINERS];	///< All the sub-container names (the base container is used to route the call)
	Name			block;								///< The block name
};

typedef Locator *pLocator;

/** A contract is a kernel-API action perfromed on a block. It returns either another block or an Answer. It may use another block as
an argument. It is only one argument since multiple arguments will be merged into a tuple.

Contracts go from simple things like returning the type to slicing, function calls.

L_values do not have contracts. You cannot assign a[4] = "new value".

R_values can have multiple. E.g., you can a = math/average(weather_data/berlin:temp[1,4]).as_json. Note that the parser will first lock
"[1,4]" the constant into a new block. Then, it will lock "weather_data/berlin:temp[..]" which has two contracts: :column "temp" if berlin
is a table (also possible  :item "temp" if berlin is a tuple) and slice [1,4]. Finally, the call "math/average(..).as_json" on the
locked block has two contracts, a function call and the .as_json(). No step required more than 2. The total number of contracts is not
limited other than by query length, but the number of contracts per step is limited by MAX_CONTRACTS_IN_R_VALUE.
*/
struct ContractStep {
	Name			action;								///< The action performed at that step or an empty
	pTransaction	p_args;								///< The argument as a locked block or tuple (or nullptr if none)
};

/** An L_value is just a Locator
*/
typedef Locator L_value;

#define MAX_CONTRACTS_IN_R_VALUE   4

/** An R_value is a Locator with a number of contract steps to apply to it.
*/
struct R_value : Locator {
	ContractStep contract[MAX_CONTRACTS_IN_R_VALUE];	///< The contract to be a applied in order. The first empty one breaks.
};
// ------------------------------ Remove all this ------------------------------------------------------------------------------------------


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


extern MHD_Result http_request_callback(void *cls,
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
			 pChannels	 a_channels,
			 pVolatile	 a_volatile,
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

		MHD_Result	return_error_message (struct MHD_Connection *connection,
								  int					 http_status);

		// Specific execution methods

		bool upload	   (APIParseBuffer &parse_buff,
						const char	   *upload,
						size_t			size,
						bool			continue_upload);
		bool remove	   (APIParseBuffer &parse_buff);
		bool http_get  (APIParseBuffer &parse_buff,
						pMHD_Response  &response);

		StatusCode get (pTransaction   *p_txn,
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

		StatusCode _parse_const_meta(pChar	 &p_url,
									 pBlock	  p_block);

		StatusCode _parse_const_data(pChar 		  &p_url,
									 BlockHeader  &hea,
									 pTransaction *p_txn);

		pChannels	p_channels;
		pVolatile	p_volatile;
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

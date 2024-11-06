/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/include/jazz_elements.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_BASE_API
#define INCLUDED_JAZZ_BEBOP_BASE_API


/** \brief A language to access any container by base using locators.

	BaseAPI is a Container and a parent of: Core, ModelsAPI and API.

	\see Container, Core, ModelsAPI and API
*/
namespace jazz_bebop
{

using namespace jazz_elements;

#define SIZE_OF_BASE_ENT_KEY	(sizeof(Locator) - sizeof(pExtraLocator))	///< Used to convert ApiQueryState -> Locator

#define BASE_API_GET						 3	///< This is numerically equivalent to HTTP_GET in api.h http predicate GET
#define BASE_API_PUT						 4	///< This is numerically equivalent to HTTP_PUT in api.h http predicate PUT
#define BASE_API_DELETE						 5	///< This is numerically equivalent to HTTP_DELETE in api.h http predicate DELETE

/// Parser state values

#define PSTATE_INITIAL						 0	///< Begin parsing, assumes already seen / to avoid unnecessary initial counting
#define PSTATE_DONE_NODE					 1	///< Equivalent to PSTATE_INITIAL, except node was done and it cannot happen again
#define PSTATE_NODE0						 2	///< Already seen ///
#define PSTATE_IN_NODE						 3	///< Name starts after /// + letter, stays with valid char
#define PSTATE_BASE0						 4	///< Already seen //
#define PSTATE_IN_BASE						 5	///< Name starts after // + letter, stays with valid char
#define PSTATE_ENTITY0						 6	///< Already seen / after reading a base
#define PSTATE_IN_ENTITY					 7	///< Name starts after / + letter, stays with valid char
#define PSTATE_KEY0							 8	///< Already seen / after reading an entity
#define PSTATE_IN_KEY						 9	///< Name starts after / + letter, stays with valid char
#define PSTATE_INFO_SWITCH					10	///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_BASE_SWITCH					11	///< Found # while reading a base
#define PSTATE_ENT_SWITCH					12	///< Found # while reading a base
#define PSTATE_KEY_SWITCH					13	///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_FAILED						98	///< Set by the parser on any error (possibly in the r_value too)
#define PSTATE_COMPLETE_OK					99	///< Set by the parser on complete success

// BaseAPI.move_const() return values

#define RET_MV_CONST_FAILED					-1	///< return value for move_const() failed.
#define RET_MV_CONST_NOTHING				 0	///< return value for move_const() normal moving.
#define RET_MV_CONST_NEW_ENTITY				 1	///< return value for move_const() there is a ";.new" ending, otherwise parses ok.


/** \brief A buffer to keep the state while parsing/executing a query
*/
struct ApiQueryState {
	int	state;									///< The parser state from PSTATE_INITIAL to PSTATE_COMPLETE_OK
	int apply;									///< APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, .. APPLY_ASSIGN_CONST

	char l_node	[NAME_SIZE];					///< An optional name of a Jazz node that is either the only one or the left of assignment.
	char r_node	[NAME_SIZE];					///< An additional optional name of a Jazz node for the right part in an assignment.
	char base	[SHORT_NAME_SIZE];				///< A Locator compatible base for the l_value.
	char entity	[NAME_SIZE];					///< A Locator compatible entity for the l_value.
	char key	[NAME_SIZE];					///< A Locator compatible key for the l_value.
	char name	[NAME_SIZE];					///< A possible item name
	char url	[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)

	Locator r_value;							///< Parsed //r_base/r_entity/r_key
	Locator rr_value;							///< Parsed //r_base/r_entity/r_key(//rr_base/rr_entity/rr_key)
};


/** \brief BaseAPI: The parent of API and Core.

This manages parsing queries and them to the appropriate containers.

*/
class BaseAPI : public Container {

	public:

		BaseAPI(pLogger a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted);
	   ~BaseAPI();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		// parsing methods

		bool parse		   (ApiQueryState &q_state,
							pChar			p_url,
							int				method,
							bool			recurse = false);



#ifndef CATCH_TEST
	protected:
#endif

		bool parse_locator	(Locator	   &loc,
							 pChar			p_url);

		/** Copy the string "as-is" (without percent-decoding) a string into a buffer.

			\param p_buff	 A buffer to store the result. This first char must be a zero on call or it will not write anything, just check for
							 the return code and return. (This is a trick to avoid overriding the buffer in forward call. This function is an
							 internal part of parse().)
			\param buff_size The size of the output buffer (ending zero included).
			\param p_url	 The input string.
			\param p_base	 (optional) Prefix with a base to be prefixed

			\return			 RET_MV_CONST_FAILED on error (buffer sizes or start and final characters), RET_MV_CONST_NOTHING normal moving or
							 RET_MV_CONST_NEW_ENTITY there is a ";.new" ending and no errors.

		Note: This replaces expand_url_encoded() since the string passed to parse() is already %-decoded by libmicrohttpd.
		*/
		inline int move_const(pChar p_buff, int buff_size, pChar p_url, pChar p_base = nullptr) {

			if (*(p_url++) != '&')
				return RET_MV_CONST_FAILED;

			int url_len = strlen(p_url) - 1;

			pChar p_end = p_url + url_len;

			int ret = RET_MV_CONST_NOTHING;

			if (*p_end == 'w') {
				if ((*(--p_end) != 'e') || (*(--p_end) != 'n') || (*(--p_end) != '.') || (*(--p_end) != ';'))
					return RET_MV_CONST_FAILED;

				ret		 = RET_MV_CONST_NEW_ENTITY;
				url_len -= 4;
			}

			if (*p_end != ';' && *p_end != ']' && *p_end != ')')
				return RET_MV_CONST_FAILED;

			if (*p_buff != 0)
				return ret;

			if (p_base != nullptr) {
				if (buff_size < 4 + SHORT_NAME_SIZE)
					return RET_MV_CONST_FAILED;

				*(p_buff++) = '/';
				*(p_buff++) = '/';

				buff_size -= 2;

				for (int i = 0; i < SHORT_NAME_SIZE; i++) {
					char c = *(p_base++);

					buff_size--;

					if (c == 0) {
						*(p_buff++) = '/';
						break;
					}
					*(p_buff++) = c;
				}
			}

			if (url_len >= buff_size)
				return RET_MV_CONST_FAILED;

			memcpy(p_buff, p_url, url_len);

			p_buff += url_len;

			*(p_buff) = 0;

			return ret;
		}




		pChannels	p_channels;		///< The Channels container
		pVolatile	p_volatile;		///< The Volatile container
		pPersisted	p_persisted;	///< The Persisted container

		TenBitPtrLUT base_server;	///< A LUT to convert TenBitsAtAddress(base) into a pContainer.

};
typedef BaseAPI *pBaseAPI;			///< A pointer to a BaseAPI

#ifdef CATCH_TEST

// Instancing BaseAPI
// ------------------

extern BaseAPI	BAPI;

#endif

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_BASE_API

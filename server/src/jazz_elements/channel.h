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


#include "src/jazz_elements/container.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL
#define INCLUDED_JAZZ_ELEMENTS_CHANNEL


namespace jazz_elements
{

#define BASE_BASH_10BIT				0x022		//< First 10 bits of base "bash"
#define BASE_FILE_10BIT				0x126		//< First 10 bits of base "file"
#define BASE_HTTP_10BIT				0x288		//< First 10 bits of base "http"
#define BASE_0_MQ_10BIT				0x1b0		//< First 10 bits of base "0-mq"


#define MAX_FILE_OR_URL_SIZE		1824		///< Used inside an ExtraLocator, it makes the structure 2 Kbytes.

/// HttpQueryState apply values (on state == PSTATE_COMPLETE_OK)

#define APPLY_NOTHING					 0		///< Just an l_value with {///node}//base/entity or {///node}//base/entity/key
#define APPLY_NAME						 1		///< //base/entity/key:name (Select an item form a Tuple by name)
#define APPLY_URL						 2		///< //base# any_url_encoded_url ; (A call to http or file)
#define APPLY_FUNCTION					 3		///< {///node}//base/entity/key(//r_base/r_entity/r_key) (A function call on a block.)
#define APPLY_FUNCT_CONST				 4		///< //base/entity/key(# any_url_encoded_const ;) (A function call on a const.)
#define APPLY_FILTER					 5		///< {///node}//base/entity/key[//r_base/r_entity/r_key] (A filter on a block.)
#define APPLY_FILT_CONST				 6		///< //base/entity/key[# any_url_encoded_const ;] (A filter on a const.)
#define APPLY_RAW						 7		///< {///node}//base/entity/key.raw (Serialize text to raw.)
#define APPLY_TEXT						 8		///< {///node}//base/entity/key.text (Serialize raw to text.)
#define APPLY_ASSIGN					 9		///< {///node}//base/entity/key=//r_base/r_entity/r_key (Assign block to block.)
#define APPLY_ASSIGN_CONST				10		///< //base/entity/key=# any_url_encoded_const ; (Assign const to block.)
#define APPLY_JAZZ_INFO					11		///< /// Show the server info.
#define APPLY_NEW_ENTITY				12		///< {///node}//base/entity.new (Create a new entity)
#define APPLY_GET_ATTRIBUTE				13		///< {///node}//base/entity/key.attribute(123) (read attribute 123 with HTTP_GET)
#define APPLY_SET_ATTRIBUTE				14		///< //base/entity/key.attribute(123)=# url_encoded ; (set attribute 123 with HTTP_GET)


/// A map for defining http config names
typedef std::map<int, std::string>	MapIS;


/// A structure holding connections.
typedef std::map<std::string, Index> ConnMap;


/// A map for defining http config ports
typedef std::map<int, int>	MapII;


/*! \brief A proper type for specifying http status codes

Before libmicrohttpd (somewhere between > 0.9.66-1 and <= 0.9.72-2) changed MHD_Result to an enum, MHD_Result was (improperly) used
to define HTTP responses. That ended-up badly on newer versions, since it was passed to a MHD_queue_response() and stopped working
as it became an enum.

This triggeresd the need, for clarity reasons only, to introduce a new type, MHD_StatusCode to refer to **HTTP responses**.

See: https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

*/
typedef unsigned int MHD_StatusCode;


/** \brief Channels: A Container doing block transactions across media (files, folders, shell, urls, other Containers, ..)

ExtraLocators
-------------

This Container extends the short (80 bytes) Locator structure with an extra (2 Kbytes) ExtraLocator. The structures are owned by Channels.
When you use the "easy" interface, you can ignore this. When you parse using as_locator() and use the Native API, notice that the Locator
returned is linked to an ExtraLocator and **must** be used (by the native API) or disposed (by destroy_extra_locator()).

*/
class Channels : public Container {

	public:

		Channels(pLogger	 a_logger,
				 pConfigFile a_config);
	   ~Channels();

		StatusCode	   start	   ();
		StatusCode	   shut_down   ();

		StatusCode	   get		   (pTransaction		&p_txn,
									pChar				 p_what);
		StatusCode	   get		   (pTransaction		&p_txn,
									pChar				 p_what,
									pBlock				 p_row_filter);
		StatusCode	   get		   (pTransaction		&p_txn,
									pChar				 p_what,
									pChar				 name);
		StatusCode	   locate	   (Locator				&location,
									pChar				 p_what);
		StatusCode	   header	   (StaticBlockHeader	&hea,
									pChar				 p_what);
		StatusCode	   header	   (pTransaction		&p_txn,
									pChar				 p_what);
		StatusCode	   put		   (pChar				 p_where,
									pBlock				 p_block,
									int					 mode = WRITE_EVERYTHING);
		StatusCode	   new_entity  (pChar				 p_where);
		StatusCode	   remove	   (pChar				 p_where);
		StatusCode	   copy		   (pChar				 p_where,
									pChar				 p_what);
		StatusCode	   translate   (pTuple				 p_tuple,
									pChar				 p_pipe);
		MHD_StatusCode forward_get (pTransaction		&p_txn,
									Name				 node,
									pChar				 p_url,
									int					 apply);
		MHD_StatusCode forward_put (Name				 node,
									pChar				 p_url,
									pBlock				 p_block);
		MHD_StatusCode forward_del (Name				 node,
									pChar				 p_url);

		// Support for container names in the API .base_names()

		void base_names(BaseNames &base_names);

		// Public config variables

		MapIS jazz_node_name = {};
		MapIS jazz_node_ip   = {};
		MapII jazz_node_port = {};

		int jazz_node_my_index	   = -1;
		int jazz_node_cluster_size =  0;

		std::string filesystem_root = {};

#ifndef CATCH_TEST
	private:
#endif

		int can_curl = false, curl_ok = false;
		int can_zmq  = false, zmq_ok  = false;
		int can_bash = false;
		int file_lev = 0;

		Index	pipes	= {};
		ConnMap connect = {};

		void *zmq_context	= nullptr;
		void *zmq_requester = nullptr;
};
typedef Channels *pChannels;

// --------------------------------------------------------------------------------------------------------------------------------------------

		// StatusCode url_put	   ();
		// StatusCode url_get	   ();
		// StatusCode url_delete  ();

		// The shell

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po

// #include <cstdio>
// #include <iostream>
// #include <memory>
// #include <stdexcept>
// #include <string>
// #include <array>

// std::string exec(const char* cmd) {
//     std::array<char, 128> buffer;
//     std::string result;
//     std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
//     if (!pipe) {
//         throw std::runtime_error("popen() failed!");
//     }
//     while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
//         result += buffer.data();
//     }
//     return result;
// }

		// StatusCode shell_exec  ();

#ifdef CATCH_TEST

// Instancing Channels
// -------------------

extern Channels CHN;

#endif

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL

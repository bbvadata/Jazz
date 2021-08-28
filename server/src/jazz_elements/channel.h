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

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL
#define INCLUDED_JAZZ_ELEMENTS_CHANNEL


namespace jazz_elements
{

#define MAX_FILE_OR_URL_SIZE		1824		///< Used inside an ExtraLocator, it makes the structure 2 Kbytes.


/// HttpQueryState apply values (on state == PSTATE_COMPLETE_OK)

#define APPLY_NOTHING					 0		///< Just an r_value with {///node}//base/entity or {///node}//base/entity/key
#define APPLY_NAME						 1		///< //base/entity/key:name (Select an item form a Tuple by name)
#define APPLY_URL						 2		///< {///node}//base# any_url_encoded_url ; (A call to http or file)
#define APPLY_FUNCTION					 3		///< {///node}//base/entity/key(//r_base/r_entity/r_key) (A function call on a block.)
#define APPLY_FUNCT_CONST				 4		///< {///node}//base/entity/key(# any_url_encoded_const ;) (A function call on a const.)
#define APPLY_FILTER					 5		///< {///node}//base/entity/key[//r_base/r_entity/r_key] (A filter on a block.)
#define APPLY_FILT_CONST				 6		///< {///node}//base/entity/key[# any_url_encoded_const ;] (A filter on a const.)
#define APPLY_RAW						 7		///< {///node}//base/entity/key.raw (Serialize text to raw.)
#define APPLY_TEXT						 8		///< {///node}//base/entity/key.text (Serialize raw to text.)
#define APPLY_ASSIGN					 9		///< {///node}//base/entity/key=//r_base/r_entity/r_key (Assign block to block.)
#define APPLY_ASSIGN_CONST				10		///< {///node}//base/entity/key=# any_url_encoded_const ; (Assign const to block.)
#define APPLY_JAZZ_INFO					11		///< /// Show the server info.
#define APPLY_NEW_ENTITY				12		///< {///node}//base/entity.new (Create a new entity)


/** \brief ExtraLocator: A structure that replaces the entity/key in a Locator by a long URL or file name and some http quirks.

**Valid characters**: Most non-zero characters should be usable or will be rejected by the endpoint, not the parser. See the doc
on Channels::as_locator for details.
*/
struct ExtraLocator {
	Name	user_name;							///< An optional CURLOPT_USERNAME for http calls
	Name	user_pw;							///< An optional CURLOPT_USERNAME for http calls
	Locator	cookie_file;						///< An optional CURLOPT_USERNAME for http calls
	Locator	cookie_jar;							///< An optional CURLOPT_USERNAME for http calls
	char	url[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)
};


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

		Channels (pLogger	  a_logger,
				  pConfigFile a_config);

		StatusCode start	 ();
		StatusCode shut_down ();

		// The easy interface (Requires explicit pulling because of the native interface using the same names.)

		using Container::get;
		using Container::header;
		using Container::put;
		using Container::new_entity;
		using Container::remove;
		using Container::copy;

		// The parser: This overrides the parser in Channels.

		virtual StatusCode as_locator  (Locator			   &result,
										pChar				p_what);

		// The "native" interface

		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what,
										pBlock				p_row_filter);
		virtual StatusCode get		   (pTransaction	   &p_txn,
							  			Locator			   &what,
							  			pChar				name);
		virtual StatusCode header	   (StaticBlockHeader  &hea,
										Locator			   &what);
		virtual StatusCode header	   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode put		   (Locator			   &where,
										pBlock				p_block,
										int					mode = WRITE_ALWAYS_COMPLETE);
		virtual StatusCode new_entity  (Locator			   &where);
		virtual StatusCode remove	   (Locator			   &where);
		virtual StatusCode copy		   (Locator			   &where,
										Locator			   &what);

		MHD_StatusCode	   forward	   (Name				node,
										pChar				p_url,
										int					method);

		// Support for container names in the API .base_names()

		void base_names (BaseNames &base_names);

		// Public config variables

		IndexIS jazz_node_name = {};
		IndexIS jazz_node_ip   = {};
		IndexII jazz_node_port = {};

		int jazz_node_my_index	   = -1;
		int jazz_node_cluster_size =  0;

		std::string filesystem_root = {};

#ifndef CATCH_TEST
	protected:
#endif

		inline void destroy_extra_locator(Locator &loc) {
			if (loc.p_extra != nullptr) {
				alloc_bytes -= sizeof(ExtraLocator);
				free(loc.p_extra);
			}
			loc.p_extra = nullptr;
		}

};
typedef Channels *pChannels;


		// Libcurl
// --------------------------------------------------------------------------------------------------------------------------------------------
// https://curl.se/libcurl/c/libcurl.html

// The basic rule for constructing a program that uses libcurl is this: Call curl_global_init, with a CURL_GLOBAL_ALL argument, immediately after the program starts, while it is still only one thread and before it uses libcurl at all. Call curl_global_cleanup immediately before the program exits, when the program is again only one thread and after its last use of libcurl.

// You can call both of these multiple times, as long as all calls meet these requirements and the number of calls to each is the same.

// It isn't actually required that the functions be called at the beginning and end of the program -- that's just usually the easiest way to do it. It is required that the functions be called when no other thread in the program is running.

// These global constant functions are not thread safe, so you must not call them when any other thread in the program is running. It isn't good enough that no other thread is using libcurl at the time, because these functions internally call similar functions of other libraries, and those functions are similarly thread-unsafe. You can't generally know what these libraries are, or whether other threads are using them.

// The global constant situation merits special consideration when the code you are writing to use libcurl is not the main program, but rather a modular piece of a program, e.g. another library. As a module, your code doesn't know about other parts of the program -- it doesn't know whether they use libcurl or not. And its code doesn't necessarily run at the start and end of the whole program.

// A module like this must have global constant functions of its own, just like curl_global_init and curl_global_cleanup. The module thus has control at the beginning and end of the program and has a place to call the libcurl functions. Note that if multiple modules in the program use libcurl, they all will separately call the libcurl functions, and that's OK because only the first curl_global_init and the last curl_global_cleanup in a program change anything. (libcurl uses a reference count in static memory).

// In a C++ module, it is common to deal with the global constant situation by defining a special class that represents the global constant environment of the module. A program always has exactly one object of the class, in static storage. That way, the program automatically calls the constructor of the object as the program starts up and the destructor as it terminates. As the author of this libcurl-using module, you can make the constructor call curl_global_init and the destructor call curl_global_cleanup and satisfy libcurl's requirements without your user having to think about it. (Caveat: If you are initializing libcurl from a Windows DLL you should not initialize it from DllMain or a static initializer because Windows holds the loader lock during that time and it could cause a deadlock.)

// curl_global_init has an argument that tells what particular parts of the global constant environment to set up. In order to successfully use any value except CURL_GLOBAL_ALL (which says to set up the whole thing), you must have specific knowledge of internal workings of libcurl and all other parts of the program of which it is part.

// A special part of the global constant environment is the identity of the memory allocator. curl_global_init selects the system default memory allocator, but you can use curl_global_init_mem to supply one of your own. However, there is no way to use curl_global_init_mem in a modular program -- all modules in the program that might use libcurl would have to agree on one allocator.

// There is a failsafe in libcurl that makes it usable in simple situations without you having to worry about the global constant environment at all: curl_easy_init sets up the environment itself if it hasn't been done yet. The resources it acquires to do so get released by the operating system automatically when the program exits.

// This failsafe feature exists mainly for backward compatibility because there was a time when the global functions didn't exist. Because it is sufficient only in the simplest of programs, it is not recommended for any program to rely on it.

// --------------------------------------------------------------------------------------------------------------------------------------------
// https://curl.se/libcurl/c/libcurl-tutorial.html

// The name might make it seem that the multi interface is for multi-threaded programs, but the truth is almost the reverse. The multi interface allows a single-threaded application to perform the same kinds of multiple, simultaneous transfers that multi-threaded programs can perform. It allows many of the benefits of multi-threaded transfers without the complexity of managing and synchronizing many threads.

// To use this interface, you are better off if you first understand the basics of how to use the easy interface. The multi interface is simply a way to make multiple transfers at the same time by adding up multiple easy handles into a "multi stack".

// --------------------------------------------------------------------------------------------------------------------------------------------
// https://curl.se/libcurl/c/threadsafe.html

// libcurl is thread safe but has no internal thread synchronization. You may have to provide your own locking should you meet any of the thread safety exceptions below.

// Handles. You must never share the same handle in multiple threads. You can pass the handles around among threads, but you must never use a single handle from more than one thread at any given time.

// Shared objects. You can share certain data between multiple handles by using the share interface but you must provide your own locking and set curl_share_setopt CURLSHOPT_LOCKFUNC and CURLSHOPT_UNLOCKFUNC.

// --------------------------------------------------------------------------------------------------------------------------------------------
// https://curl.se/libcurl/c/example.html

// libcurl - small example snippets

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

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL

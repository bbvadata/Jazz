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

/// Different values for Block.cell_type
#define CHANNEL__

// Anything -> Block

#define CHANNEL_FILE_READ			0x0001		///< A channel reading from a file into a Tensor of byte
#define CHANNEL_FOLDER_READ			0x0002		///< A channel reading from a folder into a Tensor of strings
#define CHANNEL_DISK_READ			0x0003		///< A channel reading from a file path either as a CHANNEL_FILE_RD or a CHANNEL_FOLDER_RD
#define CHANNEL_URL_GET				0x0004		///< A channel reading via a libcurl GET into a Tensor
#define CHANNEL_URL_BLOCK_GET		0x0008		///< A channel reading (from Jazz) via a libcurl GET a complete Block, Kind or Tuple
#define CHANNEL_INDEX_II_SAVE		0x0010		///< A channel serializing an IndexII into a Tuple of (int, int)
#define CHANNEL_INDEX_IS_SAVE		0x0020		///< A channel serializing an IndexIS into a Tuple of (int, string)
#define CHANNEL_INDEX_SI_SAVE		0x0040		///< A channel serializing an IndexSI into a Tuple of (string, int)
#define CHANNEL_INDEX_SS_SAVE		0x0080		///< A channel serializing an IndexSS into a Tuple of (string, string)
#define CHANNEL_CONTAINER_GET		0x0100		///< A channel reading any Block, Kind or Tuple from a Container or descendant
#define CHANNEL_SHELL_EXEC			0x0200		///< A channel reading the output of a shell command into a Tensor of strings

// Block -> Anything

#define CHANNEL_FILE_WRITE			0x1001		///< A channel writing the tensor inside a Tensor to a binary file
#define CHANNEL_URL_PUT				0x1004		///< A channel writing the tensor inside a Tensor via a libcurl PUT
#define CHANNEL_URL_BLOCK_PUT		0x1008		///< A channel writing any complete Block, Kind or Tuple via a libcurl PUT to a Jazz server
#define CHANNEL_INDEX_II_LOAD		0x1010		///< A channel serializing a Tuple of (int, int) into an IndexII
#define CHANNEL_INDEX_IS_LOAD		0x1020		///< A channel serializing a Tuple of (int, string) into an IndexIS
#define CHANNEL_INDEX_SI_LOAD		0x1040		///< A channel serializing a Tuple of (string, int) into an IndexSI
#define CHANNEL_INDEX_SS_LOAD		0x1080		///< A channel serializing a Tuple of (string, string) into an IndexSS
#define CHANNEL_CONTAINER_PUT		0x1100		///< A channel writing any Block, Kind or Tuple from any Container or descendant

// Delete anything

#define CHANNEL_FILE_UNLINK			0x2001		///< A channel deleting a file
#define CHANNEL_FOLDER_REMOVE_ALL	0x2002		///< A channel deleting a folder with anything in it
#define CHANNEL_URL_DELETE			0x2004		///< A channel deleting via a libcurl DELETE
#define CHANNEL_CONTAINER_REMOVE	0x2100		///< A channel deleting any Block, Kind or Tuple inside a Container or descendant


/** \brief Channels: A Service to manage Channel objects.

*/
class Channels : public Container {

	public:

		Channels (pLogger	  a_logger,
				  pConfigFile a_config);

		StatusCode start	 ();
		StatusCode shut_down ();

		void base_names (BaseNames &base_names);
};
typedef Channels *pChannels;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL

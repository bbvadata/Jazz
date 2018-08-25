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

//TODO: Integrate project building with https://opencv.org/
//TODO: Integrate image classes from https://docs.opencv.org/master/de/d7a/tutorial_table_of_content_core.html


/* NOTES from the Jazz draft description including tasks for this
   --------------------------------------------------------------

//TODO: JazzFileSystem is an abstraction to manage files in the Linux box running Jazz as JazzBlocks without necessarily copying their
		content.
//TODO: JazzRemoteSource is an http client connecting to a REST API and storing the result as a block. The connection can be initiated
		periodically or on demand. The result is typically filtered by a validation function written in Bebop.

//TODO: JazzClusterFileSystem is a JazzFileSystem interface that uses a DenseMapping to shard the file names across a cluster.

*/

#include "src/jazz_functional/jazz_cluster.h"


#ifndef INCLUDED_JAZZ_FUNCTIONAL_FILESYSTEM
#define INCLUDED_JAZZ_FUNCTIONAL_FILESYSTEM


/**< \brief TODO

//TODO: Write module description for jazz_filesystem when implemented.
*/
namespace jazz_filesystem
{

using namespace jazz_containers;


//TODO: Document interface for module jazz_filesystem.

//TODO: Implement interface for module jazz_filesystem.


/**
//TODO: Write the JazzFileSystem description
*/
class JazzFileSystem: public JazzTree {

	public:
		 JazzFileSystem(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzFileSystem();
};


/**
//TODO: Write the JazzClusterFileSystem description
*/
class JazzClusterFileSystem: public JazzFileSystem {

	public:
		 JazzClusterFileSystem(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzClusterFileSystem();
};


/**
//TODO: Write the JazzRemoteSource description
*/
class JazzRemoteSource: public JazzBlockKeepr {

	public:
		 JazzRemoteSource(jazz_utils::pJazzLogger a_logger = nullptr);
		~JazzRemoteSource();
};


}

#endif

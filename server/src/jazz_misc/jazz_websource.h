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

/*! \brief Root class providing all the necessary web resource functionality.

This class keeps a dictionary of known urls as keys, the values include: block name Content-Length, Content-Language and
Last-Modified. The source name is always "www". These dictionaries are stored in persistence.

The urls and all their attributes are stored in URLattrib structures. An abstract entity known as a websource organizes the links in the persistence
in a way that a whole websource can be queried, created, deleted. websources are identified by name (websourceName) or index in a in internal vector
queried via find_websource(). websources do no exist for the API, they are just a clean way to manage sets of urls that are created/removed together.
*/

#include "src/include/jazz01_commons.h"
#include "src/jazz_blocks/jazz_blocks.h"
#include "src/jazz_main/jazz_instances.h"

/*~ end of automatic header ~*/

#ifndef jzz_IG_JAZZWEBSOURCE
#define jzz_IG_JAZZWEBSOURCE


/** Struct sizes
*/
#define MAX_URL_LENGTH		215		///< Maximum length of a URL in the www API. (This makes the URLattrib size = 256
#define MAX_NUM_WEBSOURCES	100		///< The maximum number of web sources. Limited to avoid buffer overflow in GET function.

typedef persistedKey websourceName;


/** A row in the url dictionary
*/
struct URLattrib
{
	char		 url[MAX_URL_LENGTH + 1];	///< The url
	persistedKey block;						///< The key identifying the resource. The resource is persisted as source="www", key=key.
	persistedKey websource;					///< The key identifying the resource. The resource is persisted as source="www", key=key.
	int			 blocktype;					///< The block type of the resource. BLOCKTYPE_RAW_MIME_*
	int			 language;					///< The language as an index to HTTP_LANGUAGE_STRING (consts like LANG_EN_US are valid too.)
};


/** A persistence block storing dictionary entries. The length parameter in the header defines the number of rows.
*/
struct block_URLdictionary : jzzBlockHeader
{
	URLattrib attr[];		///< The array of block_URLdictionary
};


class jazzWebSource: public jazzService {

	public:

		 jazzWebSource();
		~jazzWebSource();

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();

// Functions to manage websources.

		bool open_all_websources  ();
		bool new_websource		  (const char * websource);
		bool kill_websource		  (const char * websource);
		bool set_url_to_block	  (const char * url,  const char * block, const char * websource);
		bool set_mime_to_block	  (int type,		  const char * block, const char * websource);
		bool set_lang_to_block	  (const char * lang, const char * block, const char * websource);
		bool close_all_websources ();

// Function for the API.

		bool get_url (const char * url, URLattrib &uattr);

	protected:

		map<string, int> sources;			// websource -> 1 (0 when deleted)

	private:

		typedef jazzService super;

		map<string, string> url_block;		// url	  -> block
		map<string, int>	block_mime;		// block  -> mime type
		map<string, int>	block_lang;		// block  -> language
		map<string, string> block_source;	// block  -> websource
};

#endif

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

#include <chrono>
#include <iostream>
#include <map>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

/**< \brief Basic JAZZ types and definitions.

	This file has things as all the declarations. There are too many method declared to be implemented in a single file. jazzCommond.cpp  only
implements trivial constructors/destructors + root methods of start()/stop()/reload() + Some cluster configuration parsing. The rest are implemented
in the appropriate source files in: jzzMISCUTILS, jzzALLOC, jzzCONFIG, jzzACCESS, jzzTHREADS, jzzLOGGER and jzzHTTP.
*/

#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header
#include "microhttpd.h"

#if defined CATCH_TEST

	#ifndef jzz_IG_CATCH
	#define jzz_IG_CATCH

		#include "src/catch2/catch.hpp"

	#endif

#endif

/*~ end of automatic header ~*/

#include <math.h>

#include "src/include/jazz_platform.h"

#ifndef jzz_IG_JAZZCOMMONS
#define jzz_IG_JAZZCOMMONS


/** The trace levels for log()
*/
#define LOG__

	/// Just for checking, normally not logged, should not exist in case of NDEBUG. In that case, it becomes a LOG_WARN to force removing it.
#define LOG_DEBUG				1
	/// A good, non trivial, non frequent event to discard trouble. E.g., "Jazz successfully installed on host xxx", "backup completed."
#define LOG_INFO				2
	/// A function returned an error status. This may still be normal. E.g., "configuration key xxx cannot be converted to integer."
#define LOG_MISS				3
	/// A warning. More serious than the previous. Should not happen. It is desirable to treat the existence of a warning as a bug.
#define LOG_WARN				4
	/// Something known to be a requisite is failing. The program or task halts due to this.
#define LOG_ERROR				5


/** Perimetral security modes.

In any case, writing requires a signed PUT if MHD_ENABLE_ROLE_CREATOR & (MHD_ENABLE_ROLE_DATAWRITER | MHD_ENABLE_ROLE_FUNCTIONWRITER) regardless
of the perimetral security level established.
*/
#define MODE__

	/// There is either no perimetral security or the nodes are "physically" (typically via a firewall) separated from the clients.
#define MODE_SAFE_IF_PHYSICAL	0
	/// The nodes are only accessible to a list of IP ranges (typically, running jzzPERIMETRAL only instances).
#define MODE_TRUSTED_ONLY		1
	/// The nodes are both working nodes and perimeters checking all the API calls through jzzPERIMETRAL.
#define MODE_INTERNAL_PERIMETER 2
	/// This node is only a perimeter node and communicates with workers that are either MODE_SAFE_IF_PHYSICAL or MODE_TRUSTED_ONLY.
#define MODE_EXTERNAL_PERIMETER 3


/** The node roles. What a node can or must do in mask 0x00ff, the state a node is in mask 0xff00.

	If JAZZ_NODE_MIN has the bit set, the node MUST have that role working.
	If JAZZ_NODE_MAX has the bit set, the node CAN be used to play that role.
*/
#define ROLE__
//Low 8
#define ROLE_PERSISTENCE		0x0001	///< The node implements blocks and LMDB persistence.
#define ROLE_FUNCTION_WORKER	0x0002	///< The node implements the basic functional part. (R and RAMQ are independent.)
#define ROLE_HAS_RLANGUAGE		0x0004	///< The node also implements R. (Requires ROLE_FUNCTION_WORKER being set.)
#define ROLE_HAS_RAMQ			0x0008	///< The node caches the results of functions using RAMQ. (Requires ROLE_FUNCTION_WORKER being set.)
#define ROLE_PERIMETRAL			0x0010	///< The node implements perimetral. (If appropriately configured in section JazzHTTPSERVER.)
#define ROLE_FOREMAN			0x0020	///< The node is the foreman.
//High 8
#define ROLE_WORKING_OK			0x0000	///< The node is working properly.
#define ROLE_NOT_INSTALLED		0x0100	///< The node is part of the cluster, accessible via ssh, but Jazz is not installed yet.
#define ROLE_INSTALL_BUSY		0x0200	///< Jazz is currently being installed.
#define ROLE_INSTALL_FAILED		0x0400	///< Jazz installation failed. No automatic recovery other than retry after some delay.
#define ROLE_NOT_STARTED		0x0800	///< The node is part of the cluster, accessible via ssh, with Jazz installed, but not started yet.
#define ROLE_START_BUSY			0x1000	///< Jazz is currently being started.
#define ROLE_START_FAILED		0x2000	///< Jazz starting failed. No automatic recovery other than retry after some delay.
#define ROLE_IN_ERROR			0x4000	///< The node is unavailable or reporting failure. Typically combines with ROLE_DISABLED.
#define ROLE_DISABLED			0x8000	///< The node was disabled. Typically due to being ROLE_IN_ERROR at some moment.


/** Data types of BLOCKS

	All block types supported by Jazz (See the description on each type and the notes here for details.)

	NOTES:
	------

	\b Binary \b compatibility: When said to be binary compatible, it only means allocation independent data. Specifically, it is compatible with
	the result of R's serialize(*, NULL) excluding the trailing 14 bytes.
	For example, a LGLSXP x would be returned by R's serialize(x, NULL) as:
	X	 \n		|- format version 2 -|	|- writer = R 3.2.5 -|	|- min read R 2.3.0 -|	|-	 LGLSXP == 10	-|	...
	0x58, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x02, 0x05, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0a, ...
	will be stored as:
	|-	 LGLSXP == 10	-|	...
	0x00, 0x00, 0x00, 0x0a, ...
	The Simple types (LGLSXP, INTSXP, REALSXP, RAWSXP) are straightforward. Simply add the trailing constant 14 bytes, unserialize() and you have
	an R object.
	In the case of STRSXP, The R object is a SEXP followed by a block of pointers (SEXPs pointing to CHARSXPs). What we store are the strings,
	and not the pointers, obviously.

	\b Factors and \b grades: The blocks contain integers only as INTSXP, just like a BLOCKTYPE_INTEGER and treated internally the same. At mesh
	level, some attributes define labels corresponding to each value. The values should be correlative starting at 0 and can be later converted
	into R factors combined with their labels. The only difference between them is BLOCKTYPE_GRADE is sorted and BLOCKTYPE_FACTOR unsorted, for
	whoever that makes sense (plots, regressions, etc.).

	\b Instrumental \b types: Are the types in [BLOCKTYPE_C_BOOL..BLOCKTYPE_C_R_REAL] that may be part of instruments. All other types only support
	block level.

	\b Mime \b types: All types will be returned with mime type "application/octet-stream", except BLOCKTYPE_RAW_MIME_* that will be returned with
	a specific mime type.

	\b Strings: Strings are stored as a BLOCKTYPE_C_OFFS_CHARS, NOT compatible with R. It is more efficient in two ways (storage, because strings
	are reused, a block of 50K NAs is stored in one byte (a single zero) a block of 50K with a cardinality of 10 can be stored in tenths of bytes)
	and more importantly, the offset part of a BLOCKTYPE_C_OFFS_CHARS is a vector of integer and can be applied filtering, union, etc. without the
	need to scan it. An R block of strings is not persisted, just is returned via jzzROM, created on demand to create an R vector of character by
	just calling unserialize() over it.

	\b Time: Time is store in double a Unix time (also known as POSIX time or Epoch time) is a system for describing instants in time, defined as
	the number of seconds that have elapsed since 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970, not counting leap seconds.

	\b System \b blocks: Are blocks that are persisted in the SYS source. BLOCKTYPE_ALL_* are identical in all nodes and are forced to be written
	by the foreman with no other writing mechanism. BLOCKTYPE_MY_* are written by each tode to itself and usually are logging events, security
	events and profiling statistics.
*/
#define BLOCKTYPE__
//Instrumental (All have the same Mime type: "application/octet-stream")
#define BLOCKTYPE_C_BOOL		 1		///< An array of boolean (1 byte per value) not directly R compatible.
#define BLOCKTYPE_C_OFFS_CHARS	 2		///< The internal storage of strings. (See above)
#define BLOCKTYPE_C_FACTOR		 3		///< An unsorted categorical variable. (See more above.)
#define BLOCKTYPE_C_GRADE		 4		///< A sorted categorical variable. (See more above.)
#define BLOCKTYPE_C_INTEGER		 5		///< A vector of integer. Binary compatible(*see above*) with an R INTSXP (vector of integer).
#define BLOCKTYPE_C_TIMESEC		 6		///< A vector of time points stored as doubles. (See more above.)
#define BLOCKTYPE_C_REAL		 7		///< A vector of floating point numbers. Binary compatible with an R REALSXP (vector of numeric).
//Raw types (Have individual Mime types see each description.)
#define BLOCKTYPE_RAW_ANYTHING	10		///< Anything other than a specific BLOCKTYPE_RAW_*. Returned with Mime type "application/octet-stream"
#define BLOCKTYPE_RAW_STRINGS	11		///< Strings up/down-loaded for conversion to/from data types. Returned with Mime type "text/plain"
#define BLOCKTYPE_RAW_R_RAW		12		///< Anything R serialized to RAWSXP with serialize(x, NULL). Mime type: "application/octet-stream"
#define BLOCKTYPE_RAW_MIME_APK	13		///< An Android application. Mime type: "application/vnd.android.package-archive"
#define BLOCKTYPE_RAW_MIME_CSS	14		///< A CSS style sheet. Mime type: "text/css"
#define BLOCKTYPE_RAW_MIME_CSV	15		///< A comma separated file with one row of strings "as is", no escapes or quotes. Mime type: "text/csv"
#define BLOCKTYPE_RAW_MIME_GIF	16		///< An image. Mime type: "image/gif"
#define BLOCKTYPE_RAW_MIME_HTML	17		///< An html object. Mime type: "text/html"
#define BLOCKTYPE_RAW_MIME_IPA	18		///< An iOS application. Mime type: "application/octet-stream"
#define BLOCKTYPE_RAW_MIME_JPG	19		///< An image. Mime type: "image/jpeg"
#define BLOCKTYPE_RAW_MIME_JS	20		///< A JS script. Mime type: "application/javascript"
#define BLOCKTYPE_RAW_MIME_JSON	21		///< A JSON object. Mime type: "application/json"
#define BLOCKTYPE_RAW_MIME_MP4	22		///< A video. Mime type: "video/mp4"
#define BLOCKTYPE_RAW_MIME_PDF	23		///< A PDF file. Mime type: "application/pdf"
#define BLOCKTYPE_RAW_MIME_PNG	24		///< An image. Mime type: "image/png"
#define BLOCKTYPE_RAW_MIME_TSV	25		///< A tab separated file of strings, no escapes or quotes. Mime type: "text/tab-separated-values"
#define BLOCKTYPE_RAW_MIME_TXT	26		///< Any text file. All sources (R, Python, SQL, ...) use this type. Mime type: "text/plain"
#define BLOCKTYPE_RAW_MIME_XAP	27		///< A Windows Phone application. Mime type: "application/x-silverlight-app"
#define BLOCKTYPE_RAW_MIME_XML	28		///< An XML file. Mime type: "application/xml"
#define BLOCKTYPE_RAW_MIME_ICO	29		///< An ICO file, aka "image/vnd.microsoft.icon". Mime type: "image/x-icon"
//Functions (Have individual Mime types see each description.)
#define BLOCKTYPE_FUN_R			31		///< An R function. Mime type: "text/plain"
#define BLOCKTYPE_FUN_DFQL		32		///< A function defining a data frame. Data Frame Query Language. Mime type: "text/plain"
#define BLOCKTYPE_FUN_DFQO		33		///< Bytecode for a DFQL. Mime type: "application/jazz-vm"
//System blocks (All have the same Mime type: "application/jazz-sys")
#define BLOCKTYPE_ALL_ROLES		41		///< The working image of the cluster with all official roles and states.
#define BLOCKTYPE_ALL_KEYSPACE	42		///< The division of the key space across the cluster. Nodes store history as division changes.
#define BLOCKTYPE_ALL_SECURITY	43		///< Global security rules, grant revocation, etc. applicable to all.
#define BLOCKTYPE_MY_LOG_EVENTS	44		///< A node's logger events.
#define BLOCKTYPE_MY_PROFILING	45		///< A node's profiling, access and computing statistics.
#define BLOCKTYPE_MY_SECURITY	46		///< A node's security relevant information.
#define BLOCKTYPE_MY_URL_DICT	47		///< A node's table of valid URLs that point to resources stored as blocks.
#define BLOCKTYPE_SOURCE_ATTRIB	48		///< Attributes of a source. All sources have this block with key "."

#define BLOCKTYPE_LAST_ BLOCKTYPE_SOURCE_ATTRIB

#define MAX_KEY_LENGTH			16		///< The key length is 15 base 64 chars (90 bits) for both keys and source names

/** A base64 encoded key to identify a block inside a source.
*/
struct persistedKey {char key[MAX_KEY_LENGTH];};

/** Types of RAM allocation for jazzAlloc()/jazzFree()

	All Jazz RAM allocation must use the appropriate type to support tracking of RAM activity and failure.

	See the doc of jazzAlloc() for details.

	Always use the macros JAZZALLOC() and JAZZFREE(). Never call the functions jazzAlloc()/jazzFree() directly!

	\b For \b anything \b but \b blocks: RAM_ALLOC_SERVICE .. RAM_ALLOC_RESPONSE

	\b For \b blocks \b not \b managed \b by \b R: (Typically: Storage and querying at block and instrument level.)
	RAM_ALLOC_C_C_BOOL .. RAM_ALLOC_C_RAW

	\b For \b blocks \b managed \b by \b R: (Typically: function evaluation.) RAM_ALLOC_R_LGLSXP .. RAM_ALLOC_R_RAWSXP
*/
#define RAM_ALLOC__
//Anything but blocks:
#define RAM_ALLOC_SERVICE		 1		///< RAM allocation for as long as the service (e.g., RAMQ) is running. (E.g., buffers.)
#define RAM_ALLOC_RESPONSE		 2		///< RAM allocation inside an http response, like a (non R) function evaluation.
//Jazz blocks:
#define RAM_ALLOC_C_BOOL		11		///< Assign for a block of type BLOCKTYPE_C_BOOL
#define RAM_ALLOC_C_OFFS_CHARS	12		///< Assign for a block of type BLOCKTYPE_C_OFFS_CHARS
#define RAM_ALLOC_C_INTEGER		13		///< Assign for a block of types BLOCKTYPE_R_FACTOR, BLOCKTYPE_R_GRADE and BLOCKTYPE_R_INTEGER
#define RAM_ALLOC_C_REAL		14		///< Assign for a block of types BLOCKTYPE_R_TIMESEC and BLOCKTYPE_R_REAL
#define RAM_ALLOC_C_RAW			15		///< Assign for any block of type in {BLOCKTYPE_RAW_GENERIC..BLOCKTYPE_RAW_MIME_XML}

///RAM tracking fun values:
#define RAM_TRACK_

#define RAM_TRACK_LEARN			1		///< Learn a new (atpt, typ) and set it as allocated.
#define RAM_TRACK_CHECK			2		///< Check if atpt is known, has the same type and is still allocated.

#define RAM_TRACK_WAS_CHECKED	0x10000	///< Flag the learned type as already checked (meaning closed if only JAZZFREE() macro is used).

#ifdef NDEBUG
	/// Allocate any object except an R string element (CHARSXP)
	#define JAZZALLOCVEC(atpt, typ, siz) jazzAlloc((void * &)atpt, typ, siz)

	/// Allocate an R string element RAM_ALLOC_R_CHARSXP
	#define JAZZALLOCSTR(atpt, str) jazzAlloc((void * &)atpt, RAM_ALLOC_R_CHARSXP, 0, (char *) str)

	/// This is the ONLY freeing method used!! It forces a previous check, except in RELEASE.
	#define JAZZFREE(atpt, typ) (jazzFree((void * &)atpt, typ))

	#define AUTOTYPEBLOCK(atpt) (RAM_ALLOC_RESPONSE)
#else
	/// Allocate any object except an R string element (CHARSXP)
	#define JAZZALLOCVEC(atpt, typ, siz) jazzAlloc((void * &)atpt, typ, siz); jazzPtrTrack((void * &)atpt, typ, RAM_TRACK_LEARN)

	/// Allocate an R string element RAM_ALLOC_R_CHARSXP
	#define JAZZALLOCSTR(atpt, str) jazzAlloc((void * &)atpt, RAM_ALLOC_R_CHARSXP, 0, (char *) str); \
		jazzPtrTrack((void * &)atpt, RAM_ALLOC_R_CHARSXP, RAM_TRACK_LEARN)

	/// This is the ONLY freeing method used!! It forces a previous check, except in RELEASE.
	#define JAZZFREE(atpt, typ) jazzPtrTrack((void * &)atpt, typ, RAM_TRACK_CHECK); jazzFree((void * &)atpt, typ)

	#define AUTOTYPEBLOCK(atpt) (auto_type_block(atpt))
#endif

/// Select a macro depending on the number of params
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME

/// This macro expands to JAZZALLOCVEC() or JAZZALLOCSTR() and is the ONLY allocation method used!!!
#define JAZZALLOC(...) GET_MACRO(__VA_ARGS__, JAZZALLOCVEC, JAZZALLOCSTR)(__VA_ARGS__)


/** Sizes of constants inside struct
*/
#define MAX__

#define MAX_CLUSTER_NODES		  32	///< Consider a more efficient management of cluster definitions before increasing this.
#define MAX_NUM_JAZZ_SERVICES	   8	///< The maximum number of jazzService instances stored in a jazzServices.
#define MAX_IP_RANGES			  32	///< Consider a more efficient management of IP white listing before increasing this.
#define MAX_ALIAS_NAMELENGTH	  32	///< Max length of a node alias
#define MAX_HOST_NAMELENGTH		  32	///< Max length of a host name or IP
#define MAX_IPRANGELENGTH		  16	///< Max length of an IP range. (Is an IP with optional *) E.g., 20.1.71.*
#define MAX_RSAPUBKEY_LENGTH	 380	///< Length of an RSA 2048 public key as base 64 + eps
#define MAX_LOG_FILENAME		 128	///< Length of the (optional) log file name.
#define MAX_STRING_LENGTH		2048	///< Maximum size of a string inside a block_C_OFFS_CHARS final 0 included.


/** NA values for different types
*/
#define JAZZC_NA_BOOL		0xff		///< NA for a boolean is binary 0xff.
#define JAZZC_NA_INTEGER	INT_MIN		///< NA for an integer is binary R compatible.
#define JAZZC_NA_DOUBLE		R_NA		///< NA for a double is binary R compatible.
#define JAZZC_NA_STRING		0			///< A NA in a block_C_OFFS_CHARS
#define JAZZC_EMPTY_STRING	1			///< An empty string in a block_C_OFFS_CHARS


/** Constants identifying R object types
*/
#define CHARSXP					   9	///< "scalar" string type (internal only)
#define LGLSXP					  10	///< logical vectors
#define INTSXP					  13	///< integer vectors
#define REALSXP					  14	///< real variables
#define STRSXP					  16	///< string vectors


/** A node as declared in the configuration file.
*/
struct jazz_node
{
	char  alias		 [MAX_ALIAS_NAMELENGTH];	///< Name used internally, also for reporting.
	char  host_or_ip [MAX_HOST_NAMELENGTH];		///< Host or IP. Must be a valid hostname.
	int	  min_role;								///< What the node must be able to do. (Flag combination in [ROLE_PERSISTENCE..ROLE_FOREMAN])
	int	  max_role;								///< What the node could do. (Flag combination in [ROLE_PERSISTENCE..ROLE_FOREMAN])
	int	  port;									///< Http port
};


/** A range of IP. Can be just an IP or parts of it can be *. E.g., 20.1.71.31 or 192.168.*.*
*/
struct IPrange	 {char ipr[MAX_IPRANGELENGTH];};


/** A base64 encoded RSA 2048 public key like the one generated by ssh-keygen (only the key without 'ssh-rsa' or user(at)host.
*/
struct RSApubKey {char key[MAX_RSAPUBKEY_LENGTH];};


/** Security settings as declared in the configuration file.
*/
struct jazz_http_security
{
	int enable_role_creator,
		enable_role_datawriter,
		enable_role_functionwriter,
		enable_role_groupadmin,
		enable_role_group,
		enable_role_reader;

	int num_ip_ranges;

	IPrange range[MAX_IP_RANGES];
};

/** Remote ssh actions done via remote_ssh_action()
*/
#define SSHACTION_PING			 1	///< Check it the remote node is alive.
#define SSHACTION_CHECKREQS		 2	///< Check the remote node's requisites such as: OS, port is free, ssh works, free space, etc.
#define SSHACTION_COPYJAZZ		 3	///< Copy the Jazz artifact to the remote node.
#define SSHACTION_INSTALL		 4	///< Install Jazz in the remote node.
#define SSHACTION_CONFIGURE		 5	///< Configure the remote Jazz.
#define SSHACTION_START			 6	///< Start the remote Jazz.
#define SSHACTION_RELOAD		 7	///< Reload the remote Jazz.
#define SSHACTION_STATUS		 8	///< Check status of the remote Jazz.
#define SSHACTION_STOP			 9	///< Stop the remote Jazz.
#define SSHACTION_UNINSTALL		10	///< Uninstall Jazz from the remote node.

/** Access modes (acmode values) for enter_persistence()
*/
#define ACMODE_READONLY			 1	///< Need lmdb access for reading blocks only.
#define ACMODE_READWRITE		 2	///< Need lmdb access for writing/killing blocks, but not changing sources.
#define ACMODE_SOURCECTL		 3	///< Need lmdb access for changing sources.

/** Http response languages as integer. These constants are entries in the HTTP_LANGUAGE_STRING, they are only used for hard-coding languages.
The list is longer, but these codes have to be kept in this order to let these consts be valid.
*/
#define LANG_DONT_CARE	  0		///< Do not send language header.
#define LANG_EN_US		  1		///< Send the entry 1 (= 'en-US').
#define LANG_ES_ES		  2		///< Send the entry 2 (= 'es-ES').
#define LANG_FR_FR		  3		///< Send the entry 3 (= 'fr-FR').
#define LANG_DE_DE		  4		///< Send the entry 4 (= 'de-DE').
#define LANG_IT_IT		  5		///< Send the entry 5 (= 'it-IT').
#define LANG_TR_TR		  6		///< Send the entry 6 (= 'tr-TR').
#define LANG_LAST_		143		///< An invalid language added at the end to check the size of the HTTP_LANGUAGE_STRING array.

/** A block. Anything stored in persistence is a block.
*/
struct jzzBlockHeader
{
	int	type;					///< Any type under BLOCKTYPE__.
	int length;					///< The number of elements in the block. Mandatory even for raw types (where this is 1).
	int size;					///< The size in bytes of the data part.
	int flags;					///< Flags marking the block accessible via API for updating the data. Not used at jzzBLOCKS level.
	long long hash64;			///< A MurmurHash64A() of the data part.
};

struct block_C_BOOL : jzzBlockHeader
{
	unsigned char data[];				///< 1 byte long boolean
};

struct string_buffer
{
	char NA, EMPTY;						///< A binary zero making a string with offset 0 (== JAZZC_NA_STRING) or 1 (== JAZZC_EMPTY_STRING) == ""
	bool isBig;							///< Set by get_string_idx_C_OFFS_CHARS() when the number of strings > STR_SEARCH_BIG_ABOVE
	int	 last_idx;						///< The index to the first free space after the last stored string
	char buffer[];						///< The buffer where all the non-empty strings are stored
};

struct block_C_OFFS_CHARS : jzzBlockHeader
{
/** An array of offsets inside the char buffer where the zero terminated strings are. After a fixed size data[length], a string buffer contains
the actual strings.
*/
	int data[];
//	string_buffer string_buffer_is_not_here[];
};

struct block_C_INTEGER : jzzBlockHeader
{
	int data[];							///< 32 bit integers
};

struct block_C_REAL : jzzBlockHeader
{
	double data[];						///< doubles
};

struct block_C_RAW : jzzBlockHeader
{
	char data[];						///< simply a buffer containing one element of whatever
};

typedef jzzBlockHeader	   * pJazzBlock;
typedef block_C_BOOL	   * pBoolBlock;
typedef block_C_OFFS_CHARS * pCharBlock;
typedef block_C_INTEGER	   * pIntBlock;
typedef block_C_REAL	   * pRealBlock;
typedef block_C_RAW		   * pRawBlock;
typedef string_buffer	   * pStringBuff;

typedef chrono::steady_clock::time_point time_point;

/** System structures: Source Attributes
*/

struct block_SourceAttributes : jzzBlockHeader
{
	int version;						///< Internal version of the lmdb image
};


/** Ancestor class of all Jazz services requiring start/stop/reload. This is an empty container of virtual methods that should be implemented
in the descendants.
*/
class jazzService {

	public:

		virtual bool start	();
		virtual bool stop	();
		virtual bool reload ();
};


/** A collection of jazzService descendants operated through a single point of access.
*/
class jazzServices {

	public:

		bool register_service(jazzService * service);

		bool start_all	();
		bool stop_all	();
		bool reload_all ();

	private:

		int			 num_services = 0;
		bool		 started [MAX_NUM_JAZZ_SERVICES];
		jazzService* pService[MAX_NUM_JAZZ_SERVICES];
};


/** A collection of common utils including: configuration, cryptography, html, threads, logger and profiling.
*/
class jazzCommons {

	public:

		 jazzCommons();
		~jazzCommons();

		// BASE: Methods in jazzCommons.cpp

		bool load_cluster_conf();
		bool get_cluster_port (const char* alias, int &port);

		// Config related methods.

		bool load_config_file(const char *conf);
		bool get_config_key	 (const char *key, int &value);
		bool get_config_key	 (const char *key, double &value);
		bool get_config_key	 (const char *key, string &value);
		void debug_config_put(const string key, const string val);

		// Access utils

		bool remote_new_source	  (int node, const char * name);
		bool remote_kill_source	  (int node, const char * name);

		bool remote_block_put	  (int node, const char * source, const persistedKey &key, pJazzBlock  block);
		bool remote_block_get	  (int node, const char * source, const persistedKey &key, pJazzBlock &block);
		bool remote_block_kill	  (int node, const char * source, const persistedKey &key);

		bool remote_api_call	  (int node, const char * query, pJazzBlock &block);
		bool remote_testing_point ();

		// Threads utils

		int	 enter_persistence	(int acmode);
		int	 enter_R_interpreter();
		void leave_persistence	(int tid);
		void leave_R_interpreter(int tid);

		// Logger utils.

		bool logger_init ();
		void logger_close();
		void log		 (int loglevel, const char *message);
		void log_printf	 (int loglevel, const char *fmt, ...);

		// HTTP & Server utils

		bool configure_MHD_server();
		void get_server_start_params (unsigned int &flags, MHD_AcceptPolicyCallback &apc, MHD_AccessHandlerCallback &dh, MHD_OptionItem* &pops);

		void enter_api_call (struct MHD_Connection * connection, const char * url, int method);
		void leave_api_call	(int status);

#ifdef DEBUG
		bool was_reloaded = false;	// Don't check for unused configuration keys if reloaded.
#endif

	private:

		/// Logger
		bool log_2_local = false;				///< Log (also) to file logfilename. Always appended, never erased.
		char logfilename[MAX_LOG_FILENAME];		///< Name of a file to (immediately) write logs to if log_2_local.
		int log_block_size = 0;					///< Number of logging events per block. Written after a maximum of LOGGER_BLOCK_TIMEOUT seconds.
												///< For persistence logs. Set to zero disables writing to persistence and logs to stdout instead.
		int	log_keep_blocks;					///< Number of most recent blocks kept by the logger.
		int	log_block_timeout;					///< Time in seconds after which the most recent events are written to their persistence block.

		time_point log_big_bang = chrono::steady_clock::now();;	/// Clock zero for the logger.

		/// Cluster
		int cluster_num_nodes = 0;				///< Number of nodes in the cluster
		int cluster_password_mode;				///< 0 = use configuration variable SSH_CLUSTER_OWNER_PASS, 1 = use password less ssh.
		string cluster_ssh_user;				///< User name with ssh access to all nodes.
		string cluster_ssh_password;			///< Password for all nodes if SSH_NODES_PASSWORD == 0

		jazz_node cluster_node[MAX_CLUSTER_NODES];	/// The definition of the nodes.

		/// Server
		unsigned int server_flags;				///< The parameter flags in MHD_start_daemon()
		MHD_AcceptPolicyCallback  server_apc;	///< The parameter apc in MHD_start_daemon()
		MHD_AccessHandlerCallback server_dh;	///< The parameter dh in MHD_start_daemon()
		MHD_OptionItem server_options[9];		///< The variadic parameter MHD_OPTION_ARRAY, server_options, MHD_OPTION_END in MHD_start_daemon()
		int server_perimetral;					///< In range MODE_SAFE_IF_PHYSICAL..MODE_EXTERNAL_PERIMETER
		jazz_http_security security;			///< Security related options

		/// The configuration
		map<string, string> config;
#ifdef DEBUG
		map<string, int> config_used;
#endif

};

extern jazzCommons	jCommons;
extern jazzServices jServices;

extern const char* const HTTP_LANGUAGE_STRING[LANG_LAST_ + 1];
extern const char* const HTTP_MIMETYPE_STRING[BLOCKTYPE_LAST_ + 1];


// HTTP callbacks

extern int jazz_answer_to_connection(void *cls,
									 struct MHD_Connection *connection,
									 const char *url,
									 const char *method,
									 const char *version,
									 const char *upload_data,
									 size_t *upload_data_size,
									 void **con_cls);

extern int perimetral_answer_to_connection(void *cls,
										   struct MHD_Connection *connection,
										   const char *url,
										   const char *method,
										   const char *version,
										   const char *upload_data,
										   size_t *upload_data_size,
										   void **con_cls);

extern int jazz_apc_callback (void *cls,
							  const struct sockaddr *addr,
							  socklen_t addrlen);

// R NA equivalence
inline int R_IsNA(double x)
{
	if (!isnan(x)) return 0;

	union {double d; int i[2];} v;

	v.d = x;

	return (v.i[0] == 1954);
}

// From jzzMISCUTILS.cpp

bool		 exists			 (const char* fnam);
const char * okfail			 (bool		  b);

// From jzzALLOC.cpp

bool jazzAlloc		  (void* &ptr, int alloc_type, int size, char* pstr = NULL);
bool jazzPtrTrackClose();
void jazzPtrTrack	  (void* &ptr, int alloc_type, int fun);
void jazzFree		  (void* &ptr, int alloc_type);
int	 auto_type_block  (pJazzBlock pb);

// From jzzCommons.cpp

extern double R_NA;

#endif

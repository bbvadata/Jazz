/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   1. Biomodelling - The AATModelQueue class (c) Jacques Basaldúa, 2009-2012 licensed
      exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

   2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

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

/**< \brief Implementation of the ONLY entry point for RAM allocation. This is what JAZZALLOC() and JAZZFREE() call.

This implements all the RAM allocation parts declared in jzzH/jazzCommons.h. The stats on RAM allocation are implemented too as part of the
functionalities available through jzzSTEERING.
*/

#include "src/include/jazz01_commons.h"
#include "src/include/jazz.h"

/*~ end of automatic header ~*/

/*	-----------------------------------------------
	  R A M	  A L L O C A T I O N
--------------------------------------------------- */

/** Allocate RAM. This is the ONLY allocation function in the executable.

	**Always use the macros JAZZALLOC() and JAZZFREE().** Never call the functions jazzAlloc()/jazzFree() directly!

	All the alloc_type below RAM_ALLOC_R_LGLSXP, allocate/free with malloc/free, but size is in elements!
	The codes in RAM_ALLOC_R_LGLSXP.. create object that will be owned by R sessions.

	\param ptr		  The address where the pointer is stored
	\param alloc_type The type of allocation RAM_ALLOC_SERVICE .. RAM_ALLOC_R_RAWSXP
	\param size		  The size of the request in elements, depends on alloc_type.
	\param pstr		  The string (CHARSXP) for alloc_type == RAM_ALLOC_R_CHARSXP only.

	\return			  TRUE on success. Further actions may be introduced on failure depending on alloc_type

	It is mandatory to route all RAM alloc/free through these functions in order to support monitoring and debugging.

	The calling process should use jazzAlloc() once per ptr address with a mandatory jazzFree() only when jazzAlloc() succeeded.

	**functions other than R functions (alloc_type < RAM_ALLOC_R_LGLSXP)**: Are simply encapsulated malloc() calls with the advantage
	of tracking capabilities due to the single entry point and type checking.

	**R functions other than RAM_ALLOC_R_CHARSXP**: Use allocVector(xx, size) + PROTECT(). Note that it is MANDATORY to use these
	pointers as R objects owned by R. The final, and also MANDATORY, jazzFree call when the pointer is no longer necessary will not
	actually free the object, just UNPROTECT_PTR() it to let R manage the memory.

	**RAM_ALLOC_R_CHARSXP**: This creates the individual strings to be assigned. It uses mkChar(). From the R doc: ''You can obtain a CHARSXP
	by calling mkChar and providing a null-terminated C-style string. This function will return a pre-existing CHARSXP if one with a matching
	string already exists, otherwise it will create a new one and add it to the cache before returning it to you.'' These objects are immutable
	and managed by R. They MUST be assigned as an element of a STRSXP vector. When that vector is jazzFree()d (which is an UNPROTECT_PTR() call)
	all its members will be understood by R as not used (if their usage counter counts down to 0, remember they are shared). This is the only
	acceptable use of RAM_ALLOC_R_CHARSXP. jazzFree(pt, RAM_ALLOC_R_CHARSXP) does nothing, but is still mandatory to balance the RAM tracking
	and checking.

	**Errors**: Errors in RAM_ALLOC_SERVICE should terminate the program. Errors in all other non R calls should terminate the http call with
	some internal server error. Errors in R functions should let R handle memory issues and simply not use the pointer and take further actions
	where necessary. This function just returns false an logs the errors with LOG_ERROR level.
*/
bool jazzAlloc (void* &ptr, int alloc_type, int size, char* pstr)
{
	switch (alloc_type)
	{
		case RAM_ALLOC_SERVICE:
		case RAM_ALLOC_RESPONSE:
			ptr = malloc(size);

			if (ptr != NULL) return true;

			break;

		case RAM_ALLOC_C_BOOL:
			ptr = malloc(sizeof(jzzBlockHeader) + size);

			if (ptr != NULL)
			{
				memset(ptr, 0, sizeof(jzzBlockHeader));

				reinterpret_cast<pJazzBlock>(ptr)->type	  = BLOCKTYPE_C_BOOL;
				reinterpret_cast<pJazzBlock>(ptr)->length = size;
				reinterpret_cast<pJazzBlock>(ptr)->size	  = size;

				return true;
			}

			break;

		case RAM_ALLOC_C_OFFS_CHARS:
			ptr = malloc(sizeof(jzzBlockHeader) + size);

			if (ptr != NULL)
			{
				memset(ptr, 0, sizeof(jzzBlockHeader));

				reinterpret_cast<pJazzBlock>(ptr)->type	  = BLOCKTYPE_C_OFFS_CHARS;
				reinterpret_cast<pJazzBlock>(ptr)->length = 0;						// Must be assigned outside
				reinterpret_cast<pJazzBlock>(ptr)->size	  = size;

				return true;
			}

			break;

		case RAM_ALLOC_C_INTEGER:
			ptr = malloc(sizeof(jzzBlockHeader) + size*sizeof(int));

			if (ptr != NULL)
			{
				memset(ptr, 0, sizeof(jzzBlockHeader));

				reinterpret_cast<pJazzBlock>(ptr)->type	  = BLOCKTYPE_C_INTEGER;
				reinterpret_cast<pJazzBlock>(ptr)->length = size;
				reinterpret_cast<pJazzBlock>(ptr)->size	  = size*sizeof(int);

				return true;
			}

			break;

		case RAM_ALLOC_C_REAL:
			ptr = malloc(sizeof(jzzBlockHeader) + size*sizeof(double));

			if (ptr != NULL)
			{
				memset(ptr, 0, sizeof(jzzBlockHeader));

				reinterpret_cast<pJazzBlock>(ptr)->type	  = BLOCKTYPE_C_REAL;
				reinterpret_cast<pJazzBlock>(ptr)->length = size;
				reinterpret_cast<pJazzBlock>(ptr)->size	  = size*sizeof(double);

				return true;
			}

			break;

		case RAM_ALLOC_C_RAW:
			ptr = malloc(sizeof(jzzBlockHeader) + size);

			if (ptr != NULL)
			{
				memset(ptr, 0, sizeof(jzzBlockHeader));

				reinterpret_cast<pJazzBlock>(ptr)->type	  = BLOCKTYPE_RAW_ANYTHING;
				reinterpret_cast<pJazzBlock>(ptr)->length = 1;
				reinterpret_cast<pJazzBlock>(ptr)->size	  = size;

				return true;
			}

			break;

		default:
			ptr = NULL;

			jCommons.log(LOG_ERROR, "jazzAlloc(): Unexpected alloc_type.");

			return false;
	}

	jCommons.log_printf(LOG_ERROR, "jazzAlloc(): malloc() failed, size = %d, alloc_type = %d.", size, alloc_type);

	return false;
}


#ifdef DEBUG
	map<void *, int> alloc_hist;
#endif

/** Verify if all the pointers allocated have been checked (closed when using the only recommended mechanism).
*/
bool jazzPtrTrackClose ()
{
#ifdef DEBUG
	int i = 0;

	for(map<void *, int>::iterator it = alloc_hist.begin(); it != alloc_hist.end(); ++it)
	{
		if (!(it->second & RAM_TRACK_WAS_CHECKED)) i++;
	}

	if (i)
	{
		jCommons.log_printf(LOG_ERROR, "jazzPtrTrackClose(): %d unclosed pointers found.", i);

		return false;
	}
	else jCommons.log(LOG_INFO, "jazzPtrTrackClose() All pointers closed ok.");
#endif

	return true;
}

/** Learn a pointer or test the type and knowledge of a pointer to support RAM debugging.

	**Never call this function directly. JAZZALLOC() and JAZZFREE() may call it depending on conditional defines.**

	\param ptr		  The address where the pointer is stored
	\param alloc_type The type of allocation RAM_ALLOC_SERVICE .. RAM_ALLOC_R_RAWSXP
	\param fun		  Either RAM_TRACK_LEARN or RAM_TRACK_CHECK
*/
void jazzPtrTrack (void* &ptr, int alloc_type, int fun)
{
#ifdef DEBUG
	if (ptr == NULL)
	{
		jCommons.log(LOG_ERROR, "NULL pointer passed to jazzPtrTrack");

		return;
	}
	switch (fun)
	{
		case RAM_TRACK_LEARN:
			alloc_hist[ptr] = alloc_type;

			return;

		case RAM_TRACK_CHECK:
			try
			{
				int typ = alloc_hist[ptr];

				if (typ != alloc_type)
					jCommons.log(LOG_ERROR, "jazzPtrTrack(): Wrong type for known pointer.");

				alloc_hist[ptr] = typ | RAM_TRACK_WAS_CHECKED;

//				jCommons.log(LOG_INFO, "jazzPtrTrack() checked.");

				return;
			}
			catch (...)
			{
				jCommons.log(LOG_ERROR, "jazzPtrTrack(): Unknown pointer.");

				return;
			}
	}
#endif
}


/** Free RAM. This is the ONLY freeing function in the executable.

	**Always use the macros JAZZALLOC() and JAZZFREE().** Never call the functions jazzAlloc()/jazzFree() directly!

	\param ptr		  The address where the pointer is stored
	\param alloc_type The type of allocation RAM_ALLOC_SERVICE .. RAM_ALLOC_R_RAWSXP

	The calling process should use jazzAlloc() once per ptr address with a mandatory jazzFree() only when jazzAlloc() succeeded.

	See jazzAlloc for details.
*/
void jazzFree (void* &ptr, int alloc_type)
{
	switch (alloc_type)
	{
		case RAM_ALLOC_SERVICE:
		case RAM_ALLOC_RESPONSE:
		case RAM_ALLOC_C_BOOL:
		case RAM_ALLOC_C_OFFS_CHARS:
		case RAM_ALLOC_C_INTEGER:
		case RAM_ALLOC_C_REAL:
		case RAM_ALLOC_C_RAW:
			free(ptr);
			ptr = NULL;

			return;

		default:
			jCommons.log_printf(LOG_ERROR, "jazzFree(): Unexpected alloc_type = %d.", alloc_type);

			return;
	}
}

#ifdef DEBUG
int auto_type_block(pJazzBlock pb)
{
	switch (pb->type)
	{
		case BLOCKTYPE_C_BOOL:
			return RAM_ALLOC_C_BOOL;

		case BLOCKTYPE_C_OFFS_CHARS:
			return RAM_ALLOC_C_OFFS_CHARS;

		case BLOCKTYPE_C_FACTOR:
		case BLOCKTYPE_C_GRADE:
		case BLOCKTYPE_C_INTEGER:
			return RAM_ALLOC_C_INTEGER;

		case BLOCKTYPE_C_TIMESEC:
		case BLOCKTYPE_C_REAL:
			return RAM_ALLOC_C_REAL;

		default:
			if (pb->type >= BLOCKTYPE_RAW_ANYTHING && pb->type <= BLOCKTYPE_SOURCE_ATTRIB)
				return RAM_ALLOC_C_RAW;
	}
	jCommons.log(LOG_ERROR, "Unexpected type in auto_type_block()");

	return RAM_ALLOC_RESPONSE;
}
#endif


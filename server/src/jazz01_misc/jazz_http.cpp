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

/*! \brief Some http support utilities.

This module includes the support for gathering http statistics, server configuration utilities and a set of constants converting integer values to
well formed http protocol messages.
*/

#include "src/include/jazz01_commons.h"
#include "src/jazz01_misc/jazz_websource.h"

/*~ end of automatic header ~*/

/*	-----------------------------------------------
	 jazzCommons : S e r v e r	 u t i l s
--------------------------------------------------- */

/** Configure the MHD server from the appropriate configuration variables.

	\return	If the configuration is valid. logs with LOG_MISS if failed.

	This sets the private server variables in jazzCommons according to the configuration variables.

	After a successful call to this, get_server_start_params() can be used to get the necessary values to start the server via MHD_start_daemon()
*/
bool jazzCommons::configure_MHD_server()
{
	int ok, udb, ssl, iv6, pch, sdn, txf;

	ok =   get_config_key("JazzHTTPSERVER.MHD_USE_DEBUG", udb)
		 & get_config_key("JazzHTTPSERVER.MHD_USE_SSL", ssl)
		 & get_config_key("JazzHTTPSERVER.MHD_USE_IPv6", iv6)
		 & get_config_key("JazzHTTPSERVER.MHD_USE_PEDANTIC_CHECKS", pch)
		 & get_config_key("JazzHTTPSERVER.MHD_SUPPRESS_DATE_NO_CLOCK", sdn)
		 & get_config_key("JazzHTTPSERVER.MHD_USE_TCP_FASTOPEN", txf);

	if ((!ok ) | ((udb | ssl | iv6 | pch | sdn | txf) & 0xfffffffe))
	{
		log(LOG_MISS, "configure_MHD_server() failed. In flags variables block.");

		return false;
	}
	server_flags =	udb*MHD_USE_DEBUG | ssl*MHD_USE_SSL | iv6*MHD_USE_IPv6 | pch*MHD_USE_PEDANTIC_CHECKS
				  | sdn*MHD_SUPPRESS_DATE_NO_CLOCK | txf*MHD_USE_TCP_FASTOPEN;

	server_flags = server_flags | MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;	// Only threading model for Jazz.

	ok = get_config_key("JazzHTTPSERVER.MHD_PERIMETRAL_MODE", server_perimetral);

	if (!ok)
	{
		log(LOG_MISS, "configure_MHD_server() failed. Key JazzHTTPSERVER.MHD_PERIMETRAL_MODE not found.");

		return false;
	}

	switch(server_perimetral)
	{
		case MODE_SAFE_IF_PHYSICAL:
			server_dh  = &jazz_answer_to_connection;
			server_apc = NULL;

			break;

/*		case MODE_TRUSTED_ONLY:
			server_dh  = &jazz_answer_to_connection;
			server_apc = &jazz_apc_callback;

			break;

		case MODE_INTERNAL_PERIMETER ... MODE_EXTERNAL_PERIMETER:
			server_dh = &perimetral_answer_to_connection;
			server_apc = NULL;

			break;
 */
		default:
			log(LOG_MISS, "configure_MHD_server() failed. MHD_PERIMETRAL_MODE out of range.");

			return false;
	}

	int cml, cmi, ocl, oct, pic, tps, tss, lar;

	ok =   get_config_key("JazzHTTPSERVER.MHD_OPTION_CONNECTION_MEMORY_LIMIT", cml)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_CONNECTION_MEMORY_INCREMENT", cmi)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_CONNECTION_LIMIT", ocl)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_CONNECTION_TIMEOUT", oct)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_PER_IP_CONNECTION_LIMIT", pic)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_THREAD_POOL_SIZE", tps)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_THREAD_STACK_SIZE", tss)
		 & get_config_key("JazzHTTPSERVER.MHD_OPTION_LISTENING_ADDRESS_REUSE", lar);

	if (!ok)
	{
		log(LOG_MISS, "configure_MHD_server() failed. In options variables block.");

		return false;
	}

	MHD_OptionItem *pop = server_options;

	if (cml) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_LIMIT,	   cml, NULL}; pop++;
	if (cmi) pop[0] = {MHD_OPTION_CONNECTION_MEMORY_INCREMENT, cmi, NULL}; pop++;
	if (ocl) pop[0] = {MHD_OPTION_CONNECTION_LIMIT,			   ocl, NULL}; pop++;
	if (oct) pop[0] = {MHD_OPTION_CONNECTION_TIMEOUT,		   oct, NULL}; pop++;
	if (pic) pop[0] = {MHD_OPTION_PER_IP_CONNECTION_LIMIT,	   pic, NULL}; pop++;
	if (tps) pop[0] = {MHD_OPTION_THREAD_POOL_SIZE,			   tps, NULL}; pop++;
	if (tss) pop[0] = {MHD_OPTION_THREAD_STACK_SIZE,		   tss, NULL}; pop++;
	if (lar) pop[0] = {MHD_OPTION_LISTENING_ADDRESS_REUSE,	   lar, NULL}; pop++;

	pop[0] = {MHD_OPTION_END, 0, NULL};

	ok =   get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_CREATOR",		   security.enable_role_creator)
		 & get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_DATAWRITER",	   security.enable_role_datawriter)
		 & get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_FUNCTIONWRITER", security.enable_role_functionwriter)
		 & get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_GROUPADMIN",	   security.enable_role_groupadmin)
		 & get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_GROUP",		   security.enable_role_group)
		 & get_config_key("JazzHTTPSERVER.MHD_ENABLE_ROLE_READER",		   security.enable_role_reader);

	if ((!ok) | ((	security.enable_role_creator
				  | security.enable_role_datawriter
				  | security.enable_role_functionwriter
				  | security.enable_role_groupadmin
				  | security.enable_role_group
				  | security.enable_role_reader) & 0xfffffffe))
	{
		log(LOG_MISS, "configure_MHD_server() failed. In enable_role_* variables block.");

		return false;
	}

	string prk;

	ok = get_config_key("JazzHTTPSERVER.MHD_OVERRIDE_PUBLIC_ROOT_KEY", prk);

	if (!ok)
	{
		log(LOG_MISS, "configure_MHD_server() failed. Invalid or missing MHD_OVERRIDE_PUBLIC_ROOT_KEY.");

		return false;
	}

	ok = get_config_key("JazzHTTPSERVER.MHD_TRUSTED_IP_RANGES", security.num_ip_ranges);

	if (!ok || security.num_ip_ranges < 0 || security.num_ip_ranges > MAX_IP_RANGES)
	{
		log(LOG_MISS, "configure_MHD_server() failed. Invalid or missing MHD_TRUSTED_IP_RANGES.");

		return false;
	}

	string si, key, ipr;

	for (int i = 0; i < security.num_ip_ranges; i++)
	{
		si = to_string(i);

		key = "JazzHTTPSERVER.MHD_TRUSTED_IP_RANGE_" + si;

		ok = get_config_key(key.c_str(), ipr);

		if (!ok || ipr.length() > sizeof(IPrange) - 1)
		{
			log(LOG_MISS, "configure_MHD_server() failed. Invalid or missing MHD_TRUSTED_IP_RANGES.");

			return false;
		}

		strcpy(security.range[i].ipr, ipr.c_str());
	}

	return true;
}


/** Get the necessary parameters to start the server calling MHD_start_daemon()

	This function requires a previous successful call to configure_MHD_server

	\param flags flags in MHD_start_daemon()
	\param apc	 apc in MHD_start_daemon()
	\param dh	 dh in MHD_start_daemon()
	\param pops	 a pointer to an MHD_OPTION_ARRAY structure.

	Start the server with: MHD_start_daemon (flags, port, apc, NULL, dh, NULL, MHD_OPTION_ARRAY, pops, MHD_OPTION_END)
*/
void jazzCommons::get_server_start_params (unsigned int &flags, MHD_AcceptPolicyCallback &apc, MHD_AccessHandlerCallback &dh, MHD_OptionItem* &pops)
{
	flags = server_flags;
	apc	  = server_apc;
	dh	  = server_dh;
	pops  = server_options;
}


/** Notify the http stats gathering system that a response is entered.

	\param connection The MHD_Connection passed to the answer_to_connection() callback.
	\param url		  The API query
	\param method	  The HTPP method in binary as HTTP_HEAD..HTTP_UNKNOWN

	Internally, an enter_api_call()/leave_api_call() pair is matched by the thread ID. Unmatched pairs are stored as errors. All the callback
calls do not generate an enter_api_call()/leave_api_call() pair. The number of calls sampled depends on the configuration variable
JazzHTTPSERVER.REST_SAMPLE_PROPORTION.
*/
void jazzCommons::enter_api_call (struct MHD_Connection *connection, const char *url, int method)
{
//_in_deprecated_code_TODO: Implement enter_api_call.

}


/** Notify the http stats gathering system that a response is completed.

	\param status The HTTP status returned.

	The time taken will be computed when matching this to its corresponding enter_api_call() call. Internally, an enter_api_call()/leave_api_call()
pair is matched by the thread ID. Unmatched pairs are stored as errors. All the callback calls do not generate an enter_api_call()/leave_api_call()
pair. The number of calls sampled depends on the configuration variable JazzHTTPSERVER.REST_SAMPLE_PROPORTION.
*/
void jazzCommons::leave_api_call (int status)
{
//_in_deprecated_code_TODO: Implement leave_api_call.

}

/*	--------------------------------------------------
	  S T R I N G	c o n s t a n t s
--------------------------------------------------- */

/** http response code strings. Do no change the order of those in range: LANG_EN_US..LANG_TR_TR without matching the constants appropriately.
*/
const char* const HTTP_LANGUAGE_STRING[] = {"", "en-US", "es-ES", "fr-FR", "de-DE", "it-IT", "tr-TR", "aa", "ab", "af", "am", "ar", "as", "ay",
											"az", "ba", "be", "bg", "bh", "bi", "bn", "bo", "br", "ca", "co", "cs", "cy", "da", "de", "dz", "el",
											"en", "eo", "es", "et", "eu", "fa", "fi", "fj", "fo", "fr", "fy", "ga", "gd", "gl", "gn", "gu", "ha",
											"hi", "hr", "hu", "hy", "ia", "ie", "ik", "in", "is", "it", "iw", "ja", "ji", "jw", "ka", "kk", "kl",
											"km", "kn", "ko", "ks", "ku", "ky", "la", "ln", "lo", "lt", "lv", "mg", "mi", "mk", "ml", "mn", "mo",
											"mr", "ms", "mt", "my", "na", "ne", "nl", "no", "oc", "om", "or", "pa", "pl", "ps", "pt", "qu", "rm",
											"rn", "ro", "ru", "rw", "sa", "sd", "sg", "sh", "si", "sk", "sl", "sm", "sn", "so", "sq", "sr", "ss",
											"st", "su", "sv", "sw", "ta", "te", "tg", "th", "ti", "tk", "tl", "tn", "to", "tr", "ts", "tt", "tw",
											"uk", "ur", "uz", "vi", "vo", "wo", "xh", "yo", "zh", "zu", "@the_last@"};

const char* const HTTP_MIMETYPE_STRING[] = {"", "application/octet-stream", "application/octet-stream", "application/octet-stream",
											"application/octet-stream", "application/octet-stream", "application/octet-stream",
											"application/octet-stream", "", "", "application/octet-stream",
											"text/plain; charset=utf-8", "application/octet-stream", "application/vnd.android.package-archive",
											"text/css", "text/csv", "image/gif", "text/html", "application/octet-stream", "image/jpeg",
											"application/javascript", "application/json", "video/mp4", "application/pdf", "image/png",
											"text/tab-separated-values", "text/plain", "application/x-silverlight-app", "application/xml",
											"image/x-icon",	 "",  "text/plain", "text/plain", "application/jazz-vm",
											"", "", "", "", "", "", "", "application/jazz-binary", "application/jazz-binary",
											"application/jazz-binary", "application/jazz-binary", "application/jazz-binary",
											"application/jazz-binary", "application/jazz-binary", "application/jazz-binary"};

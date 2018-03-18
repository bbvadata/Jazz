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

/**< \brief Implementations of root classes.

	This file has things as trivial constructors/destructors + root methods of start()/stop()/reload() + Some cluster configuration parsing.
The utilities are implemented in the appropriate source files in: jzzMISCUTILS, jzzALLOC, jzzCONFIG, jzzACCESS, jzzTHREADS, jzzLOGGER and jzzHTTP.
*/

#include "src/include/jazz01_commons.h"

/*~ end of automatic header ~*/

/*	-----------------------------------------------
	 jazzService : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Start a jazzService descendant.

	This is the first method of a jazzService to be called.

	If start() returns true, it MAY get reload() calls and WILL get just one stop() call.
	If start() returns false, it will never be called again.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from jazzService)
*/
bool jazzService::start ()
{
	return true;
}


/** Stop a jazzService descendant.

	This is the last method of a jazzService to be called.

	It is called only once and only if start() was successful.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from jazzService)
*/
bool jazzService::stop ()
{
	return true;
}


/** Start a jazzService descendant.

	This is the first method of a jazzService to be called.

	If start() returns true, it MAY get reload() calls and WILL get just one stop() call.
	If start() returns false, it will never be called again.

	Virtual method to be overridden in all objects implementing the interface. (I.e., descending from jazzService)
*/
bool jazzService::reload ()
{
	return true;
}

/*	-----------------------------------------------
	 jazzServices : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Adds a service to the list of jazzService objects managed by the jazzServices.
*/
bool jazzServices::register_service(jazzService * service)
{
	if (num_services >= MAX_NUM_JAZZ_SERVICES)
	{
		jCommons.log(LOG_MISS, "Too many services in jazzServices::register_service().");

		return false;
	}

	pService[num_services]	 = service;
	started [num_services++] = false;

	return true;
}


/** Starts all the registered services that have not been started yet.

	In case of a service failing to start, start_all() aborts not trying to start any other service.

	\return True on success. LOG_MISS and false on abort.
*/
bool jazzServices::start_all ()
{
	for (int i = 0; i < num_services; i++)
	{
		if (!started[i])
		{
			if (!pService[i]->start())
			{
				jCommons.log(LOG_MISS, "Some service failed to start, start_all() aborting.");

				return false;
			}
			started[i] = true;
		}
	}

	return true;
}


/** Stops all the registered services that have been started.

	In case of a service failing to stop, stop_all() will still close the rest of the started services.

	\return True on success. LOG_MISS and false on any failure.
*/
bool jazzServices::stop_all ()
{
	bool st_ok = true;

	for (; num_services >= 0; --num_services)
	{
		if (started[num_services])
		{
			st_ok = pService[num_services]->stop() & st_ok;

			started[num_services] = true;
		}
	}

	if (!st_ok) jCommons.log(LOG_MISS, "Some service(s) failed to stop in stop_all().");

	return st_ok;
}


/** Reloads all the registered services that have been started.

	In case of a service failing to reload, reload_all() aborts not trying to reload any other service.

	\return True on success. LOG_MISS and false on abort.
*/
bool jazzServices::reload_all ()
{

#ifdef DEBUG
	jCommons.was_reloaded = true;	// Don't check for unused configuration keys if reloaded.
#endif

	for (int i = 0; i < num_services; i++)
	{
		if (started[i])
		{
			if (!pService[i]->reload())
			{
				jCommons.log(LOG_MISS, "Some service failed to reload, reload_all() aborting.");

				return false;
			}
		}
	}

	return true;
}

/*	-----------------------------------------------------------
	 jazzCommons : C o n s t r u c t o r / d e s t r u c t o r
--------------------------------------------------------------- */

/** jazzCommons constructor: The root class does nothing.
*/
jazzCommons::jazzCommons()
{
};

/** jazzCommons destructor: ifdef DEBUG, the root class checks for unused configuration variables.
*/
jazzCommons::~jazzCommons() {

#ifdef DEBUG
	#ifndef CATCH_TEST

	if (!was_reloaded)
	{
		string msg("Configuration variables not used: ");
		int i = 0;

		for(map<string, int>::iterator it = config_used.begin(); it != config_used.end(); ++it)
		{
			if (!it->second)
			{
				if (i++) msg = msg + ", " + it->first;
				else	 msg = msg + it->first;
			}
		}

		if (i) log(LOG_WARN, msg.c_str());
	}

	#endif
#endif

};

/*	-----------------------------------------------
	 jazzCommons : C l u s t e r   u t i l s
--------------------------------------------------- */

/** Load the cluster configuration.

	This function loads the variables.

		int cluster_num_nodes = 0;		/// Number of nodes in the cluster
		int cluster_password_mode;		/// 0 = use configuration variable SSH_CLUSTER_OWNER_PASS, 1 = use password less ssh.
		string cluster_ssh_user;		/// User name with ssh access to all nodes.
		string cluster_ssh_password;	/// Password for all nodes if SSH_NODES_PASSWORD == 0

		jazz_node cluster_node[MAX_CLUSTER_NODES];	/// The definition of the nodes.

	It is verbose logging the conflicts via LOG_MISS, strict in the buffer requirements, but permissive in the values. No values
are checked, incompatibilities must be checked elsewhere.
*/
bool jazzCommons::load_cluster_conf()
{
	bool lok =	 get_config_key("JazzCLUSTER.JAZZ_NODES", cluster_num_nodes)
			   & get_config_key("JazzCLUSTER.SSH_NODES_PASSWORD", cluster_password_mode)
			   & get_config_key("JazzCLUSTER.SSH_CLUSTER_OWNER_USER", cluster_ssh_user)
			   & get_config_key("JazzCLUSTER.SSH_CLUSTER_OWNER_PASS", cluster_ssh_password);

	if (!lok)
	{
		log(LOG_MISS, "Cluster loading failed. (In the global variables.)");

		return false;
	}

	if (cluster_num_nodes < 1 | cluster_num_nodes > MAX_CLUSTER_NODES)
	{
		log(LOG_MISS, "Cluster loading failed. (Invalid number of nodes.)");

		return false;
	}

	string si, key, val;
	string msg_knf ("Cluster loading failed. Key not found ");
	string msg_ivv ("Cluster loading failed. Invalid value ");
	jazz_node node;

	for (int i = 0; i < cluster_num_nodes; i++)
	{
		si = to_string(i);

		key = "JazzCLUSTER.SSH_NODE_ALIAS_" + si;

		if (!get_config_key(key.c_str(), val))
		{
			msg_knf = msg_knf + key;
			log(LOG_MISS, msg_knf.c_str());

			return false;
		}
		if (val.length() < 1 || val.length() > sizeof(node.alias) - 1)
		{
			msg_ivv = msg_ivv + val;
			log(LOG_MISS, msg_ivv.c_str());

			return false;
		}
		strcpy(node.alias, val.c_str());

		key = "JazzCLUSTER.SSH_NODE_HOST_OR_IP_" + si;

		if (!get_config_key(key.c_str(), val))
		{
			msg_knf = msg_knf + key;
			log(LOG_MISS, msg_knf.c_str());

			return false;
		}
		if (val.length() < 1 || val.length() > sizeof(node.host_or_ip) - 1)
		{
			msg_ivv = msg_ivv + val;
			log(LOG_MISS, msg_ivv.c_str());

			return false;
		}
		strcpy(node.host_or_ip, val.c_str());

		key = "JazzCLUSTER.JAZZ_NODE_MIN_ROLE_" + si;

		if (!get_config_key(key.c_str(), node.min_role))
		{
			msg_knf = msg_knf + key;
			log(LOG_MISS, msg_knf.c_str());

			return false;
		}

		key = "JazzCLUSTER.JAZZ_NODE_MAX_ROLE_" + si;

		if (!get_config_key(key.c_str(), node.max_role))
		{
			msg_knf = msg_knf + key;
			log(LOG_MISS, msg_knf.c_str());

			return false;
		}

		key = "JazzCLUSTER.JAZZ_NODE_PORT_" + si;

		if (!get_config_key(key.c_str(), node.port))
		{
			msg_knf = msg_knf + key;
			log(LOG_MISS, msg_knf.c_str());

			return false;
		}

		cluster_node[i] = node;
	}

	return true;
}


/** Return the http port of a node by (linearly) looking at its alias in the cluster configuration.
*/
bool jazzCommons::get_cluster_port (const char* alias, int &port)
{
	for (int i = 0; i < cluster_num_nodes; i++)
	{
		if (!strcmp(alias, cluster_node[i].alias))
		{
			port = cluster_node[i].port;

			return true;
		}
	}

	string msg("Port not found. Alias not in cluster. Alias = ");
	msg = msg + alias;
	log(LOG_MISS, msg.c_str());

	return false;
}

/*	-----------------------------------------------
	  Mutex to control access to persistence
--------------------------------------------------- */

/** Request an access level to use the persistence.

	\param acmode The access mode: ACMODE_READONLY, ACMODE_READWRITE or ACMODE_SOURCECTL.
	\return		  A valid tid (>=0) on success that MUST be freed with a leave_persistence() call or -1 on error.

When an error is returned, the request will not be able to complete and http status 503 (Service Unavailable) must be returned.
*/
int jazzCommons::enter_persistence(int acmode)
{
//TODO: Implement enter_persistence.

	return 0;
}


/** Close access to the persistence for the current thread after a successful enter_persistence() call.

	\param tid A valid (internal) thread id returned by a successful enter_persistence() call.
*/
void jazzCommons::leave_persistence(int tid)
{
//TODO: Implement leave_persistence.

}

/*	-----------------------------------------------
	  R NA equivalence
--------------------------------------------------- */

inline double R_ValueOfNA()
{
	union {double d; int i[2];} na;

	na.i[1] = 0x7ff00000;
	na.i[0] = 1954;

	return na.d;
}

/*	-----------------------------------------------
	  I n s t a n t i a t i o n
--------------------------------------------------- */

jazzCommons	 jCommons;
jazzServices jServices;

double		 R_NA = R_ValueOfNA();

/*	-----------------------------------------------
	  U N I T	t e s t i n g
--------------------------------------------------- */

#if defined CATCH_TEST
TEST_CASE("Assertions on some jazzCommons sizes")
{
	REQUIRE(sizeof(jzzBlockHeader)	   == 24);
	REQUIRE(sizeof(block_C_BOOL)	   == 24);
	REQUIRE(sizeof(block_C_OFFS_CHARS) == 24);
	REQUIRE(sizeof(block_C_INTEGER)	   == 24);
	REQUIRE(sizeof(block_C_REAL)	   == 24);
	REQUIRE(sizeof(block_C_RAW)		   == 24);

	REQUIRE(sizeof(string_buffer) == 8);

	string_buffer sb;
	char * pt = (char *) &sb;

	sb.NA	 = 1;
	sb.EMPTY = 2;
	REQUIRE(pt[JAZZC_NA_STRING]	   == 1);
	REQUIRE(pt[JAZZC_EMPTY_STRING] == 2);
	sb.NA	 = 3;
	sb.EMPTY = 4;
	REQUIRE(pt[JAZZC_NA_STRING]	   == 3);
	REQUIRE(pt[JAZZC_EMPTY_STRING] == 4);

	REQUIRE(sizeof(bool) == 1);
}
#endif

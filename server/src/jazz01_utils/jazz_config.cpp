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

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;

/**< \brief	 Low level functions to support: cluster management, options, configuration, data source definition, ssh replication, etc.

	Low level functions to provide an interface for the configuration. This does not define a class, but defines methods declared in jazzCommons.
*/

#include "src/include/jazz01_commons.h"
#include "src/include/jazz.h"

/*~ end of automatic header ~*/


/** Load the server configuration from a file.

	Configuration is stored in: map<string, string> config which is private and read using the function get_config_key().
	The first key must be JazzHEAD.JAZZ_CONFIG_VERSION with value = 1.0
	Errors are logged with trace level LOG_MISS.

	\param conf The file name
	\return		True when completed ok.
*/
bool jazzCommons::load_config_file (const char *conf)
{
	ifstream fh (conf);

	if (!fh.is_open())
	{
		string msg ("In function load_config_file(): File not found ");
		msg = msg + conf;

		log(LOG_MISS, msg.c_str());

		return false;
	}

	string ln, key, val, section;
	bool firstkey = true;

	while (!fh.eof())
	{
		getline(fh, ln);

		size_t p;
		p = ln.find("//");

		if (p != string::npos) ln.erase(p, ln.length());

		p = ln.find("@SECTION");

		if (p != string::npos) section = jazz_utils::RemoveSpaceOrTab(ln.substr(p + 8, ln.length()));
		else
		{
			p = ln.find("=");

			if (p != string::npos)
			{
				key = section + "." + jazz_utils::RemoveSpaceOrTab(ln.substr(0, p - 1));
				val = jazz_utils::RemoveSpaceOrTab(ln.substr(p + 1, ln.length()));

				if (firstkey)
				{
					if (key != "JazzHEAD.JAZZ_CONFIG_VERSION" || val != "1.0")
					{
						fh.close();

						string msg ("In function load_config_file(): reading \"");
						msg = msg + conf + "\" this expects the first key to be JazzHEAD.JAZZ_CONFIG_VERSION with value = 1.0";

						log(LOG_MISS, msg.c_str());
						return false;
					}
					firstkey = false;
				}
				else
				{
					// cout << "config_put(\"" << key << "\", \"" << val << "\");" << endl;
					config[key] = val;
#ifdef DEBUG
					config_used[key] = 0;
#endif
				}
			}
		}
	}
	fh.close();

	return !section.compare("EOF");	// No errors
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched. Configuration keys are SECTION.name, e.g., "JazzHEAD.JAZZ_CONFIG_VERSION".
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool jazzCommons::get_config_key (const char *key, int &value)
{
	string keys (key);

	try
	{
		string val = config[keys];
		string::size_type extra;

		int i = stoi(val, &extra);

		if (extra != val.length()) return false;

		value = i;

#ifdef DEBUG
		config_used[keys] = 1;
#endif

		return true;
	}
	catch (...)
	{
		return false;
	}
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched. Configuration keys are SECTION.name, e.g., "JazzHEAD.JAZZ_CONFIG_VERSION".
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool jazzCommons::get_config_key (const char *key, double &value)
{
	string keys (key);

	try
	{
		string val = config[keys];
		string::size_type extra;

		double d = stod(val, &extra);

		if (extra != val.length()) return false;

		value = d;

#ifdef DEBUG
		config_used[keys] = 1;
#endif

		return true;
	}
	catch (...)
	{
		return false;
	}
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched. Configuration keys are SECTION.name, e.g., "JazzHEAD.JAZZ_CONFIG_VERSION".
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool jazzCommons::get_config_key (const char *key, string &value)
{
	string keys (key);

	string s = config[keys];

	if (!s.length()) return false;

	value = s;

#ifdef DEBUG
		config_used[keys] = 1;
#endif

	return true;
}


/** DEBUG ONLY function: Set a config key manually.

	\param key	The configuration key to be set. Configuration keys are SECTION.name, e.g., "JazzHEAD.JAZZ_CONFIG_VERSION".
	\param val	New value of the key as a string (also valid for int and double if the string can be converted).
*/
void jazzCommons::debug_config_put(const string key, const string val)
{
#ifdef NDEBUG
	log(LOG_ERROR, "Function debug_config_put() was called in release mode.");
#endif
	config[key] = val;
}

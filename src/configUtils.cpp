/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 * Copyright (c) 2017,2018 Patrick P. Frey, Andreas Baumann, Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "configUtils.hpp"
#include "defaultContants.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/platform.hpp"
#include "strus/base/numstring.hpp"
#include "strus/errorCodes.hpp"
#include "internationalization.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;
using namespace strus::webservice;

std::vector<std::string> webservice::getConfigArray( const cppcms::json::value& config, const std::string& path)
{
	std::vector<std::string> rt;
	const cppcms::json::value& configval = config.find( path);
	if (configval.is_undefined())
	{
		return rt;
	}
	else if (configval.type() == cppcms::json::is_object)
	{
		throw strus::runtime_error(_TXT("path '%s' referring to subtree and not a string or a list of strings"), path.c_str());
	}
	else if (configval.type() == cppcms::json::is_array)
	{
		cppcms::json::array ar = configval.array();
		cppcms::json::array::const_iterator ai = ar.begin(), ae = ar.end();
		for (; ai != ae; ++ai)
		{
			rt.push_back( ai->str());
		}
	}
	else
	{
		rt.push_back( configval.str());
	}
	return rt;
}

static void substConfigReferenceVariables( const cppcms::json::value& root, cppcms::json::array& value);
static void substConfigReferenceVariables( const cppcms::json::value& root, cppcms::json::object& map);
static void substConfigReferenceVariables( const cppcms::json::value& root, std::string& str);

static void substConfigReferenceVariables( const cppcms::json::value& root, cppcms::json::value& value)
{
	switch (value.type())
	{
		case cppcms::json::is_undefined:
			break;
		case cppcms::json::is_null:
			break;
		case cppcms::json::is_boolean:
			break;
		case cppcms::json::is_number:
			break;
		case cppcms::json::is_string:
			substConfigReferenceVariables( root, value.str());
			break;
		case cppcms::json::is_object:
			substConfigReferenceVariables( root, value.object());
			break;
		case cppcms::json::is_array:
			substConfigReferenceVariables( root, value.array());
			break;
	}
}

static void substConfigReferenceVariables( const cppcms::json::value& root, std::string& str)
{
	std::string res;
	char const* prev = str.c_str();
	const char* si = std::strchr( str.c_str(), '{');
	for (;si; si = std::strchr( si, '{'))
	{
		res.append( prev, si-prev);
		prev = si;
		++si;
		for (; (unsigned char)*si <= 32; ++si){}
		std::string variableref;
		for (; (unsigned char)*si > 32 && *si != '}'; ++si){variableref.push_back(*si);}
		for (; (unsigned char)*si <= 32; ++si){}
		if (*si == '}' && !variableref.empty())
		{
			res.append( root.get( variableref, ""));
			prev = ++si;
		}
		else
		{
			res.append( prev, si-prev);
			prev = si;
		}
	}
	res.append( prev);
	str = res;
}

static void substConfigReferenceVariables( const cppcms::json::value& root, cppcms::json::array& ar)
{
	cppcms::json::array::iterator vi = ar.begin(), ve = ar.end();
	for (; vi != ve; ++vi)
	{
		substConfigReferenceVariables( root, *vi);
	}
}

static void substConfigReferenceVariables( const cppcms::json::value& root, cppcms::json::object& map)
{
	cppcms::json::object::iterator mi = map.begin(), me = map.end();
	for (; mi != me; ++mi)
	{
		substConfigReferenceVariables( root, mi->second);
	}
}

cppcms::json::value webservice::configFromFile( const std::string& configfile)
{
	cppcms::json::value config;
	std::string configstr;
	int ec = strus::readFile( configfile, configstr);
	if (ec)
	{
		throw strus::runtime_error(_TXT("failed to read configuration file %s (errno %u): %s"), configfile.c_str(), ec, std::strerror(ec));
	}
	int line_number = 0;
	std::istringstream configstream( configstr);
	if (!config.load( configstream, false, &line_number))
	{
		throw strus::runtime_error(_TXT("failed to parse configuration file %s: syntax error on line %d"), configfile.c_str(), line_number);
	}
	substConfigReferenceVariables( config, config);
	return config;
}

cppcms::json::value webservice::configFromString( const std::string& configstr)
{
	cppcms::json::value config;
	int line_number = 0;
	std::istringstream configstream( configstr);
	if (!config.load( configstream, false, &line_number))
	{
		throw strus::runtime_error(_TXT("failed to parse configuration: syntax error on line %d"), line_number);
	}
	return config;
}

cppcms::json::value webservice::configDefault()
{
	int nofCores = strus::platform::cores();
	cppcms::json::value config;
	config.set( "service.api", "http");
	config.set( "service.port", 8080);
	config.set( "service.worker_threads", nofCores > 0 ? nofCores : 4 );
	config.set( "service.applications_pool_size", nofCores > 0 ? nofCores : 4 );
	config.set( "logging.level", "debug");
	config.set( "logging.file.name", DefaultConstants::SERVICE_LOG_FILE());
	config.set( "logging.file.append", true);
	config.set( "debug.request_file", DefaultConstants::REQUEST_LOG_FILE());
	return config;
}


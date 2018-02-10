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
#include "strus/errorCodes.hpp"
#include "internationalization.hpp"
#include <vector>
#include <string>
#include <cstring>

std::vector<std::string> getConfigArray( const cppcms::json::value& config, const std::string& path)
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
		std::vector<cppcms::json::value> ar = configval.array();
		std::vector<cppcms::json::value>::const_iterator ai = ar.begin(), ae = ar.end();
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

cppcms::json::value configFromFile( const std::string& configfile, int& errcode)
{
	cppcms::json::value config;
	std::string configstr;
	int ec = strus::readFile( configfile, configstr);
	if (ec)
	{
		errcode = ec;
		throw strus::runtime_error(_TXT("failed to read configuration file %s (errno %u): %s"), configfile.c_str(), ec, std::strerror(ec));
	}
	int line_number = 0;
	char const* configstr_begin = configstr.c_str();
	char const* configstr_end = configstr.c_str() + configstr.size();
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "load configuration:" << std::endl << std::string( configstr_begin, configstr_end - configstr_begin) << std::endl;
#endif
	if (!config.load( configstr_begin, configstr_end, false, &line_number))
	{
		errcode = strus::ErrorCauseSyntax;
		throw strus::runtime_error(_TXT("failed to parse configuration file %s: syntax error on line %d"), configfile.c_str(), line_number);
	}
	return config;
}

cppcms::json::value configDefault()
{
	cppcms::json::value config;
	config.set( "service.api", "http");
	config.set( "service.port", 80);
	config.set( "logging.level", "info");
	config.set( "logging.file.name", DefaultConstants::SERVICE_LOG_FILE());
	config.set( "logging.file.append", true);
	config.set( "debug.log_requests", DefaultConstants::DO_LOG_REQUESTS());
	config.set( "debug.request_file", DefaultConstants::REQUEST_LOG_FILE());
	return config;
}



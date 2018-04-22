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
/// \brief Some methods to read or instantiate a server configuration
#ifndef _STRUS_WEBSERVICE_CONFIG_IMPL_HPP_INCLUDED
#define _STRUS_WEBSERVICE_CONFIG_IMPL_HPP_INCLUDED
#include <string>
#include <cppcms/json.h>

namespace strus {
namespace webservice {

std::vector<std::string> getConfigArray( const cppcms::json::value& config, const std::string& path);
cppcms::json::value configFromFile( const std::string& configfile, int& errcode);
cppcms::json::value configDefault();
void rewriteConfigNumbers( cppcms::json::value& config);

}}//namespace strus::webservice
#endif



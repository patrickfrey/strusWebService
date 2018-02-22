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
#ifndef _STRUS_WEBSERVICE_DEFAULT_CONSTANTS_HPP_INCLUDED
#define _STRUS_WEBSERVICE_DEFAULT_CONSTANTS_HPP_INCLUDED
#include <string>

struct DefaultConstants
{
	static const char* PACKAGE() {return "strusWebService";}
	static bool DEBUG_PROTOCOL_PRETTY_PRINT() {return false;}
	static bool DEBUG_PROTOCOL_DEBUG_ENABLED() {return false;}
	static bool DEBUG_PROTOCOL_QUIT_ENABLED() {return false;}
	static const bool DO_LOG_REQUESTS() {return false;}
	static const std::string REQUEST_LOG_FILE() {return "request.log";}
	static const std::string SERVICE_LOG_FILE() {return "strusWebService.log";}
	static const int HOUSKEEPING_TIMER_INTERVAL() {return 10;}
	static const int TRANSACTION_MAX_IDLE_TIME() {return 120;}
	static const std::string CORS_AGE() {return "10";}
	static const char* HTML_DEFAULT_STYLE() {return "div {\nfont-family: verdana,arial,courier;\n}";}
};

#endif


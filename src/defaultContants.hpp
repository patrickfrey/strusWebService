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

namespace strus {
namespace webservice {

struct DefaultConstants
{
	enum {MaxUrlLength=2048};

	static const char* PACKAGE() {return "strusWebService";}
	static bool DEBUG_PROTOCOL_PRETTY_PRINT() {return false;}
	static bool DEBUG_PROTOCOL_DEBUG_ENABLED() {return false;}
	static bool DEBUG_PROTOCOL_QUIT_ENABLED() {return false;}
	static const bool DO_LOG_CALLS() {return false;}
	static const bool DO_LOG_REQUESTS() {return false;}
	static const bool DO_LOG_ACTIONS() {return false;}
	static const bool DO_LOG_CONTENTEVENTS() {return false;}
	static const int LOG_STRUCT_DEPTH() {return 4;}
	static const std::string REQUEST_LOG_FILE() {return "request.log";}
	static const std::string SERVICE_LOG_FILE() {return "strusWebService.log";}
	static const std::string LOGGING_DIR() {return "/var/log/strus";}
	static const std::string AUTOSAVE_CONFIG_DIR() {return "/srv/strus/config";}
	static const std::string HTTP_SERVER_NAME() {return "";}
	static const std::string HTTP_SCRIPT_NAME() {return "";}
	static const int HOUSKEEPING_TIMER_INTERVAL() {return 10;}
	static const int TRANSACTION_MAX_IDLE_TIME() {return 120;}
	static const int TRANSACTION_MAP_SLOT_SIZE() {return 100;}
	static const int MAX_DELEGATE_CONNECTIONS() {return 32;}
	static const std::string CORS_AGE() {return "10";}
	static const std::string DEFAULT_SERVICE_NAME() {return "strus";}
	static const char* HTML_DEFAULT_STYLE()
	{
		return "div,span {\n\tfont-family: verdana,arial,courier;\n}"
			"\ndiv {\n\tpadding:5px 15px;\n\tposition:relative;\n}"
			"\n.info {\n\tcolor: green;\n}"
			"\n.error {\n\tcolor: #e60000;\n}"
			"\n.table {\n\tdisplay: table;\n}"
			"\n.row {\n\tdisplay: table-row;\n}"
			"\n.col {\n\tdisplay: table-cell;\n}"
			"\n.list {\n\tdisplay: list;\n}"
			"\n.elem {\n\tdisplay: list-item;\n\tlist-style: none;\n}"
			"\nspan.title {\n\tcolor: #666666;\ntext-decoration: underline;\n}"
			"\nspan.name {\n\tdisplay: inline;\n\tcolor: #669999;\n}"
			"\nspan.name:after {\n\tcontent: \": \";\n}"
			"\nspan.value {\n\tdisplay: inline;\n}"
			"\nspan.value:after {\n\tcontent: '\\A';\n\twhite-space: pre\n}"
			"\n";
	}
};

}}//namespace strus::webservice
#endif


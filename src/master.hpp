/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#ifndef MASTER_HPP
#define MASTER_HPP

#include <cppcms/application.h>
#include <cppcms/json.h>

#include "version.hpp"
#include "error_codes.hpp"

#include <vector>
#include <string>
#include <cstdio>
#include <map>

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"

namespace apps {

class strusWebService;

class master : public cppcms::application {
	
	protected:
		strusWebService &service;
		bool protocol_pretty_printing;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	public:
		master( strusWebService &service );
		void register_common_pages( );
		bool get_strus_environment( const std::string &name );
		void close_strus_environment( const std::string &name );

	protected:
		void report_ok( );
		void report_ok( cppcms::json::value &j );
		void report_error( unsigned int code, const std::string &msg );
		void not_found_404( );
		bool ensure_post( );
		bool ensure_json_request( );

};

} // namespace apps

#endif

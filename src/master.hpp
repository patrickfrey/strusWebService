/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"

namespace apps {

class strusWebService;

class master : public cppcms::application {
	
	protected:
		strusWebService &service;
		bool protocol_pretty_printing;
		bool log_requests;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	public:
		master( strusWebService &service );
		void register_common_pages( );
		void register_democlient_pages( );
		bool get_strus_environment( const std::string &name );
		void close_strus_environment( const std::string &name );
		void set_pretty_printing( bool enable );
		void set_log_requests( bool enable );

	protected:
		void report_ok( );
		void report_ok( cppcms::json::value &j );
		void report_error( unsigned int code, const std::string &msg );
		void not_found_404( );
		bool ensure_post( );
		bool ensure_json_request( );
		void log_request( );
		void serve_democlient( std::string file_name ); 
		void redirect_to_democlient( );
		std::string root( ) const;

};

} // namespace apps

namespace cppcms {
	namespace json {

template<>
struct traits<std::pair<std::string, std::string> > {
	
	static std::pair<std::string, std::string> get( value const &v )
	{
		std::pair<std::string, std::string> p;

		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		p.first = v.get<std::string>( "key" );
		p.second = v.get<std::string>( "value" );
		
		return p;
	}
	
	static void set( value &v, std::pair<std::string, std::string> const &p )
	{
		v.set( "key", p.first );
		v.set( "value", p.second );
	}
};

} } // namespace cppcms::json

#endif

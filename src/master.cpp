/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "strusWebService.hpp"
#include "master.hpp"
#include "constants.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/http_content_type.h>

#include <booster/log.h>

#include "strus/errorBufferInterface.hpp"

#define BOOST_FILESYSTEM_VERSION 3

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service ), protocol_pretty_printing( false )
{
}

void master::register_democlient_pages( )
{
	// catch all redirects pointing to the democlient
	service.dispatcher( ).assign( "/democlient", &master::redirect_to_democlient, this );
	service.dispatcher( ).assign( "/democlient/", &master::redirect_to_democlient, this );

	// service built-in democlient
	service.dispatcher( ).assign( "/democlient/((index\\.html)|(js/.*\\.js)|(css/.*\\.css))", &master::serve_democlient, this, 1 );
}

void master::register_common_pages( )
{
	// catch all for missing handlers
	service.dispatcher( ).assign( ".*", &master::not_found_404, this );
}

bool master::get_strus_environment( const std::string &name )
{
	dbi = service.getDataBaseInterface( name );
	if( dbi == NULL ) {
		report_error( ERROR_INDEX_CREATE_DATABASE_INTERFACE, service.getLastStrusError( ) );
		return false;
	}

	sti = service.getStorageInterface( name );
	if( sti == NULL ) {
		report_error( ERROR_INDEX_CREATE_STORAGE_INTERFACE, service.getLastStrusError( ) );
		return false;
	}
	
	return true;
}

void master::close_strus_environment( const std::string &name )
{
	service.abortRunningTransactions( name );
	
	service.deleteQueryEvalInterface( );
	service.deleteQueryProcessorInterface( );
	service.deleteAttributeReaderInterface( name );
	service.deleteMetaDataReaderInterface( name );
	service.deleteStorageClientInterface( name );
	service.deleteStorageInterface( name );
	service.deleteDataBaseInterface( name );

	dbi = NULL;
	sti = NULL;
}

void master::set_pretty_printing( bool enable )
{
	protocol_pretty_printing = enable;
}

void master::set_log_requests( bool enable )
{
	log_requests = enable;
}
	
void master::report_error( unsigned int code, const std::string &msg )
{	
	BOOSTER_ERROR( PACKAGE ) << msg;
	
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "error";
	j["err"]["code"] = code;
	j["err"]["msg"] = msg;

	if( protocol_pretty_printing ) {
		j.save( response( ).out( ), cppcms::json::readable );
	} else {
		j.save( response( ).out( ), cppcms::json::compact );
	}
	response( ).out( ) << std::endl;
}

void master::report_ok( )
{
	cppcms::json::value j;
	report_ok( j );
}

void master::report_ok( cppcms::json::value &j )
{	
	response( ).content_type( "application/json" );
	j["result"] = "ok";
	
	if( protocol_pretty_printing ) {
		j.save( response( ).out( ), cppcms::json::readable );
	} else {
		j.save( response( ).out( ), cppcms::json::compact );
	}
	response( ).out( ) << std::endl;
}

void master::not_found_404( )
{
	report_error( 404, "Illegal URL" );
}

bool master::handle_preflight_cors( )
{
	std::vector<std::string> cors_hosts = settings( ).get<std::vector<std::string> >( "security.cors.allowed_origins" );
	std::string origin = request( ).getenv( "HTTP_ORIGIN" );
	if( origin.empty( )|| cors_hosts.size( ) == 0 ) {
		// no Origin header, let's assume we can continue
		return true;
	}
	std::vector<std::string>::const_iterator it, end = cors_hosts.end( );
	for( it = cors_hosts.begin( ); it != end; it++ ) {
		if( *it == origin ) {
			// though Access-Control-Allow-Origin should allow a
			// list of space separated hosts, in practive only
			// echoing the origin Origin host works
			response( ).content_type( "application/json" );
			response( ).set_header( "Access-Control-Allow-Method", "GET, POST" );
			response( ).set_header( "Access-Control-Allow-Origin", origin );
			response( ).set_header( "Access-Control-Allow-Headers", "content-type" );
			int age = settings( ).get<int>( "security.cors.age" );
			std::ostringstream ss;
			ss << age;
			response( ).set_header( "Access-Control-Max-Age", ss.str( ) );
			return true;
		}
	}

	// drop out of normal processing because we are in the accepted
	// preflight check. When the allow origin is set, the browser will
	// act accordingly and come back with the original request
	return false;
}

bool master::ensure_post( )
{
	if( request( ).request_method( ) != "POST" ) {
		report_error( ERROR_ILLEGAL_METHOD, "Expecting HTTP method 'POST'" );
		return false;
	}
	return true;
}

bool master::ensure_json_request( )
{
	cppcms::http::content_type content_type = request( ).content_type_parsed( );
	std::string type = content_type.type( );
	std::string subtype = content_type.subtype( );
	if( type != "application" || subtype != "json" ) {
		report_error( ERROR_IILLEGAL_JSON, "Expecting the content type of the body of the request to be 'application/json'" );
		return false;
	}
	return true;
}

void master::log_request( )
{
	if( log_requests ) {
		std::ofstream *os = service.log_request_stream( );

		std::string request_method = request( ).request_method( );
		*os << request_method << std::endl;
		// TODO: how to find whether we are secure or not? later..
		*os << "http" << "://"
			<< request( ).http_host( ) << request( ).script_name( )
			<< request( ).path_info( ) << std::endl;
		if( request_method == "GET" ) {
			*os << std::endl;
		} else if( request_method == "POST" ) {
			std::pair<void *, size_t> data = request( ).raw_post_data( );
			std::string payload = std::string( reinterpret_cast<char const *>( data.first ), data.second );
			*os << payload << std::endl;
		}			
	}
}

static bool endsWith( const std::string &s, const std::string &end )
{
	if( s.length( ) >= end.length( ) ) {
		return s.compare( s.length( ) - end.length( ), end.length( ), end ) == 0;
	} else {
		return false;
	}
}

void master::serve_democlient( std::string file_name )
{
	std::string full_file_name = service.settings( ).get<std::string>( "democlient.basedir" ) + "/" + file_name;
	std::ifstream f( full_file_name.c_str( ) );
	if( !f ) {
		not_found_404( );
		return;
	}

	if( endsWith( file_name, ".css" ) ) {
		response( ).content_type( "text/css" );
	} else if( endsWith( file_name, ".js" ) ) {
		response( ).content_type( "application/javascript" );
	} else if( endsWith( file_name, ".html" ) ) {
		response( ).content_type( "text/html" );
	} else {
		response( ).content_type( "application/octet-stream" );
	}

	response( ).content_length( boost::filesystem::file_size( full_file_name ) );	
	response( ).out( ) << f.rdbuf( );
}

void master::redirect_to_democlient( )
{
	response( ).set_redirect_header( root( ) +"/democlient/index.html" );
}

std::string master::root( ) const
{
	return service.settings( ).get<std::string>( "http.script" );
}
	
} // namespace apps

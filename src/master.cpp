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

#include <fstream>

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service )
{
	protocol_pretty_printing = service.settings( ).get<bool>( "protocol.pretty_print", DEFAULT_PROTOCOL_PRETTY_PRINT );
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
	service.deleteQueryEvalInterface( );
	service.deleteQueryProcessorInterface( );
	service.deleteAttributeReaderInterface( name );
	service.deleteMetaDataReaderInterface( name );
	service.deleteStorageClientInterface( name );
	service.deleteDatabaseClientInterface( name );	
	service.deleteStorageInterface( name );
	service.deleteDataBaseInterface( name );

	dbi = NULL;
	sti = NULL;
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
	std::ifstream f( ( service.settings( ).get<std::string>( "democlient.basedir" ) + "/" + file_name ).c_str( ) );
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
	
	response( ).out( ) << f.rdbuf( );
}

void master::redirect_to_democlient( )
{
	response( ).set_redirect_header( "/democlient/index.html" );
}
	
} // namespace apps

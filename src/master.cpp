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

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service )
{
	protocol_pretty_printing = service.settings( ).get<bool>( "protocol.pretty_print", DEFAULT_PROTOCOL_PRETTY_PRINT );
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
	std::vector<std::string> errors = service.getStrusErrorDetails( );
	
	for( std::vector<std::string>::const_iterator it = errors.begin( ); it != errors.end( ); it++ ) {
		BOOSTER_ERROR( PACKAGE ) << *it;
	}
	
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "error";
	j["err"]["code"] = code;
	j["err"]["msg"] = msg;
	if( !errors.empty( ) ) {
		unsigned int pos = 0;
		for( std::vector<std::string>::const_iterator it = errors.begin( ); it != errors.end( ); it++, pos++ ) {
			j["err"]["details"][pos] = *it;
		}
	}
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
	std::vector<std::string> errors = service.getStrusErrorDetails( );
	
	response( ).content_type( "application/json" );
	j["result"] = "ok";
	if( !errors.empty( ) ) {
		unsigned int pos = 0;
		for( std::vector<std::string>::const_iterator it = errors.begin( ); it != errors.end( ); it++, pos++ ) {
			j["err"]["details"][pos] = *it;
		}
	}
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
	
} // namespace apps

#include "query.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include "strus/queryInterface.hpp"

namespace apps {

query::query( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/query/(\\w++)/(\\w+)", &query::query_url_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/query/(\\w++)", &query::query_payload_cmd, this, 1 );
}

void query::query_url_cmd( const std::string name, const std::string qry )
{
	query_cmd( name, qry, true );
}

void query::query_payload_cmd( const std::string name )
{
	query_cmd( name, "", false );
}
	
void query::query_cmd( const std::string name, const std::string qry, bool query_in_url )
{	
	if( !query_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
		return;
	}
	
	struct QueryRequest qry_req; 

	if( !query_in_url ) {
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "query" ) == cppcms::json::is_object ) {
			try {
				qry_req = p.get<struct QueryRequest>( "query" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Illegal JSON document payload received" );
				return;
			}
		} else {
			report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
			return;
		}
	}
	
	get_strus_environment( name );

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_QUERY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_QUERY_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_QUERY_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::QueryEvalInterface *query_eval = service.getQueryEvalInterface( name );
	
	strus::QueryInterface *query = query_eval->createQuery( storage );
	
	delete query;
}

} // namespace apps

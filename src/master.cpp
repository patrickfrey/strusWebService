#include "strusWebService.hpp"
#include "master.hpp"
#include "constants.hpp"

#include <cppcms/service.h>
#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/http_content_type.h>

#include <booster/log.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/lib/storage.hpp"

#include <boost/algorithm/string.hpp>

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service )
{
	// TODO: number of threads, depends on CppCMS running model, must pass
	// parts of this config here
	// TODO: stderr, really? how can we redirect to the booster log?
	// TODO: do this once and not here (master?)
	// implementing a ErrorBufferInterface is not really possible!
	// redirecting the output to a tmpfile and reading and rewinding that
	// one periodically seems to be the only hacky option right now
	logfile = std::tmpfile( );
	
	unsigned int nof_threads;
	if( service.service( ).procs_no( ) == 0 ) {
		nof_threads = service.service( ).threads_no( );
	} else {
		nof_threads = service.service( ).procs_no( ) * service.service( ).threads_no( );
	}
	BOOSTER_DEBUG( PACKAGE ) << "Using '" << nof_threads << "' threads for strus logging buffers";
	g_errorhnd = strus::createErrorBuffer_standard( logfile, nof_threads );
}

void master::register_common_pages( )
{
	// catch all for missing handlers
	service.dispatcher( ).assign( ".*", &master::not_found_404, this );
}

void master::prepare_strus_environment( )
{	
	dbi = strus::createDatabase_leveldb( g_errorhnd );
	if( dbi == NULL ) {
		report_error( ERROR_INDEX_CREATE_DATABASE_INTERFACE, g_errorhnd->fetchError( ) );
		return;
	}

	sti = strus::createStorage( g_errorhnd );
	if( sti == NULL ) {
		report_error( ERROR_INDEX_CREATE_STORAGE_INTERFACE, g_errorhnd->fetchError( ) );
		return;
	}
}

void master::close_strus_environment( )
{
	delete dbi;
	delete sti;
}

std::vector<std::string> master::handle_strus_errors( )
{
	char buf[512];
	std::vector<std::string> v;
	
	long pos = ftell( logfile );
	if( pos > 0 ) {
		fseek( logfile, 0 , SEEK_SET );
		while( fgets( buf, sizeof( buf ), logfile ) != NULL ) {
			std::string s( buf );
			boost::trim_right_if( s, boost::is_any_of( "\r\n" ) );
			BOOSTER_ERROR( PACKAGE_STRUS ) << s;
			v.push_back( s );
		}
	}
	
	return v;
}

void master::report_error( unsigned int code, const std::string &msg )
{
	std::vector<std::string> errors = handle_strus_errors( );
	
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
	response( ).out( ) << j << std::endl;
}

void master::report_ok( )
{
	cppcms::json::value j;
	report_ok( j );
}

void master::report_ok( cppcms::json::value &j )
{
	std::vector<std::string> errors = handle_strus_errors( );
	
	response( ).content_type( "application/json" );
	j["result"] = "ok";
	if( !errors.empty( ) ) {
		unsigned int pos = 0;
		for( std::vector<std::string>::const_iterator it = errors.begin( ); it != errors.end( ); it++, pos++ ) {
			j["details"][pos] = *it;
		}
	}
	response( ).out( ) << j << std::endl;
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
		return false;;
	}
	return true;
}
	
} // namespace apps

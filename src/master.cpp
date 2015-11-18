#include "strusWebService.hpp"
#include "master.hpp"

#include <cppcms/service.h>
#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/lib/storage.hpp"

// TODO: read from config
#define NOF_THREADS 128

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
	g_errorhnd = strus::createErrorBuffer_standard( logfile, NOF_THREADS );
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

void master::report_error( unsigned int code, const std::string &msg )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "error";
	j["err_code"] = code;
	j["err_msg"] = msg;
	response( ).out( ) << j << std::endl;
}

void master::report_ok( )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	response( ).out( ) << j << std::endl;
}

void master::not_found_404( )
{
	report_error( 404, "No API found on this URL." );
}

} // namespace apps

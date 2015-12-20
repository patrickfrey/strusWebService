#include <cppcms/service.h>
#include <cppcms/applications_pool.h>
#include <booster/log.h>

#include <signal.h>

#include "strusWebService.hpp"

static bool terminate = false;
static bool got_sighup = false;
static cppcms::service *global_srv = 0;

static void handle_signal( int sig )
{
	switch( sig ) {
		case SIGHUP:
			got_sighup = true;
			if( global_srv ) global_srv->shutdown( );
			break;
		
		default:
			// unknown signal, ignore
			break;
	}
}

int main( int argc, char *argv[] )
{
	signal( SIGHUP, handle_signal );
	while( !terminate ) {
		cppcms::service srv( argc, argv );
		global_srv = &srv;
		
		try {
			BOOSTER_INFO( "strusCms" ) << "Restarting strus web service..";
			
			StrusContext *strusContext = new StrusContext( );
			srv.applications_pool( ).mount( cppcms::applications_factory<apps::strusWebService, StrusContext *>( strusContext ) );
	
			srv.run( );
			
			delete strusContext;

			if( got_sighup ) {
				BOOSTER_INFO( "strusWebService" ) << "Reloading configuration on SIGHUP..";
				got_sighup = false;
			} else {
				terminate = true;
			}
			
		} catch( std::exception const &e ) {
			BOOSTER_ERROR( "strusWebService" ) << e.what() ;
			srv.shutdown( );
			continue;
		}

		srv.shutdown( );		
	}
	
	return 0;
}

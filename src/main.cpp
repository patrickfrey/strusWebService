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

			unsigned int nof_threads;
			if( srv.procs_no( ) == 0 ) {
				nof_threads = srv.threads_no( );
			} else {
				nof_threads = srv.procs_no( ) * srv.threads_no( );
			}
			BOOSTER_DEBUG( PACKAGE ) << "Using '" << nof_threads << "' threads for strus logging buffers";
			
			StrusContext *strusContext = new StrusContext( nof_threads,
				srv.settings( ).get<std::string>( "extensions.directory" ), 
				srv.settings( ).get<std::vector<std::string> >( "extensions.modules" ) );
				
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

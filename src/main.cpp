/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cppcms/service.h>
#include <cppcms/applications_pool.h>

#include <booster/log.h>

#include <signal.h>

#include "strusWebService.hpp"

#include "version.hpp"
#include "boost/program_options.hpp" 

static bool terminate = false;
static bool got_sighup = false;
static cppcms::service *srv = 0;

static void handle_signal( int sig )
{
	switch( sig ) {
		case SIGHUP:
			got_sighup = true;
			if( srv ) srv->shutdown( );
			break;
		
		default:
			// unknown signal, ignore
			break;
	}
}

int main( int argc, char *argv[] )
{
    boost::program_options::options_description desc( "Options" ); 
    desc.add_options( ) 
      ( "help,h", "Print help page" ) 
      ( "version,V", "Print version" )
      ( "verbose,v", "Enable verbose output" )
      ( "pretty-print,p", "Enable pretty-printing of JSON" )
      ( "config,c", "Configuraton file" );
	boost::program_options::variables_map vm;
	
	try {
		boost::program_options::store( boost::program_options::command_line_parser( argc, argv )
			.options( desc ).run( ), vm );
			
		if( vm.count( "help" ) ) {
			std::cout << desc;
			return 0;
		}
		
		if( vm.count( "version" ) ) {
			std::cout << PACKAGE << " " << STRUS_WEB_SERVICE_VERSION_STRING
				<< ", strus version " << STRUS_STORAGE_VERSION_STRING << std::endl;
			return 0;
		}
		
		boost::program_options::notify( vm );
	
	} catch( boost::program_options::required_option &e ) {
		std::cerr << "ERROR: " << e.what( ) << std::endl << std::endl;
		std::cerr << desc;
		return 1;
	} catch( boost::program_options::error &e ) {
		std::cerr << "ERROR: " << e.what( ) << std::endl << std::endl;
		std::cerr << desc;
		return 1;
	}
	
	if( !vm.count( "config" ) ) {
		std::cerr << "ERROR: configuration file and -c option expected" << std::endl << std::endl;
		std::cerr << desc;
		return 1;
	}
      
	signal( SIGHUP, handle_signal );
	while( !terminate ) {
		try {
            srv = new cppcms::service( argc, argv );

            if( vm.count( "verbose" ) ) {
                booster::log::logger::instance( ).set_default_level( booster::log::logger::string_to_level( "debug" ) );
            }
		
			BOOSTER_INFO( PACKAGE ) << "Restarting strus web service..";

			unsigned int nof_threads;
			if( srv->procs_no( ) == 0 ) {
				nof_threads = srv->threads_no( );
			} else {
				nof_threads = srv->procs_no( ) * srv->threads_no( );
			}
			BOOSTER_DEBUG( PACKAGE ) << "Using '" << nof_threads << "' threads for strus logging buffers";
			
			StrusContext *strusContext = new StrusContext( nof_threads,
				srv->settings( ).get<std::string>( "extensions.directory" ), 
				srv->settings( ).get<std::vector<std::string> >( "extensions.modules" ) );
			
			srv->applications_pool( ).mount( cppcms::applications_factory<apps::strusWebService, StrusContext *, bool>( strusContext, vm.count( "pretty-print" ) ) );
	
			srv->run( );
			
			delete strusContext;

			if( got_sighup ) {
				BOOSTER_INFO( PACKAGE ) << "Reloading configuration on SIGHUP..";
				got_sighup = false;
			} else {
				terminate = true;
			}

            srv->shutdown( );
            delete srv;		
			
		} catch( std::exception const &e ) {
			if( srv != 0 ) {
                BOOSTER_ERROR( PACKAGE ) << e.what() ;
                srv->shutdown( );
                delete srv;
            } else {
                std::cerr << "FATAL: Fatal error on startup: " << e.what( ) << std::endl;
            }
			return 1;
		}

	}
	
	return 0;
}

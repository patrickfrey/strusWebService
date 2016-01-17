/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann <mail@andreasbaumann.cc>
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#include "strusContext.hpp"
#include "version.hpp"

#include <unistd.h>

#include <boost/algorithm/string.hpp>

#include <booster/log.h>

#include <boost/filesystem.hpp>

#include "strus/storageModule.hpp"

StrusContext::StrusContext( unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules_ )
	: context_map( ), modules( )
{
	// implementing a ErrorBufferInterface is not really possible!
	// redirecting the output to a tmpfile and reading and rewinding that
	// one periodically seems to be the only hacky option right now
	logfile = std::tmpfile( );
	
	errorhnd = strus::createErrorBuffer_standard( logfile, nof_threads );
	
	BOOSTER_DEBUG( PACKAGE ) << "Search directory for modules implementing extensions is '" << moduleDir << "'";	
	
	for( std::vector<std::string>::const_iterator it = modules_.begin( ); it != modules_.end( ); it++ ) {
		boost::filesystem::path path( moduleDir );
		boost::filesystem::path fullPath = boost::filesystem::absolute( path ) /= *it;
		BOOSTER_DEBUG( PACKAGE ) << "Loading module '" << fullPath.string( ) << "'";
		
		strus::ModuleEntryPoint::Status status;
		const strus::ModuleEntryPoint *entrypoint = strus::loadModuleEntryPoint( fullPath.string( ).c_str( ), status );
		if( !entrypoint ) {
			BOOSTER_WARNING( PACKAGE ) << "failed loading extension module '" << fullPath.string( ) << "': " << status.errormsg;
			continue;
		}

		if( entrypoint->type != strus::ModuleEntryPoint::Storage ) {
			BOOSTER_WARNING( PACKAGE ) << "module '" << fullPath.string( ) << "' is not a storage module and is not loaded into the server";
			continue;
		}

		modules.push_back( entrypoint );
	}
}

void StrusContext::registerModules( strus::QueryProcessorInterface *qpi ) const
{
	for( std::vector<const strus::ModuleEntryPoint *>::const_iterator it = modules.begin( ); it != modules.end( ); it++ ) {
		const strus::StorageModule *module = reinterpret_cast<const strus::StorageModule *>( *it );
		if( module->weightingFunctionConstructor ) {
			strus::WeightingFunctionConstructor const *constructor = module->weightingFunctionConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::WeightingFunctionInterface *func = constructor->create( errorhnd );
				if( !func ) {
					BOOSTER_WARNING( PACKAGE ) << "weighting function '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->defineWeightingFunction( constructor->name, func );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering weighting function '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}
		
		if( module->summarizerFunctionConstructor ) {
			strus::SummarizerFunctionConstructor const *constructor = module->summarizerFunctionConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::SummarizerFunctionInterface *func = constructor->create( errorhnd );
				if( !func ) {
					BOOSTER_WARNING( PACKAGE ) << "summarizer function '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->defineSummarizerFunction( constructor->name, func );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering summarizer function '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}
		
		if( module->postingIteratorJoinConstructor ) {
			strus::PostingIteratorJoinConstructor const *constructor = module->postingIteratorJoinConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::PostingJoinOperatorInterface *op = constructor->create( errorhnd );
				if( !op ) {
					BOOSTER_WARNING( PACKAGE ) << "posting join operator '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->definePostingJoinOperator( constructor->name, op );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering posting join operator '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}					
	} 
}

StrusIndexContext *StrusContext::acquire( const std::string &name )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return 0;
	} else {
		return it->second;
	}
	mutex.lock( );
}

void StrusContext::release( const std::string &name, StrusIndexContext *ctx )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		context_map[name] = ctx;
	} else {
		it->second = ctx;
	}
	mutex.unlock( );
}

std::vector<std::string> StrusContext::getStrusErrorDetails( ) const
{
	char buf[512];
	std::vector<std::string> v;
	
	long pos = ftell( logfile );
	if( pos > 0 ) {
		fseek( logfile, 0 , SEEK_SET );
		while( fgets( buf, sizeof( buf ), logfile ) != 0 ) {
			std::string s( buf );
			boost::trim_right_if( s, boost::is_any_of( "\r\n" ) );
			v.push_back( s );
		}
		(void)ftruncate( fileno( logfile ), 0 );
		fseek( logfile, 0 , SEEK_SET );
	}
	
	return v;
}

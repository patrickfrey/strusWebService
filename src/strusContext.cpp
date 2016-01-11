#include "strusContext.hpp"
#include "version.hpp"

#include <unistd.h>

#include <boost/algorithm/string.hpp>

#include <booster/log.h>

#include <boost/filesystem.hpp>
#include "strus/moduleEntryPoint.hpp"
#include "strus/storageModule.hpp"

StrusContext::StrusContext( unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules )
	: context_map( ), weighting_func_map( )
{
	// implementing a ErrorBufferInterface is not really possible!
	// redirecting the output to a tmpfile and reading and rewinding that
	// one periodically seems to be the only hacky option right now
	logfile = std::tmpfile( );
	
	g_errorhnd = strus::createErrorBuffer_standard( logfile, nof_threads );
	
	BOOSTER_DEBUG( PACKAGE ) << "Search directory for modules implementing extensions is '" << moduleDir << "'";	
	
	for( std::vector<std::string>::const_iterator it = modules.begin( ); it != modules.end( ); it++ ) {
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
		
		const strus::StorageModule *module = reinterpret_cast<const strus::StorageModule *>( entrypoint );
		
		if( module->weightingFunctionConstructor ) {
			strus::WeightingFunctionConstructor const *constructor = module->weightingFunctionConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::WeightingFunctionInterface *func = constructor->create( g_errorhnd );
				if( !func ) {
					BOOSTER_WARNING( PACKAGE ) << "weighting function cannot be constructed";
					continue;
				}
				weighting_func_map[constructor->name] = func;
			}
		}
	}
}

void StrusContext::registerModules( strus::QueryProcessorInterface *qpi ) const
{
	for( std::map<std::string, strus::WeightingFunctionInterface *>::const_iterator it = weighting_func_map.begin( ); it != weighting_func_map.end( ); it++ ) {
		qpi->defineWeightingFunction( it->first, it->second );
		if( g_errorhnd->hasError( ) ) {
			BOOSTER_WARNING( PACKAGE ) << "registering weighting function '" <<
				it->first << "' resulted in an error: " << it->second;
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

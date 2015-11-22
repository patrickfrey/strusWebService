#include "strusWebService.hpp"

#include <booster/log.h>

#include <cppcms/service.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"

#include <boost/algorithm/string.hpp>
#include <booster/locale/format.h>

#include <sstream>

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	other( *this ),
	index( *this, settings( ).get<std::string>( "storage.basedir" ) ),
	document( *this ),
	query( *this )
{
	add( master );
	add( other );
	add( index );
	add( document );
	add( query );
			
	master.register_common_pages( );

	// implementing a ErrorBufferInterface is not really possible!
	// redirecting the output to a tmpfile and reading and rewinding that
	// one periodically seems to be the only hacky option right now
	logfile = std::tmpfile( );
	
	unsigned int nof_threads;
	if( service( ).procs_no( ) == 0 ) {
		nof_threads = service( ).threads_no( );
	} else {
		nof_threads = service( ).procs_no( ) * service( ).threads_no( );
	}
	BOOSTER_DEBUG( PACKAGE ) << "Using '" << nof_threads << "' threads for strus logging buffers";
	g_errorhnd = strus::createErrorBuffer_standard( logfile, nof_threads );
}

strus::DatabaseInterface *strusWebService::getDataBaseInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		struct strusContext context;
		memset( &context, 0, sizeof( context ) );
		context_map[name] = context;
	}
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.dbi == NULL ) {
		strus::DatabaseInterface *dbi = strus::createDatabase_leveldb( g_errorhnd );
		if( dbi == NULL ) {
			return NULL;
		}
		(*it).second.dbi = dbi;
	}
	return (*it).second.dbi;	
}

strus::StorageInterface *strusWebService::getStorageInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		struct strusContext context;
		memset( &context, 0, sizeof( context ) );
		context_map[name] = context;
	}
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.sti == NULL ) {
		strus::StorageInterface *sti = strus::createStorage( g_errorhnd );
		if( sti == NULL ) {
			return NULL;
		}
		(*it).second.sti = sti;
	}
	return (*it).second.sti;
}

void strusWebService::deleteDataBaseInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.dbi != NULL ) {
		delete (*it).second.dbi;
		(*it).second.dbi = NULL;
	}
}

void strusWebService::deleteStorageInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.sti != NULL ) {
		delete (*it).second.sti;
		(*it).second.sti = NULL;
	}
}

std::string strusWebService::getLastStrusError( )
{
	return g_errorhnd->fetchError( );
}

std::vector<std::string> strusWebService::getStrusErrorDetails( )
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

std::string strusWebService::getStorageDirectory( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "{1}/{2}" ) % base_storage_dir % name;
	std::string directory = ss.str( );

	return directory;
}

std::string strusWebService::getStorageConfig( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name )
{
	std::ostringstream ss;
	std::ostringstream ss2;
	bool first = true;
	std::vector<struct MetadataDefiniton>::const_iterator it;
	
	for( it = params.metadata.begin( ); it != params.metadata.end( ); it++ ) {
		if( !first ) {
			ss2 << ", ";		
		} else {
			first = false;
		}
		ss2 << it->name << " " << it->type;
	}
	
	ss << booster::locale::format( "database={1}; path={2}; compression={3}; cache={4}; max_open_files={5}; write_buffer_size={6}; block_size={7}; metadata={8}" )
		% params.database
		% getStorageDirectory( base_storage_dir, name )
		% ( params.compression ? "yes" : "no" )
		% params.cache_size
		% params.max_open_files
		% params.write_buffer_size
		% params.block_size
		% ss2.str( );
	std::string config = ss.str( );

	BOOSTER_DEBUG( PACKAGE ) << "Storage config string: " << config;
	
	return config;
}

} // namespace apps

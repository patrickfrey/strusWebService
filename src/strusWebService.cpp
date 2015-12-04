#include "strusWebService.hpp"

#include <booster/log.h>

#include <cppcms/service.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"

#include <boost/algorithm/string.hpp>
#include <booster/locale/format.h>

#include <sstream>

#include <unistd.h>

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	storage_base_directory( settings( ).get<std::string>( "storage.basedir" ) ),
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

void strusWebService::getOrCreateStrusContext( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		struct strusContext context( getStorageConfig( storage_base_directory, name ) );
		context_map[name] = context;
	}
}

void strusWebService::registerStorageConfig( const std::string &name, const std::string &config )
{
	// TODO: actually we should rather throw here
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	(*it).second.config = config;
}

strus::DatabaseInterface *strusWebService::getDataBaseInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
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
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
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

strus::DatabaseClientInterface *strusWebService::getDatabaseClientInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.dbci == NULL ) {
		strus::DatabaseInterface *dbi = (*it).second.dbi;		
		std::string config = (*it).second.config;
		strus::DatabaseClientInterface *dbci = dbi->createClient( config );
		if( dbci == NULL ) {
			return NULL;
		}
		(*it).second.dbci = dbci;
	}
	return (*it).second.dbci;
}

strus::StorageClientInterface *strusWebService::getStorageClientInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.stci == NULL ) {
		std::string config = (*it).second.config;
		strus::StorageInterface *sti = (*it).second.sti;		
		strus::DatabaseClientInterface *dbci = (*it).second.dbci;		
		strus::StorageClientInterface *stci = sti->createClient( config, dbci );
		if( stci == NULL ) {
			return NULL;
		}
		(*it).second.stci = stci;
	}
	return (*it).second.stci;
}

strus::MetaDataReaderInterface *strusWebService::getMetaDataReaderInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.mdri == NULL ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::MetaDataReaderInterface *mdri = stci->createMetaDataReader( );
		if( mdri == NULL ) {
			return NULL;
		}
		(*it).second.mdri = mdri;
	}
	return (*it).second.mdri;
}

strus::AttributeReaderInterface *strusWebService::getAttributeReaderInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.atri == NULL ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::AttributeReaderInterface *atri = stci->createAttributeReader( );
		if( atri == NULL ) {
			return NULL;
		}
		(*it).second.atri = atri;
	}
	return (*it).second.atri;
}

strus::StorageTransactionInterface *strusWebService::getStorageTransactionInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.stti == NULL ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::StorageTransactionInterface *stti = stci->createTransaction( );
		if( stti == NULL ) {
			return NULL;
		}
		(*it).second.stti = stti;
	}
	return (*it).second.stti;
}

strus::QueryEvalInterface *strusWebService::getQueryEvalInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	if( (*it).second.stti == NULL ) {
		// TODO:  no queryeval.h, how to create a QueryEvalInterface if not over
		// the object builder (but the should I rewrite everything to use
		// object builder?) Why should in this case there be two interfaces?
		//~ strus::QueryEvalInterface *qei = strus::createQueryEval( g_errorhnd );
		strus::QueryEvalInterface *qei = NULL;
		if( qei == NULL ) {
			return NULL;
		}
		(*it).second.qei = qei;
	}
	return (*it).second.qei;
}

std::string strusWebService::getConfigString( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return NULL;
	}
	return (*it).second.config;
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

void strusWebService::deleteDatabaseClientInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.dbci != NULL ) {
		delete (*it).second.dbci;
		(*it).second.dbci = NULL;
	}	
}

void strusWebService::deleteStorageClientInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.stci != NULL ) {
		delete (*it).second.stci;
		(*it).second.stci = NULL;
		// deletes implicitly also dbci?!
		// TODO: I thought I pass a database reference to the storage object?
		// For now we just set dbci to NULL too without deleting it
		(*it).second.dbci = NULL;
	}	
}

void strusWebService::deleteMetaDataReaderInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.mdri != NULL ) {
		delete (*it).second.mdri;
		(*it).second.mdri = NULL;
	}	
}

void strusWebService::deleteAttributeReaderInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.atri != NULL ) {
		delete (*it).second.atri;
		(*it).second.atri = NULL;
	}	
}

void strusWebService::deleteStorageTransactionInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.stti != NULL ) {
		delete (*it).second.stti;
		(*it).second.stti = NULL;
	}	
}

void strusWebService::deleteQueryEvalInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.qei != NULL ) {
		delete (*it).second.qei;
		(*it).second.qei = NULL;
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
		(void)ftruncate( fileno( logfile ), 0 );
		fseek( logfile, 0 , SEEK_SET );
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

std::string strusWebService::getStorageConfig( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "path={1}" )
		% getStorageDirectory( base_storage_dir, name );
	std::string config = ss.str( );

	BOOSTER_DEBUG( PACKAGE ) << "Simple storage config string: " << config;
	
	return config;	
}

} // namespace apps

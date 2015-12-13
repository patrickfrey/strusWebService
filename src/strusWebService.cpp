#include "strusWebService.hpp"

#include <booster/log.h>

#include <cppcms/service.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"

#include <boost/algorithm/string.hpp>
#include <booster/locale/format.h>

#include <sstream>

#include <unistd.h>

#include "strus/lib/queryeval.hpp"
#include "strus/lib/queryproc.hpp"

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
		return 0;
	}
	if( (*it).second.dbi == 0 ) {
		strus::DatabaseInterface *dbi = strus::createDatabase_leveldb( g_errorhnd );
		if( dbi == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.sti == 0 ) {
		strus::StorageInterface *sti = strus::createStorage( g_errorhnd );
		if( sti == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.dbci == 0 ) {
		strus::DatabaseInterface *dbi = (*it).second.dbi;		
		std::string config = (*it).second.config;
		strus::DatabaseClientInterface *dbci = dbi->createClient( config );
		if( dbci == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.stci == 0 ) {
		std::string config = (*it).second.config;
		strus::StorageInterface *sti = (*it).second.sti;		
		strus::DatabaseClientInterface *dbci = (*it).second.dbci;		
		strus::StorageClientInterface *stci = sti->createClient( config, dbci, 0 );
		if( stci == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.mdri == 0 ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::MetaDataReaderInterface *mdri = stci->createMetaDataReader( );
		if( mdri == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.atri == 0 ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::AttributeReaderInterface *atri = stci->createAttributeReader( );
		if( atri == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.stti == 0 ) {
		strus::StorageClientInterface *stci = (*it).second.stci;
		strus::StorageTransactionInterface *stti = stci->createTransaction( );
		if( stti == 0 ) {
			return 0;
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
		return 0;
	}
	if( (*it).second.stti == 0 ) {
		strus::QueryEvalInterface *qei = strus::createQueryEval( g_errorhnd );
		if( qei == 0 ) {
			return 0;
		}
		(*it).second.qei = qei;
	}
	return (*it).second.qei;
}

strus::QueryProcessorInterface *strusWebService::getQueryProcessorInterface( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return 0;
	}
	if( (*it).second.stti == 0 ) {
		strus::QueryProcessorInterface *qpi = strus::createQueryProcessor( g_errorhnd );
		if( qpi == 0 ) {
			return 0;
		}
		(*it).second.qpi = qpi;
	}
	return (*it).second.qpi;	
}

strus::QueryProcessorInterface *strusWebService::getQueryProcessorInterface( )
{
	strus::QueryProcessorInterface *qpi = strus::createQueryProcessor( g_errorhnd );
	return qpi;
}

std::string strusWebService::getConfigString( const std::string &name )
{
	getOrCreateStrusContext( name );
	std::map<std::string, struct strusContext>::iterator it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return 0;
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
	if( (*it).second.dbi != 0 ) {
		delete (*it).second.dbi;
		(*it).second.dbi = 0;
	}
}

void strusWebService::deleteStorageInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.sti != 0 ) {
		delete (*it).second.sti;
		(*it).second.sti = 0;
	}
}

void strusWebService::deleteDatabaseClientInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.dbci != 0 ) {
		delete (*it).second.dbci;
		(*it).second.dbci = 0;
	}	
}

void strusWebService::deleteStorageClientInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.stci != 0 ) {
		delete (*it).second.stci;
		(*it).second.stci = 0;
		// deletes implicitly also dbci?!
		// TODO: I thought I pass a database reference to the storage object?
		// For now we just set dbci to 0 too without deleting it
		(*it).second.dbci = 0;
	}	
}

void strusWebService::deleteMetaDataReaderInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.mdri != 0 ) {
		delete (*it).second.mdri;
		(*it).second.mdri = 0;
	}	
}

void strusWebService::deleteAttributeReaderInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.atri != 0 ) {
		delete (*it).second.atri;
		(*it).second.atri = 0;
	}	
}

void strusWebService::deleteStorageTransactionInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.stti != 0 ) {
		delete (*it).second.stti;
		(*it).second.stti = 0;
	}	
}

void strusWebService::deleteQueryEvalInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.qei != 0 ) {
		delete (*it).second.qei;
		(*it).second.qei = 0;
	}	
}

void strusWebService::deleteQueryProcessorInterface( const std::string &name )
{
	std::map<std::string, struct strusContext>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return;
	}
	if( (*it).second.qpi != 0 ) {
		delete (*it).second.qpi;
		(*it).second.qpi = 0;
	}	
}

void strusWebService::deleteQueryProcessorInterface( )
{
	if( qpi != 0 ) {
		delete qpi;
		qpi = 0;
	}
}

bool strusWebService::hasError( ) const
{
	return g_errorhnd->hasError( );
}

std::string strusWebService::getLastStrusError( ) const
{
	return g_errorhnd->fetchError( );
}

std::vector<std::string> strusWebService::getStrusErrorDetails( ) const
{
	char buf[512];
	std::vector<std::string> v;
	
	long pos = ftell( logfile );
	if( pos > 0 ) {
		fseek( logfile, 0 , SEEK_SET );
		while( fgets( buf, sizeof( buf ), logfile ) != 0 ) {
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

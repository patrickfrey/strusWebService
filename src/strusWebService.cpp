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

strusWebService::strusWebService( cppcms::service &srv, StrusContext *_context )
	: cppcms::application( srv ), context( _context ),
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

StrusConnectionContext *strusWebService::getOrCreateStrusContext( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		ctx = new StrusConnectionContext( getStorageConfig( storage_base_directory, name ) );
	}
	context->release( name, ctx );
	return ctx;
}

void strusWebService::registerStorageConfig( const std::string &name, const std::string &config )
{
	// TODO: actually we should rather throw here (why?)
	StrusConnectionContext *ctx = getOrCreateStrusContext( name );
	ctx->config = config;
	context->release( name, ctx );
}

strus::DatabaseInterface *strusWebService::getDataBaseInterface( const std::string &name )
{
	StrusConnectionContext *ctx = getOrCreateStrusContext( name );
	if( ctx->dbi == 0 ) {
		strus::DatabaseInterface *dbi = strus::createDatabase_leveldb( g_errorhnd );
		if( dbi == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->dbi = dbi;
	}
	context->release( name, ctx );
	return ctx->dbi;	
}

strus::StorageInterface *strusWebService::getStorageInterface( const std::string &name )
{
	StrusConnectionContext *ctx = getOrCreateStrusContext( name );
	if( ctx->sti == 0 ) {
		strus::StorageInterface *sti = strus::createStorage( g_errorhnd );
		if( sti == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->sti = sti;
	}
	context->release( name, ctx );
	return ctx->sti;
}

strus::DatabaseClientInterface *strusWebService::getDatabaseClientInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->dbci == 0 ) {
		strus::DatabaseInterface *dbi = ctx->dbi;		
		strus::DatabaseClientInterface *dbci = dbi->createClient( ctx->config );
		if( dbci == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->dbci = dbci;
	}
	context->release( name, ctx );
	return ctx->dbci;
}

strus::StorageClientInterface *strusWebService::getStorageClientInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->stci == 0 ) {
		strus::StorageClientInterface *stci = ctx->sti->createClient( ctx->config, ctx->dbci, 0 );
		if( stci == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->stci = stci;
	}
	context->release( name, ctx );
	return ctx->stci;
}

strus::MetaDataReaderInterface *strusWebService::getMetaDataReaderInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->mdri == 0 ) {
		strus::MetaDataReaderInterface *mdri = ctx->stci->createMetaDataReader( );
		if( mdri == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->mdri = mdri;
	}
	context->release( name, ctx );
	return ctx->mdri;
}

strus::AttributeReaderInterface *strusWebService::getAttributeReaderInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->atri == 0 ) {
		strus::AttributeReaderInterface *atri = ctx->stci->createAttributeReader( );
		if( atri == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->atri = atri;
	}
	context->release( name, ctx );
	return ctx->atri;
}

strus::StorageTransactionInterface *strusWebService::getStorageTransactionInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->stti == 0 ) {
		strus::StorageTransactionInterface *stti = ctx->stci->createTransaction( );
		if( stti == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->stti = stti;
	}
	context->release( name, ctx );
	return ctx->stti;
}

strus::QueryEvalInterface *strusWebService::getQueryEvalInterface( )
{
	if( qei == 0 ) {
		qei = strus::createQueryEval( g_errorhnd );
	}
	return qei;
}

strus::QueryProcessorInterface *strusWebService::getQueryProcessorInterface( )
{
	if( qpi == 0 ) {
		qpi = strus::createQueryProcessor( g_errorhnd );
	}
	return qpi;
}

std::string strusWebService::getConfigString( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	return ctx->config;
}

void strusWebService::deleteDataBaseInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->dbi != 0 ) {
		delete ctx->dbi;
		ctx->dbi = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->sti != 0 ) {
		delete ctx->sti;
		ctx->sti = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteDatabaseClientInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->dbci != 0 ) {
		delete ctx->dbci;
		ctx->dbci = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageClientInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->stci != 0 ) {
		delete ctx->stci;
		ctx->stci = 0;
		// deletes implicitly also dbci?!
		// TODO: I thought I pass a database reference to the storage object?
		// For now we just set dbci to 0 too without deleting it
		ctx->dbci = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteMetaDataReaderInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->mdri != 0 ) {
		delete ctx->mdri;
		ctx->mdri = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteAttributeReaderInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->atri != 0 ) {
		delete ctx->atri;
		ctx->atri = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageTransactionInterface( const std::string &name )
{
	StrusConnectionContext *ctx = context->acquire( name );
	if( ctx->stti != 0 ) {
		delete ctx->stti;
		ctx->stti = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteQueryEvalInterface(  )
{
	if( qei != 0 ) {
		delete qei;
		qei = 0;
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

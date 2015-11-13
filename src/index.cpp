#include "strusWebService.hpp"
#include "index.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

// TODO: sort out, copied from testRandomCollection.cpp, actually there
// I would prefer 4 to 5 files at most!
#include "strus/reference.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"

#include "error_codes.hpp"

// TODO: read from config
#define NOF_THREADS 128

namespace apps {

index::index( strusWebService &service )
	: master( service )
{
	prepare_strus_environment( );
	
	service.dispatcher( ).assign( "/index/create/(\\w+)", &index::create_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/delete/(\\w+)", &index::delete_cmd, this, 1 );
}

index::~index( )
{
}

void index::prepare_strus_environment( )
{	
	// TODO: number of threads, depends on CppCMS running model, must pass
	// parts of this config here
	// TODO: stderr, really? how can we redirect to the booster log?
	g_errorhnd = strus::createErrorBuffer_standard( stderr, NOF_THREADS );

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

void index::create_cmd( const std::string name )
{
	// TODO: storage configuration must be read from json request and also
	// read the basedir from the config
	std::string config = "path=./storage";

	prepare_strus_environment( );

	if( dbi->exists( config ) ) {
		report_error( ERROR_INDEX_CREATE_DATABASE_EXISTS, "An index with that name already exists" );
		return;
	}
	
	if( !dbi->createDatabase( config ) ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_DATABASE, g_errorhnd->fetchError( ) );
		return;
	}
	
	strus::DatabaseClientInterface *database = dbi->createClient( config );
	if( !database ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_CLIENT, g_errorhnd->fetchError( ) );
		return;
	}

	sti->createStorage( config, database );
	
	delete database;
	delete dbi;
	delete sti;
			
	report_ok( );
}

void index::delete_cmd( const std::string name )
{
	// TODO: storage configuration must be read from json request and also
	// read the basedir from the config
	std::string config = "path=./storage";
	
	prepare_strus_environment( );
	
	if( !dbi->exists( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}
	
	if( !dbi->destroyDatabase( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE, g_errorhnd->fetchError( ) );
		return;
	}

	delete dbi;
	delete sti;
	
	report_ok( );
}

} // namespace apps

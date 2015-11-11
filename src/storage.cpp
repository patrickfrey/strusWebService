#include "strusWebService.hpp"
#include "storage.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

// TODO: sort out, copied from testRandomCollection.cpp, actually there
// I would prefer 4 to 5 files at most!
#include "strus/reference.hpp"
#include "strus/databaseInterface.hpp"
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
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"

#include "error_codes.hpp"

namespace apps {

storage::storage( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/storage/create/(\\w+)", &storage::create_cmd, this, 1 );
}

void storage::create_cmd( const std::string name )
{
	strus::ErrorBufferInterface *g_errorhnd;
	// TODO: number of threads, depends on CppCMS running model, must pass
	// parts of this config here
	// TODO: stderr, really? how can we redirect to the booster log?
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1 );
	std::auto_ptr<strus::DatabaseInterface> dbi( strus::createDatabase_leveldb( g_errorhnd ) );
	if( !dbi.get( ) ) {
		report_error( ERROR_STORAGE_CREATE_CMD_CREATE_DATABASE_INTERFACE, g_errorhnd->fetchError( ) );
		return;
	}

	// TODO: storage configuration must be read from json request and also
	// read the basedir from the config
	std::string config = "path=./storage";
	
	std::auto_ptr<strus::StorageInterface> sti( strus::createStorage( g_errorhnd ) );
	if( !sti.get( ) ) {
		report_error( ERROR_STORAGE_CREATE_CMD_CREATE_STORAGE_INTERFACE, g_errorhnd->fetchError( ) );
		return;
	}

	if( !dbi->createDatabase( config ) ) {
		report_error( ERROR_STORAGE_CREATE_CMD_CREATE_DATABASE, g_errorhnd->fetchError( ) );
		return;
	}
		
	std::auto_ptr<strus::DatabaseClientInterface> database( dbi->createClient( config ) );

	sti->createStorage( config, database.get( ) );
			
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	response( ).out( ) << j << std::endl;
}

} // namespace apps

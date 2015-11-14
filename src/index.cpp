#include "strusWebService.hpp"
#include "index.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

#include <booster/locale/format.h>
#include <sstream>

// TODO: sort out, copied from testRandomCollection.cpp, actually there
// I would prefer 4 to 5 files at most!
#include "strus/reference.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"

#include "error_codes.hpp"

#include <sys/stat.h>
#include <cstring>
#include <cerrno>

// TODO: read from config
#define NOF_THREADS 128

namespace apps {

index::index( strusWebService &service, std::string storage_base_directory )
	: master( service ),
	storage_base_directory( storage_base_directory )
{
	// TODO: number of threads, depends on CppCMS running model, must pass
	// parts of this config here
	// TODO: stderr, really? how can we redirect to the booster log?
	g_errorhnd = strus::createErrorBuffer_standard( stderr, NOF_THREADS );
	
	service.dispatcher( ).assign( "/index/create/(\\w+)", &index::create_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/delete/(\\w+)", &index::delete_cmd, this, 1 );
}

index::~index( )
{
}

void index::prepare_strus_environment( )
{	
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

std::string index::get_storage_directory( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "{1}/{2}" ) % base_storage_dir % name;
	std::string directory = ss.str( );

	return directory;	
}

std::string index::get_storage_config( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "path={1}" ) % get_storage_directory( base_storage_dir, name );
	std::string config = ss.str( );

	return config;
}

void index::create_cmd( const std::string name )
{
	prepare_strus_environment( );

	std::string config = get_storage_config( storage_base_directory, name );

	if( dbi->exists( config ) ) {
		report_error( ERROR_INDEX_CREATE_DATABASE_EXISTS, "An index with that name already exists" );
		return;
	}
	
	int res = mkdir( storage_base_directory.c_str( ), S_IRUSR | S_IWUSR | S_IXUSR );
	if( res < 0 && errno != EEXIST ) {
		report_error( ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR, strerror( errno ) );
		return;
	}
	
	res = mkdir( get_storage_directory( storage_base_directory, name ).c_str( ), S_IRUSR | S_IWUSR | S_IXUSR );
	if( res < 0 ) {
		report_error( ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR, strerror( errno ) );
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
	prepare_strus_environment( );

	std::string config = get_storage_config( storage_base_directory, name );
	
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

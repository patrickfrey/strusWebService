#include "strusWebService.hpp"
#include "index.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

#include <booster/locale/format.h>
#include <sstream>

#include <iterator>
#include <algorithm>
#include <vector>
#include <boost/filesystem.hpp>

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
	// TODO: do this once and not here (master?)
	g_errorhnd = strus::createErrorBuffer_standard( stderr, NOF_THREADS );
	
	initialize_default_create_parameters( );
	
	service.dispatcher( ).assign( "/index/create/(\\w+)", &index::create_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/delete/(\\w+)", &index::delete_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/stats/(\\w+)", &index::stats_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/list", &index::list_cmd, this );
}

index::~index( )
{
}

void index::initialize_default_create_parameters( )
{
	default_create_parameters = service.settings( ).get<struct StorageCreateParameters>( "storage.default_create_parameters" );
	cppcms::json::value j;
	j["config"] = default_create_parameters;
	std::cout << j << std::endl;
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

void index::close_strus_environment( )
{
	delete dbi;
	delete sti;
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
	
	boost::system::error_code err;
	if( !boost::filesystem::create_directories(
		get_storage_directory( storage_base_directory, name ), err ) ) {
		report_error( ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR, err.message( ) );
		close_strus_environment( );
		return;
	}
		
	if( !dbi->createDatabase( config ) ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_DATABASE, g_errorhnd->fetchError( ) );
		close_strus_environment( );
		return;
	}
	
	strus::DatabaseClientInterface *database = dbi->createClient( config );
	if( !database ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_CLIENT, g_errorhnd->fetchError( ) );
		close_strus_environment( );
		return;
	}

	sti->createStorage( config, database );
	
	delete database;

	close_strus_environment( );
			
	report_ok( );
}

void index::delete_cmd( const std::string name )
{
	prepare_strus_environment( );

	std::string config = get_storage_config( storage_base_directory, name );
	
	if( !dbi->exists( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		close_strus_environment( );
		return;
	}
	
	if( !dbi->destroyDatabase( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE, g_errorhnd->fetchError( ) );
		close_strus_environment( );
		return;
	}
	
	close_strus_environment( );
	
	report_ok( );
}

void index::stats_cmd( const std::string name )
{
	prepare_strus_environment( );

	std::string config = get_storage_config( storage_base_directory, name );

	if( !dbi->exists( config ) ) {
		report_error( ERROR_INDEX_STATS_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		close_strus_environment( );
		return;
	}

	strus::DatabaseClientInterface *database = dbi->createClient( config );
	if( !database ) {
		report_error( ERROR_INDEX_STATS_CMD_CREATE_DATABASE_CLIENT, g_errorhnd->fetchError( ) );
		return;
	}

	strus::StorageClientInterface *storage = sti->createClient( config, database );
	if( !storage ) {
		delete database;
		close_strus_environment( );
		report_error( ERROR_INDEX_STATS_CMD_CREATE_STORAGE_CLIENT, g_errorhnd->fetchError( ) );
		return;
	}
	
	strus::GlobalCounter nof_docs = storage->globalNofDocumentsInserted( );
	
	response( ).content_type( "application/json" );
	cppcms::json::value j;
	j["result"] = "ok";
	j["stats"]["nofDocs"] = nof_docs;

	// database is deleted implicitely!
	delete storage;
	
	close_strus_environment( );

	response( ).out( ) << j << std::endl;	
}

void index::list_cmd( )
{
	typedef std::vector<boost::filesystem::directory_entry> dirlist;
	dirlist dirs;
	
	boost::filesystem::path dir( storage_base_directory );
	if( exists( dir ) ) {
		if( !is_directory( dir ) ) {
			report_error( ERROR_INDEX_STATS_CMD_ILLEGAL_STORAGE_DIR, "Base storage directory is not a directory" );
			return;
		}
	} else {
		report_error( ERROR_INDEX_STATS_CMD_ILLEGAL_STORAGE_DIR, "Base storage directory does not exist" );
		return;
	}
		  
	std::copy( boost::filesystem::directory_iterator( storage_base_directory ),
		boost::filesystem::directory_iterator( ), std::back_inserter( dirs ) );

	response( ).content_type( "application/json" );
	cppcms::json::value j;
	j["result"] = "ok";
	std::vector<std::string> v;
	for( dirlist::const_iterator it = dirs.begin( ); it != dirs.end( ); it++ ) {
		std::string last;
		for( boost::filesystem::path::iterator pit = it->path( ).begin( ); pit != it->path( ).end( ); pit++ ) {
			last = pit->native( );
		}
		v.push_back( last );
	}
	j["indexes"] = v;

	response( ).out( ) << j << std::endl;
}

} // namespace apps

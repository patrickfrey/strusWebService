#include "index.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <booster/log.h>
#include <sstream>

#include <iterator>
#include <algorithm>
#include <vector>
#include <utility>
#include <istream>

#include <boost/filesystem.hpp>

namespace apps {

index::index( strusWebService &service, std::string storage_base_directory )
	: master( service ),
	storage_base_directory( storage_base_directory )
{	
	initialize_default_create_parameters( );
	
	service.dispatcher( ).assign( "/index/create/(\\w+)", &index::create_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/delete/(\\w+)", &index::delete_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/config/(\\w+)", &index::config_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/stats/(\\w+)", &index::stats_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/list", &index::list_cmd, this );
	service.dispatcher( ).assign( "/index/exists/(\\w+)", &index::exists_cmd, this, 1 );
}

index::~index( )
{
}
		
void index::initialize_default_create_parameters( )
{
	default_create_parameters = service.settings( ).get<struct StorageCreateParameters>( "storage.default_create_parameters" );

	cppcms::json::value j;
	j["config"] = default_create_parameters;

	BOOSTER_DEBUG( PACKAGE ) << "Default storage configuration parameters:" << j;
}

void index::create_cmd( const std::string name )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	struct StorageCreateParameters params;
	
	std::pair<void *, size_t> data = request( ).raw_post_data( );
	std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
	cppcms::json::value p;
	if( !p.load( is, true) ) {
		report_error( ERROR_INDEX_ILLEGAL_JSON, "Illegal JSON received" );
		return;
	}
	
	if( p.type( "params" ) == cppcms::json::is_object ) {
		try {
			params = p.get<struct StorageCreateParameters>( "params" );
		} catch( cppcms::json::bad_value_cast &e ) {
			report_error( ERROR_INDEX_ILLEGAL_JSON, "Illegal storage creation parameter received" );
			return;
		}
	} else if( p.type( "params" ) == cppcms::json::is_null ) {
		params = default_create_parameters;
	} else {
		report_error( ERROR_INDEX_ILLEGAL_JSON, "Expecting a JSON object as storage creation parameter" );
		return;
	}
		
	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;
	// TODO: merge configurations

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( dbi->exists( config ) ) {
		report_error( ERROR_INDEX_CREATE_DATABASE_EXISTS, "An index with that name already exists" );
		return;
	}
	
	boost::system::error_code err;
	if( !boost::filesystem::create_directories(
		service.getStorageDirectory( storage_base_directory, name ), err ) ) {
		report_error( ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR, err.message( ) );
		return;
	}
		
	if( !dbi->createDatabase( config ) ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_DATABASE, service.getLastStrusError( ) );
		return;
	}
	
	strus::DatabaseClientInterface *database = dbi->createClient( config );
	if( !database ) {
		report_error( ERROR_INDEX_CREATE_CMD_CREATE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	sti->createStorage( config, database );
	
	service.registerStorageConfig( name, config );
	
	delete database;
			
	report_ok( );
}

void index::delete_cmd( const std::string name )
{
	if( !ensure_post( ) ) return;

	// close all handles, we are going to delete the index now
	// TODO: what if in parallel other clients access it? locking with timeout or
	// error to this worker's client?
	close_strus_environment( name );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		return;
	}
	
	if( !dbi->exists( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}
	
	if( !dbi->destroyDatabase( config ) ) {
		report_error( ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE, service.getLastStrusError( ) );
		return;
	}
			
	report_ok( );
}

void index::config_cmd( const std::string name )
{
	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string configStr = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( configStr ) ) {
		report_error( ERROR_INDEX_CONFIG_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::MetaDataReaderInterface *metadata = service.getMetaDataReaderInterface( name );
	if( !metadata ) {
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_METADATA_READER, service.getLastStrusError( ) );
		return;
	}
	
	struct StorageConfiguration config;
	
	for( strus::Index it = 0; it < metadata->nofElements( ); it++ ) {
		struct MetadataDefiniton meta;
		meta.name = metadata->getName( it );
		meta.type = metadata->getType( it );		
		config.metadata.push_back( meta );
	}

	cppcms::json::value j;
	j["config"] = config;
	
	report_ok( j );	
}

void index::stats_cmd( const std::string name )
{
	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string configStr = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( configStr ) ) {
		report_error( ERROR_INDEX_STATS_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_INDEX_STATS_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		delete database;
		report_error( ERROR_INDEX_STATS_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}
	
	struct StorageStatistics stats;
	stats.nof_docs = storage->nofDocumentsInserted( );
		
	cppcms::json::value j;
	j["stats"] = stats;
	
	report_ok( j );
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
	std::sort( v.begin( ), v.end( ) );
	j["indexes"] = v;

	report_ok( j );
}

void index::exists_cmd( const std::string name )
{
	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		return;
	}

	cppcms::json::value j;
	j["exists"] = dbi->exists( config );
	
	report_ok( j );
}

} // namespace apps

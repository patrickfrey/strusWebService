#include "strusWebService.hpp"
#include "index.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/json.h>

#include <booster/locale/format.h>
#include <booster/log.h>
#include <sstream>

#include <iterator>
#include <algorithm>
#include <vector>
#include <utility>
#include <istream>

#include <boost/filesystem.hpp>

#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"

namespace apps {

index::index( strusWebService &service, std::string storage_base_directory )
	: master( service ),
	storage_base_directory( storage_base_directory )
{	
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

	BOOSTER_DEBUG( PACKAGE ) << "Default storage configuration parameters:" << j;
}

std::string index::get_storage_directory( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "{1}/{2}" ) % base_storage_dir % name;
	std::string directory = ss.str( );

	return directory;	
}

std::string index::get_storage_config( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name )
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
		% get_storage_directory( base_storage_dir, name )
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
	} else {
		report_error( ERROR_INDEX_ILLEGAL_JSON, "Expecting a JSON object as storage creation parameter" );
		return;
	}
		
	prepare_strus_environment( );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = get_storage_config( storage_base_directory, combined_params, name );

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
	if( !ensure_post( ) ) return;

	prepare_strus_environment( );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = get_storage_config( storage_base_directory, combined_params, name );
	
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

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = get_storage_config( storage_base_directory, combined_params, name );

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
	
	struct StorageStatistics stats;
	stats.nof_docs = storage->globalNofDocumentsInserted( );
	
	// database is deleted implicitely!
	delete storage;
	
	close_strus_environment( );

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
	j["indexes"] = v;

	report_ok( j );
}

} // namespace apps

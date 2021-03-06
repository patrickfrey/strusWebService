/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "index.hpp"
#include "strusWebService.hpp"

#include "strus/valueIteratorInterface.hpp"

#include <cppcms/url_dispatcher.h>
#include <cppcms/http_request.h>

#include <booster/log.h>

#include <sstream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <utility>
#include <istream>
#include <iomanip>

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
	service.dispatcher( ).assign( "/index/open/(\\w+)", &index::open_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/close/(\\w+)", &index::close_cmd, this, 1 );
	service.dispatcher( ).assign( "/index/rename/(\\w+)/(\\w+)", &index::rename_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/index/swap/(\\w+)/(\\w+)", &index::swap_cmd, this, 1, 2 );
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

struct StorageCreateParameters index::merge_create_parameters( const struct StorageCreateParameters &defaults, const struct StorageCreateParameters &explicit_params )
{
	// inherit the non-metadata part of the create parameters
	struct StorageCreateParameters params = default_create_parameters;
	params.metadata.clear( );

	// KLUDGE: for now we do not want the user of the service to be
	// able to change low-level configuration parameters (or potentially
	// dangerous parameters like cache sizes, we leave this to the administrator).
	// metadata definitions on the other hand should be configurable via API
	// TODO: add roles for the webservice like reader (query), writer, DDL admin, super admin

	// iterate explicit parameters, issue warnings if default metadata is
	// being redefined
	for( std::vector<struct MetadataDefiniton>::const_iterator it = explicit_params.metadata.begin( ); it != explicit_params.metadata.end( ); it++ ) {
		bool seen_default = false;
		for( std::vector<struct MetadataDefiniton>::const_iterator it2 = default_create_parameters.metadata.begin( ); it2 != default_create_parameters.metadata.end( ); it2++ ) {
			if( it->name == it2->name ) {
				if( it->type == it2->type ) {
					// explicit override with same type, no problem
					params.metadata.push_back( *it );
					seen_default = true;
					continue;
				} else {
					BOOSTER_WARNING( PACKAGE ) << "overwritting an already existing metadata definition for '" << it->name << "'";
					params.metadata.push_back( *it );
					seen_default = true;
					continue;
				}
			}
		}
		if( !seen_default ) {
			// new defintion
			params.metadata.push_back( *it );
		}
	}

	// check for default parameters and append them if needed
	for( std::vector<struct MetadataDefiniton>::const_iterator it = default_create_parameters.metadata.begin( ); it != default_create_parameters.metadata.end( ); it++ ) {
		bool contains_default = false;
		for( std::vector<struct MetadataDefiniton>::const_iterator it2 = explicit_params.metadata.begin( ); it2 != explicit_params.metadata.end( ); it2++ ) {
			if( it->name == it2->name ) {
				contains_default = true;
			}
		}
		if( !contains_default ) {
			params.metadata.push_back( *it );
		}
	}

	return params;
}

void index::create_cmd( const std::string name )
{
	Timer timer;

	if( !handle_preflight_cors( ) ) return;
	if( !ensure_post( ) ) return;
	if( !ensure_json_request( ) ) return;

	log_request( );

	std::pair<void *, size_t> data = request( ).raw_post_data( );
	std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
	cppcms::json::value p;
	if( !p.load( is, true) ) {
		report_error( ERROR_INDEX_ILLEGAL_JSON, "Illegal JSON received" );
		return;
	}
	
	struct StorageCreateParameters params;

	if( p.type( "params" ) == cppcms::json::is_object ) {
		try {
			struct StorageCreateParameters explicit_params;
			explicit_params = p.get<struct StorageCreateParameters>( "params" );
			params = merge_create_parameters( default_create_parameters, explicit_params );
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

	std::string config = service.getStorageConfig( storage_base_directory, params, name );

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

	sti->createStorage( config, dbi );

	service.registerStorageConfig( name, config );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );;
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "create_index(" << name << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "create_index(" << name << "): " << ss.str( );

	report_ok( j );
}

void index::delete_cmd( const std::string name )
{
	Timer timer;

	if( !handle_preflight_cors( ) ) return;
	if( !ensure_post( ) ) return;

	log_request( );

	service.lockIndex( name, true );
	
	// close all handles, we are going to delete the index now
	// TODO: what if in parallel other clients access it? locking with timeout or
	// error to this worker's client?
	close_strus_environment( name );
	
	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( config ) ) {
	service.unlockIndex( name );
		report_error( ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	if( !dbi->destroyDatabase( config ) ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE, service.getLastStrusError( ) );
		return;
	}

	service.unlockIndex( name );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "delete_index(" << name << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "delete_index(" << name << "): " << ss.str( );

	report_ok( j );
}

static bool metadata_definition_sorter( struct MetadataDefiniton const &meta1, struct MetadataDefiniton const &meta2 )
{
	if( meta1.name != meta2.name ) {
		return meta1.name < meta2.name;
	}
	return meta1.type < meta2.type;
}

void index::config_cmd( const std::string name )
{
	Timer timer;

	log_request( );

	service.lockIndex( name, false );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string configStr = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( configStr ) ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_CONFIG_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::MetaDataReaderInterface *metadata = service.getMetaDataReaderInterface( name );
	if( !metadata ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_METADATA_READER, service.getLastStrusError( ) );
		return;
	}

	strus::AttributeReaderInterface *attributeReader = service.getAttributeReaderInterface( name );
	if( !attributeReader ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_ATTRIBUTE_READER, service.getLastStrusError( ) );
		return;
	}

	struct StorageConfiguration config;

	strus::ValueIteratorInterface *valItr = storage->createTermTypeIterator( );
	std::vector<std::string> termTypes = valItr->fetchValues( FEATURE_ITERATOR_BATCH_SIZE );
	while( termTypes.size( ) > 0 ) {
		config.types.insert( config.types.end( ), termTypes.begin( ), termTypes.end( ) );
		termTypes = valItr->fetchValues( FEATURE_ITERATOR_BATCH_SIZE );
	}
	delete valItr;

	for( strus::Index it = 0; it < metadata->nofElements( ); it++ ) {
		struct MetadataDefiniton meta;
		meta.name = metadata->getName( it );
		meta.type = metadata->getType( it );
		config.metadata.push_back( meta );
	}
	std::sort( config.metadata.begin( ), config.metadata.end( ), &metadata_definition_sorter );

	std::vector<std::string> attrNames = attributeReader->getNames( );
	std::sort( attrNames.begin( ), attrNames.end( ) );
	config.attributes = attrNames;

	service.unlockIndex( name );

	cppcms::json::value j;
	j["config"] = config;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "config_index(" << name << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "config_index(" << name << "): " << ss.str( );

	report_ok( j );
}

void index::stats_cmd( const std::string name )
{
	Timer timer;

	log_request( );

	service.lockIndex( name, false );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string configStr = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( configStr ) ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_STATS_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_STATS_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	struct StorageStatistics stats;
	stats.nof_docs = storage->nofDocumentsInserted( );

	service.unlockIndex( name );

	cppcms::json::value j;
	j["stats"] = stats;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "stats_index(" << name << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "stats_index(" << name << "): " << ss.str( );

	report_ok( j );
}

void index::list_cmd( )
{
	Timer timer;

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

	log_request( );

	std::vector<std::string> v = service.getAllIndexNames( );
	std::sort( v.begin( ), v.end( ) );

	cppcms::json::value j;
	j["result"] = "ok";
	j["indexes"] = v;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "list_indexes(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "list_indexes: " << ss.str( );

	report_ok( j );
}

void index::exists_cmd( const std::string name )
{
	Timer timer;

	log_request( );

	service.lockIndex( name, false );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	cppcms::json::value j;
	j["exists"] = dbi->exists( config );
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	service.unlockIndex( name );

	BOOSTER_INFO( PACKAGE ) << "exists_index(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "exists_index: " << ss.str( );

	report_ok( j );
}

void index::open_cmd( const std::string name )
{
	Timer timer;

	log_request( );

	service.lockIndex( name, true );

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !dbi->exists( config ) ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_OPEN_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_OPEN_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	service.unlockIndex( name );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "open_index(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "open_index: " << ss.str( );

	report_ok( j );
}

void index::close_cmd( const std::string name )
{
	Timer timer;

	log_request( );

	service.lockIndex( name, true );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	std::string config = service.getStorageConfig( storage_base_directory, combined_params, name );

	if( !dbi->exists( config ) ) {
		service.unlockIndex( name );
		report_error( ERROR_INDEX_CLOSE_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	close_strus_environment( name );

	service.unlockIndex( name );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "close_index(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "close_index: " << ss.str( );

	report_ok( j );
}

void index::rename_cmd( const std::string name1, const std::string name2 )
{
	Timer timer;

	log_request( );

	if( !get_strus_environment( name1 ) ) {
		return;
	}

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config1 = service.getStorageConfig( storage_base_directory, combined_params, name1 );

	if( !dbi->exists( config1 ) ) {
		report_error( ERROR_INDEX_RENAME_CMD_NO_SUCH_DATABASE, "Search index does not exist" );
		return;
	}

	service.lockIndex( name1, true );

	close_strus_environment( name1 );

	if( !get_strus_environment( name1 ) ) {
		return;
	}

	std::string config2 = service.getStorageConfig( storage_base_directory, combined_params, name2 );

	if( dbi->exists( config2 ) ) {
		report_error( ERROR_INDEX_RENAME_CMD_ALREADY_EXISTS, "A search index with that name already exists" );
		return;
	}

	boost::filesystem::path path1( storage_base_directory ); path1 /= name1;
	boost::filesystem::path path2( storage_base_directory ); path2 /= name2;
	boost::system::error_code err;
	boost::filesystem::rename( path1, path2, err );
	if( err.value( ) != boost::system::errc::success ) {
		report_error( ERROR_INDEX_RENAME_CMD_RENAME_ERROR, err.message( ) );
	}

	service.unlockIndex( name1 );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "rename_index(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "rename_index: " << ss.str( );

	report_ok( j );
}

void index::swap_cmd( const std::string name1, const std::string name2 )
{
	Timer timer;

	log_request( );

	if( !get_strus_environment( name1 ) ) {
		return;
	}

	struct StorageCreateParameters combined_params;
	combined_params = default_create_parameters;

	std::string config1 = service.getStorageConfig( storage_base_directory, combined_params, name1 );

	if( !dbi->exists( config1 ) ) {
		report_error( ERROR_INDEX_SWAP_CMD_NO_SUCH_DATABASE1, "First search index does not exist" );
		return;
	}

	service.lockIndex( name1, true );
	service.lockIndex( name2, true );

	close_strus_environment( name1 );

	if( !get_strus_environment( name1 ) ) {
		return;
	}

	std::string config2 = service.getStorageConfig( storage_base_directory, combined_params, name2 );

	if( !dbi->exists( config2 ) ) {
		report_error( ERROR_INDEX_SWAP_CMD_NO_SUCH_DATABASE2, "Second search index does not exist" );
		return;
	}

	close_strus_environment( name2 );

	boost::filesystem::path path1( storage_base_directory ); path1 /= name1;
	boost::filesystem::path path2( storage_base_directory ); path2 /= name2;
	boost::filesystem::path pathtmp( storage_base_directory ); pathtmp /= "_tmp";
	boost::system::error_code err;
	boost::filesystem::rename( path2, pathtmp, err );
	if( err.value( ) != boost::system::errc::success ) {
		report_error( ERROR_INDEX_SWAP_CMD_RENAME_ERROR, err.message( ) );
	}
	boost::filesystem::rename( path1, path2, err );
	if( err.value( ) != boost::system::errc::success ) {
		report_error( ERROR_INDEX_SWAP_CMD_RENAME_ERROR, err.message( ) );
	}
	boost::filesystem::rename( pathtmp, path1, err );
	if( err.value( ) != boost::system::errc::success ) {
		report_error( ERROR_INDEX_SWAP_CMD_RENAME_ERROR, err.message( ) );
	}

	service.unlockIndex( name2 );
	service.unlockIndex( name1 );

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "swap_index(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "swap_index: " << ss.str( );

	report_ok( j );
}

} // namespace apps

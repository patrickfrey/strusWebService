/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INDEX_HPP
#define INDEX_HPP

#include "master.hpp"
#include "constants.hpp"

#include <cppcms/json.h>

#include <vector>
#include <string>

struct MetadataDefiniton {
	std::string name;
	std::string type;
	
	bool operator < ( const MetadataDefiniton &m ) const {
		return name.compare( m.name );
	}
};

struct StorageCreateParameters {
	std::vector<struct MetadataDefiniton> metadata;
	// currently only one value "leveldb"
	std::string database;
	// TODO: this depends on the database implementation
	bool compression;
	size_t cache_size;
	size_t max_open_files;
	size_t write_buffer_size;
	size_t block_size;
};

struct StorageConfiguration {
	std::vector<struct MetadataDefiniton> metadata;
	std::vector<std::string> attributes;
	std::vector<std::string> types;
	// TODO: more, most likely subset of 'StorageCreateParameters'
};

struct StorageStatistics {
	size_t nof_docs;
};

namespace apps {

class index : public master {

	public:
		index( strusWebService &service, std::string storage_base_directory );
		virtual ~index( );
		
	protected:
		std::string storage_base_directory;
		struct StorageCreateParameters default_create_parameters;
		
		void initialize_default_create_parameters( );
		struct StorageCreateParameters merge_create_parameters( const struct StorageCreateParameters &defaults, const struct StorageCreateParameters &explicit_params );
						
	private:
		void create_cmd( const std::string name );
		void delete_cmd( const std::string name );
		void config_cmd( const std::string name );
		void stats_cmd( const std::string name );
		void list_cmd( );
		void exists_cmd( const std::string name );
		void open_cmd( const std::string name );
		void close_cmd( const std::string name );
		void rename_cmd( const std::string name1, const std::string name2 );
		void swap_cmd( const std::string name1, const std::string name2 );
};

} // namespace apps

namespace cppcms {
	namespace json {
		
template<>
struct traits<MetadataDefiniton> {

	static MetadataDefiniton get( value const &v )
	{
		MetadataDefiniton m;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		m.name = v.get<std::string>( "name" );
		m.type = v.get<std::string>( "type" );
		return m;
	}
	
	static void set( value &v, MetadataDefiniton const &m )
	{
		v.set( "name", m.name );
		v.set( "type", m.type );
	}		
};

template<>
struct traits<StorageCreateParameters> {
	
	static StorageCreateParameters get( value const &v )
	{
		StorageCreateParameters p;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		p.database = v.get<std::string>( "database", DEFAULT_DATABASE );
		p.compression = v.get<bool>( "compression", LEVELDB_DATABASE_DEFAULT_COMPRESSION );
		// TODO: we should introspect them via the database configuration for
		// the specific implementation
		p.cache_size = v.get<size_t>( "cache_size", LEVELDB_DATABASE_DEFAULT_LRU_CACHESIZE );
		p.max_open_files = v.get<size_t>( "max_open_files", LEVELDB_DATABASE_DEFAULT_MAX_OPEN_FILES );
		p.write_buffer_size = v.get<size_t>( "write_buffer_size", LEVELDB_DATABASE_DEFAULT_WRITE_BUFFER_SIZE );
		p.block_size = v.get<size_t>( "block_size", LEVELDB_DATABASE_DEFAULT_BLOCK_SIZE );		
		p.metadata = v.get<std::vector<struct MetadataDefiniton> >( "metadata", std::vector<struct MetadataDefiniton>( ) );
		return p;
	}
	
	static void set( value &v, StorageCreateParameters const &p )
	{
		v.set( "database", p.database );
		v.set( "compression", p.compression );
		v.set( "cache_size", p.cache_size );
		v.set( "max_open_files", p.max_open_files );
		v.set( "write_buffer_size", p.write_buffer_size );
		v.set( "block_size", p.block_size );
		v.set( "metadata", p.metadata );
	}
};

template<>
struct traits<StorageConfiguration> {
	
	static StorageConfiguration get( value const &v )
	{
		StorageConfiguration c;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}		
		c.metadata = v.get<std::vector<struct MetadataDefiniton> >( "metadata", std::vector<struct MetadataDefiniton>( ) );
		c.attributes = v.get<std::vector<std::string> >( "attributes", std::vector<std::string>( ) );
		c.types = v.get<std::vector<std::string> >( "types", std::vector<std::string>( ) );
		return c;
	}
	
	static void set( value &v, StorageConfiguration const &c )
	{
		v.set( "metadata", c.metadata );
		v.set( "attributes", c.attributes );
		v.set( "types", c.types );
	}
};

template<>
struct traits<StorageStatistics> {
	
	static void set( value &v, StorageStatistics const &s )
	{
		v.set( "nof_docs", s.nof_docs );
	}
};
	
} } // namespace cppcms::json

#endif

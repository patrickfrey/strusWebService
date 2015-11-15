#ifndef INDEX_HPP
#define INDEX_HPP

#include "master.hpp"
#include "constants.hpp"

#include "strus/lib/error.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"

#include <cppcms/json.h>

#include <vector>

struct MetadataDefiniton {
	std::string name;
	std::string type;
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

namespace apps {

class index : public master {

	public:
		index( strusWebService &service, std::string storage_base_directory );
		virtual ~index( );
		
	protected:
		std::string storage_base_directory;
		strus::ErrorBufferInterface *g_errorhnd;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		struct StorageCreateParameters default_create_parameters;
		
		void initialize_default_create_parameters( );
		// TODO: or can we pass to get with a different initialzer
		//~ struct StorageCreateParameters merge_create_parameters( const struct StorageCreateParameters &defaults, const struct StorageCreateParameters &params );
		void prepare_strus_environment( );
		void close_strus_environment( );
		
		std::string get_storage_directory( const std::string &base_storage_dir, const std::string &name );
		std::string get_storage_config( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name );
				
	private:
		void create_cmd( const std::string name );
		void delete_cmd( const std::string name );
		void stats_cmd( const std::string name );
		void list_cmd( );
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

} } // namespace cppcms::json

#endif

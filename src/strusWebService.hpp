#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "other.hpp"
#include "index.hpp"
#include "document.hpp"
#include "query.hpp"

#include <cppcms/application.h>  

#include <map>

#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageTransactionInterface.hpp"

struct strusContext {
	std::string config;
	strus::DatabaseInterface *dbi;
	strus::StorageInterface *sti;
	strus::DatabaseClientInterface *dbci;
	strus::StorageClientInterface *stci;
	strus::MetaDataReaderInterface *mdri;
	strus::StorageTransactionInterface *stti;
	
	public:
		strusContext( ) : config( "" ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), stti( 0 ) { }

		strusContext( const std::string &_config )
			: config( _config ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), stti( 0 ) { }			
};

namespace apps {

class strusWebService : public cppcms::application {

	private:
		FILE *logfile;
		strus::ErrorBufferInterface *g_errorhnd;
		std::map<std::string, struct strusContext> context_map;
		std::string storage_base_directory;

	public:
		apps::master master;
		apps::other other;
		apps::index index;
		apps::document document;
		apps::query query;
		
	private:
		void persistStorageConfigurations( );
		void restoreStorageConfigurations( );

	public:
		strusWebService( cppcms::service &srv );
		std::string getLastStrusError( );
		std::vector<std::string> getStrusErrorDetails( );
		std::string getStorageDirectory( const std::string &base_storage_dir, const std::string &name );
		std::string getStorageConfig( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name );
		std::string getStorageConfig( const std::string &base_storage_dir, const std::string &name );
		void getOrCreateStrusContext( const std::string &name );
		void registerStorageConfig( const std::string &name, const std::string &config );
		strus::DatabaseInterface *getDataBaseInterface( const std::string &name );
		strus::StorageInterface *getStorageInterface( const std::string &name );
		strus::DatabaseClientInterface *getDatabaseClientInterface( const std::string &name );
		strus::StorageClientInterface *getStorageClientInterface( const std::string &name );
		strus::MetaDataReaderInterface *getMetaDataReaderInterface( const std::string &name );
		strus::StorageTransactionInterface *getStorageTransactionInterface( const std::string &name );
		std::string getConfigString( const std::string &name );
		void deleteDataBaseInterface( const std::string &name );
		void deleteStorageInterface( const std::string &name );
		void deleteDatabaseClientInterface( const std::string &name );
		void deleteStorageClientInterface( const std::string &name );
		void deleteMetaDataReaderInterface( const std::string &name );
		void deleteStorageTransactionInterface( const std::string &name );
};

} // namespace apps

#endif

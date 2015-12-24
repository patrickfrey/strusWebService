#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "other.hpp"
#include "index.hpp"
#include "document.hpp"
#include "query.hpp"

#include <cppcms/application.h>  

#include "strusContext.hpp"

#include "strus/queryEvalInterface.hpp"
#include "strus/queryProcessorInterface.hpp"

namespace apps {

class strusWebService : public cppcms::application {

	private:
		StrusContext *context;
		std::string storage_base_directory;
		strus::QueryProcessorInterface *qpi;
		strus::QueryEvalInterface *qei;

	public:
		apps::master master;
		apps::other other;
		apps::index index;
		apps::document document;
		apps::query query;
		
	public:
		strusWebService( cppcms::service &srv, StrusContext *context );
		bool hasError( ) const;
		std::string getLastStrusError( ) const;
		std::vector<std::string> getStrusErrorDetails( ) const;
		std::string getStorageDirectory( const std::string &base_storage_dir, const std::string &name );
		std::string getStorageConfig( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name );
		std::string getStorageConfig( const std::string &base_storage_dir, const std::string &name );
		StrusIndexContext *getOrCreateStrusContext( const std::string &name );
		void registerStorageConfig( const std::string &name, const std::string &config );
		strus::DatabaseInterface *getDataBaseInterface( const std::string &name );
		strus::StorageInterface *getStorageInterface( const std::string &name );
		strus::DatabaseClientInterface *getDatabaseClientInterface( const std::string &name );
		strus::StorageClientInterface *getStorageClientInterface( const std::string &name );
		strus::MetaDataReaderInterface *getMetaDataReaderInterface( const std::string &name );
		strus::AttributeReaderInterface *getAttributeReaderInterface( const std::string &name );
		strus::StorageTransactionInterface *createStorageTransactionInterface( const std::string &name );
		strus::QueryEvalInterface *getQueryEvalInterface( );
		strus::QueryProcessorInterface* getQueryProcessorInterface( );		
		std::string getConfigString( const std::string &name );
		void deleteDataBaseInterface( const std::string &name );
		void deleteStorageInterface( const std::string &name );
		void deleteDatabaseClientInterface( const std::string &name );
		void deleteStorageClientInterface( const std::string &name );
		void deleteMetaDataReaderInterface( const std::string &name );
		void deleteAttributeReaderInterface( const std::string &name );
		void deleteQueryEvalInterface( );
		void deleteQueryProcessorInterface( );
};

} // namespace apps

#endif

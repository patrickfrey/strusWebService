/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann <mail@andreasbaumann.cc>
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "other.hpp"
#include "index.hpp"
#include "document.hpp"
#include "query.hpp"
#include "transaction.hpp"

#include <cppcms/application.h>  

#include "strusContext.hpp"

#include "strus/queryEvalInterface.hpp"

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
		apps::transaction transaction;
		
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
		strus::StorageTransactionInterface *createStorageTransactionInterface( const std::string &name, const std::string &id );
		strus::StorageTransactionInterface *getStorageTransactionInterface( const std::string &name, const std::string &id );
		strus::QueryEvalInterface *getQueryEvalInterface( );
		strus::QueryProcessorInterface* getQueryProcessorInterface( );		
		std::string getConfigString( const std::string &name );
		void deleteStorageTransactionInterface( const std::string &name, const std::string &id );
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

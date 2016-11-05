/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

#ifndef _WIN32
#include <pthread.h>
#else
#error Not done for Windows currently
#endif

#include <fstream>

#include "strusContext.hpp"

#include "strus/queryEvalInterface.hpp"

namespace apps {

class strusWebService : public cppcms::application {

	private:
        cppcms::service &srv;
		StrusContext *context;
		std::string storage_base_directory;
		strus::QueryProcessorInterface *qpi;
		strus::QueryEvalInterface *qei;
        bool log_requests;
		std::string log_request_filename;
        std::map<pthread_t, std::ofstream *> log_request_streams;
        
	public:
		apps::master master;
		apps::other other;
		apps::index index;
		apps::document document;
		apps::query query;
		apps::transaction transaction;
		
	public:
		strusWebService( cppcms::service &srv, StrusContext *context, bool pretty_print );
        virtual ~strusWebService( );
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
		strus::StorageClientInterface *getStorageClientInterface( const std::string &name );
		strus::MetaDataReaderInterface *getMetaDataReaderInterface( const std::string &name );
		strus::AttributeReaderInterface *getAttributeReaderInterface( const std::string &name );
		strus::StorageTransactionInterface *createStorageTransactionInterface( const std::string &name );
		strus::StorageTransactionInterface *createStorageTransactionInterface( const std::string &name, const std::string &id );
		strus::StorageTransactionInterface *getStorageTransactionInterface( const std::string &name, const std::string &id );
		std::vector<std::string> getAllTransactionsIdsOfIndex( const std::string &name );
		strus::QueryEvalInterface *getQueryEvalInterface( );
		strus::QueryProcessorInterface* getQueryProcessorInterface( );		
		std::string getConfigString( const std::string &name );
		void deleteStorageTransactionInterface( const std::string &name, const std::string &id );
		void deleteDataBaseInterface( const std::string &name );
		void deleteStorageInterface( const std::string &name );
		void deleteStorageClientInterface( const std::string &name );
		void deleteMetaDataReaderInterface( const std::string &name );
		void deleteAttributeReaderInterface( const std::string &name );
		void deleteQueryEvalInterface( );
		void deleteQueryProcessorInterface( );
        void abortAllRunningTransactions( );
        void abortRunningTransactions( const std::string &name );
        std::vector<std::string> getAllIndexNames( );
        void raiseTerminationFlag( );
        std::ofstream *log_request_stream( );
};

} // namespace apps

#endif

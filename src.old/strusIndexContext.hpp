/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef STRUS_INDEX_CONTEXT_HPP
#define STRUS_INDEX_CONTEXT_HPP

#include <map>
#include <string>

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/storageTransactionInterface.hpp"

#include <booster/thread.h>

#include "version.hpp"

#include <ctime>

class StrusTransactionInfo {
	public:
		strus::StorageTransactionInterface *stti;
		time_t last_used;
};

class StrusIndexContext {
	public:
		std::string name;
		std::string config;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		strus::StorageClientInterface *stci;
		strus::MetaDataReaderInterface *mdri;
		strus::AttributeReaderInterface *atri;
		std::map<std::string, StrusTransactionInfo> trans_map;
		booster::mutex mutex;

	public:
		StrusIndexContext( );
		StrusIndexContext( const std::string &_name, const std::string &_config );
		virtual ~StrusIndexContext( );
				
		void read_lock( );
		void write_lock( );
		void unlock( );
		void terminateIdleTransactions( unsigned int max_livetime );
		
	private:
		void abortAllRunningTransactions( );
};

#endif

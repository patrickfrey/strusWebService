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

struct StrusIndexContext {
	std::string name;
	std::string config;
	strus::DatabaseInterface *dbi;
	strus::StorageInterface *sti;
	strus::StorageClientInterface *stci;
	strus::MetaDataReaderInterface *mdri;
	strus::AttributeReaderInterface *atri;
	std::map<std::string, strus::StorageTransactionInterface *> trans_map;
	booster::mutex mutex;

	public:
		StrusIndexContext( );
		StrusIndexContext( const std::string &_name, const std::string &_config );
		virtual ~StrusIndexContext( );
				
		void read_lock( );
		void write_lock( );
		void unlock( );
		
	private:
		void abortAllRunningTransactions( );
};

#endif

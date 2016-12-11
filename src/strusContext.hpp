/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef STRUS_CONTEXT_HPP
#define STRUS_CONTEXT_HPP

#include <map>
#include <string>
#include <vector>

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"

#include <booster/thread.h>
#include <booster/log.h>

#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/moduleEntryPoint.hpp"

#include "strusIndexContext.hpp"

#include <booster/aio/deadline_timer.h>
#include <booster/system_error.h>
#include <ctime>
#include <cppcms/service.h>  

class StrusContext {
	private:
		std::map<std::string, StrusIndexContext *> context_map;
		std::vector<const strus::ModuleEntryPoint *> modules;
        booster::aio::deadline_timer timer;
        time_t last_wake;
        unsigned int transaction_max_idle_time;

	public:
		strus::ErrorBufferInterface *errorhnd;

	public:
		StrusContext( cppcms::service *srv, unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules );
		virtual ~StrusContext( );

		StrusIndexContext *acquire( const std::string &name );
		void release( const std::string &name, StrusIndexContext *ctx );

        void lockIndex( const std::string &name, bool exclusive );
        void unlockIndex( const std::string &name );

		void registerModules( strus::QueryProcessorInterface *qpi ) const;

	private:
		void onTimer( booster::system::error_code const& e );
		void terminateIdleTransactions( );
};

#endif

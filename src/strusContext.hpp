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

#include <boost/thread.hpp>

#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"

#include "strus/moduleEntryPoint.hpp"

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
	boost::shared_mutex mutex;
	bool exclusive;

	public:
		StrusIndexContext( ) : name( "" ), config( "" ),
			dbi( 0 ), sti( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }

		StrusIndexContext( const std::string &_name, const std::string &_config )
			: name( _name ), config( _config ),
			dbi( 0 ), sti( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }
			
		virtual ~StrusIndexContext( )
		{
			abortAllRunningTransactions( );
			
			if( atri != 0 ) delete atri;
			if( mdri != 0 ) delete mdri;
			if( stci != 0 ) delete stci;
			if( sti != 0 ) delete sti;
			if( dbi != 0 ) delete dbi;
		}
		
		void read_lock( )
		{
			exclusive = false;
			mutex.lock_shared( );
		}
		
		void write_lock( )
		{
			exclusive = true;
			mutex.lock( );
		}
		
		void unlock( )
		{
			if( exclusive ) {
				mutex.unlock( );
			} else {
				mutex.unlock_shared( );
			}
		}
		
	private:
		void abortAllRunningTransactions( )
		{
			for( std::map<std::string, strus::StorageTransactionInterface *>::const_iterator it = trans_map.begin( ); it != trans_map.end( ); it++ ) {
				BOOSTER_INFO( PACKAGE ) << "forcing rollback on transaction '" << it->first << "' in index '" << name << "'";
				it->second->rollback( );
				delete it->second;
			}
		}
};

class StrusContext {
	private:
		std::map<std::string, StrusIndexContext *> context_map;
		//~ booster::mutex map_mutex;
		std::vector<const strus::ModuleEntryPoint *> modules;

	public:
		strus::ErrorBufferInterface *errorhnd;

	public:
		StrusContext( unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules );
		virtual ~StrusContext( );

		StrusIndexContext *acquire( const std::string &name );
		void release( const std::string &name, StrusIndexContext *ctx );

        void lockIndex( const std::string &name, bool exclusive );
        void unlockIndex( const std::string &name );

		void registerModules( strus::QueryProcessorInterface *qpi ) const;
};

#endif

/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "strusContext.hpp"
#include "version.hpp"
#include "constants.hpp"

#include <booster/log.h>

#include <boost/filesystem.hpp>

#include "strus/storageModule.hpp"
#include "strus/versionModule.hpp"
#include "strus/versionAnalyzer.hpp"
#include "strus/versionTrace.hpp"

#include <booster/intrusive_ptr.h>
#include <boost/bind.hpp>

static bool matchModuleVersion( const strus::ModuleEntryPoint* entryPoint, int& errorcode )
{
	const char* loader_signature = STRUS_MODULE_SIGNATURE;
	//... signature contains major version number in it
	if (std::strcmp( entryPoint->signature, loader_signature) != 0)
	{
		errorcode = strus::ModuleEntryPoint::ErrorSignature;
		return false;
	}
	unsigned short expected_modversion_minor = STRUS_MODULE_VERSION_MINOR;
	if (entryPoint->modversion_minor > expected_modversion_minor)
	{
		errorcode = strus::ModuleEntryPoint::ErrorModMinorVersion;
		return false;
	}
	unsigned short expected_compversion_major = 0;
	unsigned short expected_compversion_minor = 0;
	switch (entryPoint->type)
	{
		case strus::ModuleEntryPoint::Analyzer:
			expected_compversion_major = STRUS_ANALYZER_VERSION_MAJOR;
			expected_compversion_minor = STRUS_ANALYZER_VERSION_MINOR;
			break;

		case strus::ModuleEntryPoint::Storage:
			expected_compversion_major = STRUS_STORAGE_VERSION_MAJOR;
			expected_compversion_minor = STRUS_STORAGE_VERSION_MINOR;
			break;

		case strus::ModuleEntryPoint::Trace:
			expected_compversion_major = STRUS_TRACE_VERSION_MAJOR;
			expected_compversion_minor = STRUS_TRACE_VERSION_MINOR;
			break;

		default:
			errorcode = strus::ModuleEntryPoint::ErrorUnknownModuleType;
			return false;
	}
	if (entryPoint->compversion_major != expected_compversion_major)
	{
		errorcode = strus::ModuleEntryPoint::ErrorCompMajorVersion;
		return false;
	}
	if (entryPoint->compversion_minor < expected_compversion_minor)
	{
		errorcode = strus::ModuleEntryPoint::ErrorCompMinorVersion;
		return false;
	}
	return true;
}

StrusContext::StrusContext( cppcms::service *srv, unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules_ )
	: context_map( ), modules( ), timer( srv->get_io_service( ) )
{	
	errorhnd = strus::createErrorBuffer_standard( 0, nof_threads );
	
	BOOSTER_DEBUG( PACKAGE ) << "Search directory for modules implementing extensions is '" << moduleDir << "'";	
	
	for( std::vector<std::string>::const_iterator it = modules_.begin( ); it != modules_.end( ); it++ ) {
		boost::filesystem::path path( moduleDir );
		boost::filesystem::path fullPath = boost::filesystem::absolute( path ) /= *it;
		BOOSTER_DEBUG( PACKAGE ) << "Loading module '" << fullPath.string( ) << "'";

		strus::ModuleEntryPoint::Status status;
		strus::ModuleEntryPoint::Handle hnd;
		const strus::ModuleEntryPoint *entrypoint = strus::loadModuleEntryPoint( fullPath.string( ).c_str( ), status, hnd, &matchModuleVersion );

		if( !entrypoint ) {
			BOOSTER_WARNING( PACKAGE ) << "failed loading extension module '" << fullPath.string( ) << "': " << status.errormsg;
			continue;
		}

		if( entrypoint->type != strus::ModuleEntryPoint::Storage ) {
			BOOSTER_WARNING( PACKAGE ) << "module '" << fullPath.string( ) << "' is not a storage module and is not loaded into the server";
			continue;
		}

		modules.push_back( entrypoint );
	}

	transaction_max_idle_time = srv->settings( ).get<unsigned int>( "transactions.max_idle_time", DEFAULT_TRANSACTION_MAX_IDLE_TIME );

	last_wake = time( 0 );
	onTimer( booster::system::error_code( ) );
}

StrusContext::~StrusContext( )
{
	BOOSTER_DEBUG( PACKAGE ) << "Shutting down strus context";
	for( std::map<std::string, StrusIndexContext *>::iterator it = context_map.begin( ); it != context_map.end( ); it++ ) {
		delete it->second;
	}
}

void StrusContext::registerModules( strus::QueryProcessorInterface *qpi ) const
{
	for( std::vector<const strus::ModuleEntryPoint *>::const_iterator it = modules.begin( ); it != modules.end( ); it++ ) {
		const strus::StorageModule *module = reinterpret_cast<const strus::StorageModule *>( *it );
		if( module->weightingFunctionConstructor ) {
			strus::WeightingFunctionConstructor const *constructor = module->weightingFunctionConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::WeightingFunctionInterface *func = constructor->create( errorhnd );
				if( !func ) {
					BOOSTER_WARNING( PACKAGE ) << "weighting function '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->defineWeightingFunction( constructor->name, func );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering weighting function '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}
		
		if( module->summarizerFunctionConstructor ) {
			strus::SummarizerFunctionConstructor const *constructor = module->summarizerFunctionConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::SummarizerFunctionInterface *func = constructor->create( errorhnd );
				if( !func ) {
					BOOSTER_WARNING( PACKAGE ) << "summarizer function '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->defineSummarizerFunction( constructor->name, func );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering summarizer function '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}
		
		if( module->postingIteratorJoinConstructor ) {
			strus::PostingIteratorJoinConstructor const *constructor = module->postingIteratorJoinConstructor;
			for( ; constructor->create != 0; constructor++ ) {
				strus::PostingJoinOperatorInterface *op = constructor->create( errorhnd );
				if( !op ) {
					BOOSTER_WARNING( PACKAGE ) << "posting join operator '" << constructor->name << "' cannot be constructed";
					continue;
				}
				qpi->definePostingJoinOperator( constructor->name, op );
				if( errorhnd->hasError( ) ) {
					BOOSTER_WARNING( PACKAGE ) << "registering posting join operator '" <<
						constructor->name << "' resulted in an error: " << errorhnd->fetchError( );
				}
			}
		}					
	} 
}

StrusIndexContext *StrusContext::acquire( const std::string &name )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return 0;
	} else {
		return it->second;
	}
}

void StrusContext::release( const std::string &name, StrusIndexContext *ctx )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		context_map[name] = ctx;
	} else {
		it->second = ctx;
	}
}

void StrusContext::lockIndex( const std::string &name, bool exclusive )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it != context_map.end( ) ) {
		if( exclusive ) {
			it->second->write_lock( );
		} else {
			it->second->read_lock( );
		}
	}
}

void StrusContext::unlockIndex( const std::string &name )
{
	std::map<std::string, StrusIndexContext *>::iterator it;
	it = context_map.find( name );
	if( it != context_map.end( ) ) {
		it->second->unlock( );
	}
}

void StrusContext::terminateIdleTransactions( )
{
	BOOSTER_DEBUG( PACKAGE ) << "Checking for long-running transactions";
	for( std::map<std::string, StrusIndexContext *>::iterator it = context_map.begin( ); it != context_map.end( ); it++ ) {
		it->second->terminateIdleTransactions( transaction_max_idle_time );
	}
}

void StrusContext::onTimer( booster::system::error_code const& e )
{
	if( e ) return;
	
	if( time( 0 ) - last_wake > DEFAULT_HOUSKEEPING_TIMER_INTERVAL ) {
		last_wake = time( 0 );		
		terminateIdleTransactions( );
	}
	
	timer.expires_from_now( booster::ptime::seconds( 1 ) );
	timer.async_wait( boost::bind( &StrusContext::onTimer, this, _1 ) );
}

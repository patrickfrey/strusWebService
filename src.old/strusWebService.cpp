/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "strusWebService.hpp"

#include <booster/log.h>

#include <cppcms/service.h>

#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"

#include <booster/locale/format.h>

#include <boost/filesystem.hpp>

#include <sstream>

#include "strus/lib/queryeval.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/storageTransactionInterface.hpp"

#include <signal.h>
#include <ctime>

namespace apps {

strusWebService::strusWebService( cppcms::service &_srv, StrusContext *_context, bool pretty_print )
	: cppcms::application( _srv ), srv( _srv ), context( _context ),
	storage_base_directory( settings( ).get<std::string>( "storage.basedir" ) ),
	qpi( 0 ), qei( 0 ), log_requests( DEFAULT_DEBUG_LOG_REQUESTS ),
	master( *this ),
	other( *this ),
	index( *this, settings( ).get<std::string>( "storage.basedir" ) ),
	document( *this ),
	query( *this ),
	transaction( *this )
{
	BOOSTER_DEBUG( PACKAGE ) << "Starting strus web service";
	
	add( master );
	add( other );
	add( index );
	add( document );
	add( query );
	add( transaction );
			
	if( settings( ).get<bool>( "democlient.enable" ) ) {
		master.register_democlient_pages( );
	}
	master.register_common_pages( );
	
	if( !pretty_print ) {
		pretty_print = settings( ).get<bool>( "debug.protocol.pretty_print", DEFAULT_DEBUG_PROTOCOL_PRETTY_PRINT );
	}
	master.set_pretty_printing( pretty_print );
	other.set_pretty_printing( pretty_print );
	index.set_pretty_printing( pretty_print );
	document.set_pretty_printing( pretty_print );
	query.set_pretty_printing( pretty_print );
	transaction.set_pretty_printing( pretty_print );
	
	log_requests = settings( ).get<bool>( "debug.log_requests", DEFAULT_DEBUG_LOG_REQUESTS );
	log_request_filename = settings( ).get<std::string>( "debug.request_file", DEFAULT_DEBUG_REQUEST_FILE );
	
	master.set_log_requests( log_requests );
	other.set_log_requests( log_requests );
	index.set_log_requests( log_requests );
	document.set_log_requests( log_requests );
	query.set_log_requests( log_requests );
	transaction.set_log_requests( log_requests );
	
	bool allow_quit_command = settings( ).get<bool>( "debug.protocol.enable_quit_command", DEFAULT_DEBUG_PROTOCOL_ENABLE_QUIT_COMMAND );
	other.set_allow_quit_command( allow_quit_command );
}

strusWebService::~strusWebService( )
{
	BOOSTER_DEBUG( PACKAGE ) << "Shutting down strus web service";

	for( std::map<pthread_t, std::ofstream *>::iterator it = log_request_streams.begin( ); it != log_request_streams.end( ); it++ ) {
		delete it->second;
	}
}

StrusIndexContext *strusWebService::getOrCreateStrusContext( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		ctx = new StrusIndexContext( name, getStorageConfig( storage_base_directory, name ) );
	}
	context->release( name, ctx );
	return ctx;
}

void strusWebService::registerStorageConfig( const std::string &name, const std::string &config )
{
	// TODO: actually we should rather throw here (why?)
	StrusIndexContext *ctx = getOrCreateStrusContext( name );
	ctx->config = config;
	context->release( name, ctx );
}

strus::DatabaseInterface *strusWebService::getDataBaseInterface( const std::string &name )
{
	StrusIndexContext *ctx = getOrCreateStrusContext( name );
	if( ctx->dbi == 0 ) {
		strus::DatabaseInterface *dbi = strus::createDatabaseType_leveldb( context->errorhnd );
		if( dbi == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->dbi = dbi;
	}
	context->release( name, ctx );
	return ctx->dbi;	
}

strus::StorageInterface *strusWebService::getStorageInterface( const std::string &name )
{
	StrusIndexContext *ctx = getOrCreateStrusContext( name );
	if( ctx->sti == 0 ) {
		strus::StorageInterface *sti = strus::createStorageType_std( context->errorhnd );
		if( sti == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->sti = sti;
	}
	context->release( name, ctx );
	return ctx->sti;
}

strus::StorageClientInterface *strusWebService::getStorageClientInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx->stci == 0 ) {
		strus::StorageClientInterface *stci = ctx->sti->createClient( ctx->config, ctx->dbi, 0 );
		if( stci == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->stci = stci;
	}
	context->release( name, ctx );
	return ctx->stci;
}

strus::MetaDataReaderInterface *strusWebService::getMetaDataReaderInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx->mdri == 0 ) {
		strus::MetaDataReaderInterface *mdri = ctx->stci->createMetaDataReader( );
		if( mdri == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->mdri = mdri;
	}
	context->release( name, ctx );
	return ctx->mdri;
}

strus::AttributeReaderInterface *strusWebService::getAttributeReaderInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx->atri == 0 ) {
		strus::AttributeReaderInterface *atri = ctx->stci->createAttributeReader( );
		if( atri == 0 ) {
			context->release( name, ctx );
			return 0;
		}
		ctx->atri = atri;
	}
	context->release( name, ctx );
	return ctx->atri;
}

strus::StorageTransactionInterface *strusWebService::createStorageTransactionInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	strus::StorageTransactionInterface *stti = ctx->stci->createTransaction( );
	context->release( name, ctx );
	return stti;
}

strus::StorageTransactionInterface *strusWebService::createStorageTransactionInterface( const std::string &name, const std::string &id )
{
	StrusIndexContext *ctx = context->acquire( name );
	std::map<std::string, StrusTransactionInfo>::iterator it;
	it = ctx->trans_map.find( id );
	strus::StorageTransactionInterface *stti;
	if( it == ctx->trans_map.end( ) ) {
		stti = ctx->stci->createTransaction( );
		if( stti == 0 ) {
			return 0;
		}
		ctx->trans_map[id].stti = stti;
		ctx->trans_map[id].last_used = time( 0 );
	} else {
		ctx->trans_map[id].last_used = time( 0 );
		stti = ctx->trans_map[id].stti;
	}
	context->release( name, ctx );
	return stti;
}

strus::StorageTransactionInterface *strusWebService::getStorageTransactionInterface( const std::string &name, const std::string &id )
{
	StrusIndexContext *ctx = context->acquire( name );
	std::map<std::string, StrusTransactionInfo>::iterator it;
	it = ctx->trans_map.find( id );
	strus::StorageTransactionInterface *stti;
	if( it == ctx->trans_map.end( ) ) {
		return 0;
	} else {
		ctx->trans_map[id].last_used = time( 0 );
		stti = ctx->trans_map[id].stti;
	}
	context->release( name, ctx );
	return stti;
}

std::vector<TransactionData> strusWebService::getAllTransactionsDataOfIndex( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	std::vector<TransactionData> v;
	if( ctx == 0 ) {
		return v;
	}
	std::map<std::string, StrusTransactionInfo>::iterator it;
	for( it = ctx->trans_map.begin( ); it != ctx->trans_map.end( ); it++ ) {
		TransactionData d;
		d.id = (*it).first;
		d.age = time( 0 ) - (*it).second.last_used;
		v.push_back( d );
	}
	context->release( name, ctx );
	return v;
}

strus::QueryEvalInterface *strusWebService::getQueryEvalInterface( )
{
	if( qei == 0 ) {
		qei = strus::createQueryEval( context->errorhnd );
	}
	return qei;
}

strus::QueryProcessorInterface *strusWebService::getQueryProcessorInterface( )
{
	if( qpi == 0 ) {
		qpi = strus::createQueryProcessor( context->errorhnd );
		context->registerModules( qpi );
	}
	return qpi;
}

std::string strusWebService::getConfigString( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	return ctx->config;
}

void strusWebService::deleteDataBaseInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		return;
	}
	if( ctx->dbi != 0 ) {
		delete ctx->dbi;
		ctx->dbi = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		return;
	}
	if( ctx->sti != 0 ) {
		delete ctx->sti;
		ctx->sti = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageClientInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		return;
	}
	if( ctx->stci != 0 ) {
		delete ctx->stci;
		ctx->stci = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteMetaDataReaderInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		return;
	}
	if( ctx->mdri != 0 ) {
		delete ctx->mdri;
		ctx->mdri = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteAttributeReaderInterface( const std::string &name )
{
	StrusIndexContext *ctx = context->acquire( name );
	if( ctx == 0 ) {
		return;
	}
	if( ctx->atri != 0 ) {
		delete ctx->atri;
		ctx->atri = 0;
	}
	context->release( name, ctx );
}

void strusWebService::deleteStorageTransactionInterface( const std::string &name, const std::string &id )
{
	StrusIndexContext *ctx = context->acquire( name );
	std::map<std::string, StrusTransactionInfo>::iterator it;
	it = ctx->trans_map.find( id );
	if( it != ctx->trans_map.end( ) ) {
		delete it->second.stti;
		ctx->trans_map.erase( it );
	}
}

void strusWebService::deleteQueryEvalInterface(  )
{
	if( qei != 0 ) {
		delete qei;
		qei = 0;
	}
}

void strusWebService::deleteQueryProcessorInterface( )
{
	if( qpi != 0 ) {
		delete qpi;
		qpi = 0;
	}
}

bool strusWebService::hasError( ) const
{
	return context->errorhnd->hasError( );
}

std::string strusWebService::getLastStrusError( ) const
{
	return context->errorhnd->fetchError( );
}

std::string strusWebService::getStorageDirectory( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "{1}/{2}" ) % base_storage_dir % name;
	std::string directory = ss.str( );

	return directory;
}

std::string strusWebService::getStorageConfig( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name )
{
	std::ostringstream ss;
	std::ostringstream ss2;
	bool first = true;
	std::vector<struct MetadataDefiniton>::const_iterator it;
	
	for( it = params.metadata.begin( ); it != params.metadata.end( ); it++ ) {
		if( !first ) {
			ss2 << ", ";		
		} else {
			first = false;
		}
		ss2 << it->name << " " << it->type;
	}
	
	ss << booster::locale::format( "database={1}; path={2}; compression={3}; cache={4}; max_open_files={5}; write_buffer_size={6}; block_size={7}; metadata={8}" )
		% params.database
		% getStorageDirectory( base_storage_dir, name )
		% ( params.compression ? "yes" : "no" )
		% params.cache_size
		% params.max_open_files
		% params.write_buffer_size
		% params.block_size
		% ss2.str( );
	std::string config = ss.str( );

	BOOSTER_DEBUG( PACKAGE ) << "Storage config string: " << config;
	
	return config;
}

std::string strusWebService::getStorageConfig( const std::string &base_storage_dir, const std::string &name )
{
	std::ostringstream ss;
	
	ss << booster::locale::format( "path={1}" )
		% getStorageDirectory( base_storage_dir, name );
	std::string config = ss.str( );

	BOOSTER_DEBUG( PACKAGE ) << "Simple storage config string: " << config;
	
	return config;	
}

void strusWebService::abortRunningTransactions( const std::string &name )
{
	std::vector<TransactionData> transactions = getAllTransactionsDataOfIndex( name );
	for( std::vector<TransactionData>::const_iterator trans_data = transactions.begin( ); trans_data != transactions.end( ); trans_data++ ) {
		strus::StorageTransactionInterface *transaction = getStorageTransactionInterface( name, trans_data->id );
		if( transaction != 0 ) {
			BOOSTER_INFO( PACKAGE ) << "forcing rollback on transaction '" << trans_data->id << "' in index '" << name << "'";
			transaction->rollback( );
			deleteStorageTransactionInterface( name, trans_data->id );
		}
	}
}

void strusWebService::abortAllRunningTransactions( )
{
	std::vector<std::string> indexes = getAllIndexNames( );
	
	for( std::vector<std::string>::const_iterator index = indexes.begin( ); index != indexes.end( ); index++ ) {
		abortRunningTransactions( *index );
	}
}

std::vector<std::string> strusWebService::getAllIndexNames( )
{
	typedef std::vector<boost::filesystem::directory_entry> dirlist;
	dirlist dirs;
	
	boost::filesystem::path dir( storage_base_directory );

	std::vector<std::string> v;

	if( !exists( dir ) ) {
		return v;
	}
				  
	std::copy( boost::filesystem::directory_iterator( storage_base_directory ),
		boost::filesystem::directory_iterator( ), std::back_inserter( dirs ) );

	for( dirlist::const_iterator it = dirs.begin( ); it != dirs.end( ); it++ ) {
		std::string last;
		for( boost::filesystem::path::iterator pit = it->path( ).begin( ); pit != it->path( ).end( ); pit++ ) {
			last = pit->native( );
		}
		v.push_back( last );
	}
	
	return v;
}

void strusWebService::raiseTerminationFlag( )
{
	srv.shutdown( );
}

std::ofstream *strusWebService::log_request_stream( )
{
// TODO: have booster::thread support for this (or am I missing something?)
#ifndef _WIN32
	pthread_t tid = pthread_self( );
	
	std::map<pthread_t, std::ofstream *>::iterator it;
	it = log_request_streams.find( tid );
	if( it == log_request_streams.end( ) ) {
		// TODO: boost::filesystem::like paths needed
		std::ostringstream ss;
		ss << log_request_filename << "-" << tid;
		std::ofstream *os = new std::ofstream( ss.str( ).c_str( ), std::ofstream::out | std::ofstream::app );
		log_request_streams[tid] = os;
		return os;
	} else {
		return it->second;
	}
#else
#error We need pthread identifier on Windows
#endif
}

void strusWebService::lockIndex( const std::string &name, bool exclusive )
{
	context->lockIndex( name, exclusive );
}

void strusWebService::unlockIndex( const std::string &name )
{
	context->unlockIndex( name );
}

} // namespace apps

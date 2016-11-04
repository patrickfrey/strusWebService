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
#include "transaction.hpp"

#include <booster/log.h>

#include <boost/timer/timer.hpp>

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <cppcms/json.h>

namespace apps {

transaction::transaction( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/transaction/begin/(\\w+)/(\\w+)", &transaction::begin_url_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/transaction/begin/(\\w+)", &transaction::begin_payload_cmd, this, 1  );
	service.dispatcher( ).assign( "/transaction/commit/(\\w+)/(\\w+)", &transaction::commit_url_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/transaction/commit/(\\w+)", &transaction::commit_payload_cmd, this, 1  );
	service.dispatcher( ).assign( "/transaction/rollback/(\\w+)/(\\w+)", &transaction::rollback_url_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/transaction/rollback/(\\w+)", &transaction::rollback_payload_cmd, this, 1  );
	service.dispatcher( ).assign( "/transaction/list/(\\w+)", &transaction::list_cmd, this, 1 );
}

void transaction::begin_url_cmd( const std::string name, const std::string tid )
{
	begin_cmd( name, tid, true );
}

void transaction::begin_payload_cmd( const std::string name )
{
	begin_cmd( name, "", false );
}

void transaction::begin_cmd( const std::string name, const std::string tid, bool tid_in_url )
{	
	if( !ensure_post( ) ) return;	

	log_request( );

	struct TransactionBeginRequest trans_begin;

	if( !tid_in_url ) {
		if( !ensure_json_request( ) ) return;
	
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_TRANSACTION_BEGIN_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
	
		if( p.type( "transaction" ) == cppcms::json::is_object ) {
			try {
				trans_begin = p.get<struct TransactionBeginRequest>( "transaction" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_TRANSACTION_BEGIN_ILLEGAL_JSON, "Illegal JSON transaction payload received" );
				return;
			}
		} else {
			report_error( ERROR_TRANSACTION_BEGIN_ILLEGAL_JSON, "Expecting a JSON object as JSON transaction payload" );
			return;
		}
	}
	
	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_TRANSACTION_BEGIN_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_TRANSACTION_BEGIN_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// id can come from the JSON payload or directly in the URL
	std::string trans_id;
	if( tid_in_url ) {
		trans_id = tid;
	} else {
		if( trans_begin.id.compare( "" ) == 0 ) {
			report_error( ERROR_TRANSACTION_BEGIN_CMD_DOCID_REQUIRED, "transaction id 'id' must be part of the JSON payload as field of 'transaction'" );
			return;
		}
		trans_id = trans_begin.id;
	}

	strus::StorageTransactionInterface *transaction = service.createStorageTransactionInterface( name, trans_id );
	if( !transaction ) {
		report_error( ERROR_TRANSACTION_BEGIN_CMD_CREATE_STORAGE_TRANSACTION, service.getLastStrusError( ) );
		return;
	}

	BOOSTER_INFO( PACKAGE ) << "begin(" << name << ", " << trans_id << ")";
	
	report_ok( );	
}

void transaction::commit_url_cmd( const std::string name, const std::string tid )
{
	commit_cmd( name, tid, true );
}

void transaction::commit_payload_cmd( const std::string name )
{
	commit_cmd( name, "", false );
}

void transaction::commit_cmd( const std::string name, const std::string tid, bool tid_in_url )
{
	boost::timer::cpu_timer timer;

	if( !ensure_post( ) ) return;	

	log_request( );

	struct TransactionBeginRequest trans_commit;

	if( !tid_in_url ) {
		if( !ensure_json_request( ) ) return;
		
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_TRANSACTION_COMMIT_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "transaction" ) == cppcms::json::is_object ) {
			try {
				trans_commit = p.get<struct TransactionBeginRequest>( "transaction" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_TRANSACTION_COMMIT_ILLEGAL_JSON, "Illegal JSON transaction payload received" );
				return;
			}
		} else {
			report_error( ERROR_TRANSACTION_COMMIT_ILLEGAL_JSON, "Expecting a JSON object as JSON transaction payload" );
			return;
		}
	}
	
	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_TRANSACTION_COMMIT_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_TRANSACTION_COMMIT_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// id can come from the JSON payload or directly in the URL
	std::string trans_id;
	if( tid_in_url ) {
		trans_id = tid;
	} else {
		if( trans_commit.id.compare( "" ) == 0 ) {
			report_error( ERROR_TRANSACTION_COMMIT_CMD_DOCID_REQUIRED, "transaction id 'id' must be part of the JSON payload as field of 'transaction'" );
			return;
		}
		trans_id = trans_commit.id;
	}

	strus::StorageTransactionInterface *transaction = service.getStorageTransactionInterface( name, trans_id );
	if( !transaction ) {
		report_error( ERROR_TRANSACTION_COMMIT_CMD_GET_STORAGE_TRANSACTION, service.getLastStrusError( ) );
		return;
	}

	transaction->commit( );

	service.deleteStorageTransactionInterface( name, trans_id );

	cppcms::json::value j;
	double execution_time = (double)timer.elapsed( ).wall / (double)1000000000;
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "commit(" << name << ", " << trans_id << ", " << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}	
	BOOSTER_DEBUG( PACKAGE ) << "commit(" << name << ", " << trans_id << "): " << ss.str( );

	report_ok( j );
}

void transaction::rollback_url_cmd( const std::string name, const std::string tid )
{
	rollback_cmd( name, tid, true );
}

void transaction::rollback_payload_cmd( const std::string name )
{
	rollback_cmd( name, "", false );
}

void transaction::rollback_cmd( const std::string name, const std::string tid, bool tid_in_url )
{
	boost::timer::cpu_timer timer;

	if( !ensure_post( ) ) return;	

	log_request( );

	struct TransactionBeginRequest trans_rollback;

	if( !tid_in_url ) {
		if( !ensure_json_request( ) ) return;
		
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_TRANSACTION_ROLLBACK_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "transaction" ) == cppcms::json::is_object ) {
			try {
				trans_rollback = p.get<struct TransactionBeginRequest>( "transaction" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_TRANSACTION_ROLLBACK_ILLEGAL_JSON, "Illegal JSON transaction payload received" );
				return;
			}
		} else {
			report_error( ERROR_TRANSACTION_ROLLBACK_ILLEGAL_JSON, "Expecting a JSON object as JSON transaction payload" );
			return;
		}
	}
	
	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_TRANSACTION_ROLLBACK_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_TRANSACTION_ROLLBACK_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// id can come from the JSON payload or directly in the URL
	std::string trans_id;
	if( tid_in_url ) {
		trans_id = tid;
	} else {
		if( trans_rollback.id.compare( "" ) == 0 ) {
			report_error( ERROR_TRANSACTION_ROLLBACK_CMD_DOCID_REQUIRED, "transaction id 'id' must be part of the JSON payload as field of 'transaction'" );
			return;
		}
		trans_id = trans_rollback.id;
	}

	strus::StorageTransactionInterface *transaction = service.getStorageTransactionInterface( name, trans_id );
	if( !transaction ) {
		report_error( ERROR_TRANSACTION_ROLLBACK_CMD_GET_STORAGE_TRANSACTION, service.getLastStrusError( ) );
		return;
	}

	transaction->rollback( );

	service.deleteStorageTransactionInterface( name, trans_id );
	
	cppcms::json::value j;
	double execution_time = (double)timer.elapsed( ).wall / (double)1000000000;
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "rollback(" << name << ", " << trans_id << ", " << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "rollback(" << name << ", " << trans_id << "): " << ss.str( );

	report_ok( j );
}

void transaction::list_cmd( const std::string name )
{
	boost::timer::cpu_timer timer;

	log_request( );

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_TRANSACTION_LIST_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}
	
	cppcms::json::value j;
	j["result"] = "ok";
	std::vector<std::string> v = service.getAllTransactionsIdsOfIndex( name );
	std::sort( v.begin( ), v.end( ) );
	j["transactions"] = v;
	double execution_time = (double)timer.elapsed( ).wall / (double)1000000000;
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "list_transactions(" << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "list_transactions: " << ss.str( );

	report_ok( j );
}

} // namespace apps

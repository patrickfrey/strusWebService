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

#include "strusWebService.hpp"
#include "transaction.hpp"

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

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_TRANSACTION_BEGIN_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
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
	if( !ensure_post( ) ) return;	

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

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_TRANSACTION_COMMIT_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
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
	
	report_ok( );
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
	if( !ensure_post( ) ) return;	

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

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_TRANSACTION_ROLLBACK_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
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
	
	report_ok( );
}

} // namespace apps

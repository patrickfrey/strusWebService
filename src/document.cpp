#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/constants.hpp"

#include <sstream>

namespace apps {

document::document( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/document/insert/(\\w+)/(\\w+)", &document::insert_at_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/document/insert/(\\w+)", &document::insert_cmd, this, 1  );
	service.dispatcher( ).assign( "/document/update/(\\w+)/(\\w+)", &document::update_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)/(\\w+)", &document::delete_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/get/(\\w+)/(\\w+)", &document::get_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/exists/(\\w+)/(\\w+)", &document::exists_cmd, this, 1, 2 );	
}

void document::insert_at_cmd( const std::string name, const std::string id  )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	struct DocumentInsertRequest ins_doc;
	
	std::pair<void *, size_t> data = request( ).raw_post_data( );
	std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
	cppcms::json::value p;
	if( !p.load( is, true) ) {
		report_error( ERROR_DOCUMENT_INSERT_ILLEGAL_JSON, "Illegal JSON received" );
		return;
	}
	
	if( p.type( "doc" ) == cppcms::json::is_object ) {
		try {
			ins_doc = p.get<struct DocumentInsertRequest>( "doc" );
		} catch( cppcms::json::bad_value_cast &e ) {
			report_error( ERROR_DOCUMENT_INSERT_ILLEGAL_JSON, "Illegal JSON document payload received" );
			return;
		}
	} else {
		report_error( ERROR_DOCUMENT_INSERT_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
		return;
	}
		
	get_strus_environment( name );

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_DOCUMENT_INSERT_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_DOCUMENT_INSERT_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageTransactionInterface *transaction = service.getStorageTransactionInterface( name );
	if( !transaction ) {
		report_error( ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_TRANSACTION, service.getLastStrusError( ) );
		return;
	}
		
	strus::StorageDocumentInterface *doc = transaction->createDocument( id );

	doc->setAttribute( strus::Constants::attribute_docid( ), id );

	doc->done( );
	
	transaction->commit( );
	
	delete doc;
	service.deleteStorageTransactionInterface( name );
		
	report_ok( );	
}

void document::insert_cmd( const std::string name )
{
	// TODO: insert command with docid as part of the payload (for instance
	// if the URL is something which doesn't fit into the URL, this is in
	// violation of rest though. And for the index name we are not that picky..
}

void document::update_cmd( const std::string name, const std::string id )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	get_strus_environment( name );

	report_ok( );
}

void document::delete_cmd( const std::string name, const std::string id )
{
	if( !ensure_post( ) ) return;	

	get_strus_environment( name );
	
	report_ok( );	
}

void document::get_cmd( const std::string name, const std::string id )
{
	get_strus_environment( name );

	report_ok( );
}

void document::exists_cmd( const std::string name , const std::string id )
{
	get_strus_environment( name );

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_DOCUMENT_EXISTS_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_DOCUMENT_EXISTS_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_DOCUMENT_EXISTS_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}
	
	strus::Index docno = storage->documentNumber( id );
	
	cppcms::json::value j;
	j["docno"] = docno;
	report_ok( j );
}

} // namespace apps

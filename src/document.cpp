#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <booster/log.h>

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/constants.hpp"

#include <sstream>

namespace apps {

document::document( strusWebService &service )
	: master( service )
{
	// TODO: have two variants, GET/POST and docid in URL as well
	// as always POST with docid in JSON request
	// so far done for 'insert'
	service.dispatcher( ).assign( "/document/insert/(\\w+)/(\\w+)", &document::insert_url_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/document/insert/(\\w+)", &document::insert_payload_cmd, this, 1  );
	service.dispatcher( ).assign( "/document/update/(\\w+)/(\\w+)", &document::update_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)/(\\w+)", &document::delete_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/get/(\\w+)/(\\w+)", &document::get_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/exists/(\\w+)/(\\w+)", &document::exists_cmd, this, 1, 2 );	
}

void document::insert_url_cmd( const std::string name, const std::string id )
{
	insert_cmd( name, id, true );
}

void document::insert_payload_cmd( const std::string name )
{
	insert_cmd( name, "", false );
}

void document::insert_cmd( const std::string name, const std::string id, bool docid_id_url )
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
	
	std::string docid;
	if( docid_id_url ) {
		docid = id;
	} else {
		if( ins_doc.docid.compare( "" ) == 0 ) {
			report_error( ERROR_DOCUMENT_INSERT_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = ins_doc.docid;
	}
		
	strus::StorageDocumentInterface *doc = transaction->createDocument( docid );

	doc->setAttribute( strus::Constants::attribute_docid( ), docid );

	for( std::vector<std::pair<std::string, std::string> >::const_iterator it = ins_doc.attributes.begin( );
		it != ins_doc.attributes.end( ); it++ ) {
		doc->setAttribute( it->first, it->second );
	}
	
	for( std::vector<std::pair<std::string, strus::ArithmeticVariant> >::const_iterator it = ins_doc.metadata.begin( );
		it != ins_doc.metadata.end( ); it++ ) {
		doc->setMetaData( (*it).first, (*it).second );
	}


/*
 * 							std::vector<strus::analyzer::MetaData>::const_iterator
								mi = doc.metadata().begin(), me = doc.metadata().end();
							for (; mi != me; ++mi)
							{
								double val = mi->value();
								if (val - std::floor( val) < std::numeric_limits<float>::epsilon())
								{
									if (val < 0.0)
									{
										strus::ArithmeticVariant av( (int)(std::floor( val) + std::numeric_limits<float>::epsilon()));
										storagedoc->setMetaData( mi->name(), av);
									}
									else
									{
										strus::ArithmeticVariant av( (unsigned int)(std::floor( val) + std::numeric_limits<float>::epsilon()));
										storagedoc->setMetaData( mi->name(), av);
									}
								}
								else
								{
									storagedoc->setMetaData( mi->name(), (float) val);
								}
							}

 */
	doc->done( );
	
	transaction->commit( );
	
	delete doc;
	service.deleteStorageTransactionInterface( name );
		
	report_ok( );	
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
	bool exists = ( docno > 0 );	
		
	cppcms::json::value j;
	j["exists"] = exists;
	
	report_ok( j );
}

} // namespace apps

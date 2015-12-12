#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <booster/log.h>
#include <booster/locale/format.h>

#include <booster/log.h>

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/constants.hpp"

#include <sstream>
#include <algorithm>

namespace apps {

document::document( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/document/insert/(\\w+)/(\\w+)", &document::insert_url_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/document/insert/(\\w+)", &document::insert_payload_cmd, this, 1  );
	service.dispatcher( ).assign( "/document/update/(\\w+)/(\\w+)", &document::update_url_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/update/(\\w+)", &document::update_payload_cmd, this, 1 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)/(\\w+)", &document::delete_url_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)", &document::delete_payload_cmd, this, 1 );	
	service.dispatcher( ).assign( "/document/get/(\\w+)/(\\w+)", &document::get_url_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/get/(\\w+)", &document::get_payload_cmd, this, 1 );
	service.dispatcher( ).assign( "/document/exists/(\\w+)/(\\w+)", &document::exists_url_cmd, this, 1, 2 );	
	service.dispatcher( ).assign( "/document/exists/(\\w+)", &document::exists_payload_cmd, this, 1 );	
}

void document::insert_url_cmd( const std::string name, const std::string id )
{
	insert_cmd( name, id, true );
}

void document::insert_payload_cmd( const std::string name )
{
	insert_cmd( name, "", false );
}

void document::insert_cmd( const std::string name, const std::string id, bool docid_in_url )
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
	} else if( p.type( "doc" ) == cppcms::json::is_null ) {
		BOOSTER_WARNING( PACKAGE ) << "Inserting an empty document, hope this is ok";
	} else {
		report_error( ERROR_DOCUMENT_INSERT_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
		return;
	}
	
	if( !get_strus_environment( name ) ) {
		return;
	}

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
	
	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( ins_doc.docid.compare( "" ) == 0 ) {
			service.deleteStorageTransactionInterface( name );
			report_error( ERROR_DOCUMENT_INSERT_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = ins_doc.docid;
	}
		
	strus::StorageDocumentInterface *doc = transaction->createDocument( docid );

	// the one attribute with fixed name 'docid' must always exist
	doc->setAttribute( strus::Constants::attribute_docid( ), docid );

	// attributes
	for( std::vector<std::pair<std::string, std::string> >::const_iterator it = ins_doc.attributes.begin( );
		it != ins_doc.attributes.end( ); it++ ) {
		doc->setAttribute( it->first, it->second );
	}
	
	// metadata
	for( std::vector<std::pair<std::string, strus::ArithmeticVariant> >::const_iterator it = ins_doc.metadata.begin( );
		it != ins_doc.metadata.end( ); it++ ) {
		doc->setMetaData( (*it).first, (*it).second );
	}
		
	// forward index
	strus::Index maxPos = 0;
	for( std::vector<boost::tuple<std::string, std::string, strus::Index> >::const_iterator it = ins_doc.forward.begin( );
		it != ins_doc.forward.end( ); it++ ) {
		strus::Index pos = boost::get<2>( *it );
		if( pos > strus::Constants::storage_max_position_info( ) ) {
			if( pos > maxPos ) {
				maxPos = pos;
			}
		} else {
			doc->addForwardIndexTerm( boost::get<0>( *it ), boost::get<1>( *it ), pos );
		}
	}
	
	// search index
	for( std::vector<boost::tuple<std::string, std::string, strus::Index> >::const_iterator it = ins_doc.search.begin( );
		it != ins_doc.search.end( ); it++ ) {
		strus::Index pos = boost::get<2>( *it );
		if( pos > strus::Constants::storage_max_position_info( ) ) {
			if( pos > maxPos ) {
				maxPos = pos;
			}
		} else {
			doc->addSearchIndexTerm( boost::get<0>( *it ), boost::get<1>( *it ), pos );
		}
	}

	if( maxPos > strus::Constants::storage_max_position_info( ) ) {
		std::ostringstream ss;
		ss << booster::locale::format( "Token positions of document {1} are out or range (document too big, only {2} token positions were assigned, maximum allowed position is %{3})" )
			% docid % maxPos % strus::Constants::storage_max_position_info( );
		delete doc;
		service.deleteStorageTransactionInterface( name );
		// TODO: warning or error?
		report_error( ERROR_DOCUMENT_INSERT_TOO_BIG_POSITION, ss.str( ) );
		return;
	}
	
	doc->done( );
	
	transaction->commit( );
	
	delete doc;
	service.deleteStorageTransactionInterface( name );
			
	report_ok( );	
}

void document::update_url_cmd( const std::string name, const std::string id )
{
	update_cmd( name, id, true );
}

void document::update_payload_cmd( const std::string name )
{
	update_cmd( name, "", false );
}

void document::update_cmd( const std::string name, const std::string id, bool docid_in_url )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;
	
	//TODO: update is metadata update, not document update!
	// document update is done via insert
	report_error( ERROR_NOT_IMPLEMENTED, "metadata, attribute, ACL updates are currently not implemented" );
}

void document::delete_url_cmd( const std::string name, const std::string id )
{
	delete_cmd( name, id, true );
}

void document::delete_payload_cmd( const std::string name )
{
	delete_cmd( name, "", false );
}

void document::delete_cmd( const std::string name, const std::string id, bool docid_in_url )
{
	if( !ensure_post( ) ) return;	
	if( !docid_in_url ) {
		if( !ensure_json_request( ) ) return;
	}

	struct DocumentDeleteRequest del_doc;
	
	if( !docid_in_url ) {
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_DOCUMENT_DELETE_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "doc" ) == cppcms::json::is_object ) {
			try {
				del_doc = p.get<struct DocumentDeleteRequest>( "doc" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_DOCUMENT_DELETE_ILLEGAL_JSON, "Illegal JSON document payload received" );
				return;
			}
		} else {
			report_error( ERROR_DOCUMENT_DELETE_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
			return;
		}
	}

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( del_doc.docid.compare( "" ) == 0 ) {
			service.deleteStorageTransactionInterface( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = del_doc.docid;
	}

	strus::Index docno = storage->documentNumber( docid );
	if( docno == 0 ) {
		report_error( ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DOCUMENT, "Document to delete doesn't exist" );
		return;
	}

	strus::StorageTransactionInterface *transaction = service.getStorageTransactionInterface( name );
	if( !transaction ) {
		report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_TRANSACTION, service.getLastStrusError( ) );
		return;
	}

	// TODO: error handling, for now we catch at least the document doesn't exist
	// case above..
	transaction->deleteDocument( docid );

	transaction->commit( );

	service.deleteStorageTransactionInterface( name );
	
 	report_ok( ); 	
}

void document::get_url_cmd( const std::string name, const std::string id )
{
	get_cmd( name, id, true );
}

void document::get_payload_cmd( const std::string name )
{
	get_cmd( name, "", false );
}

void document::get_cmd( const std::string name, const std::string id, bool docid_in_url )
{
	if( !docid_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}
	
	struct DocumentGetRequest get_doc;

	if( !docid_in_url ) {
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "doc" ) == cppcms::json::is_object ) {
			try {
				get_doc = p.get<struct DocumentGetRequest>( "doc" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Illegal JSON document payload received" );
				return;
			}
		} else {
			report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
			return;
		}
	}

	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_DOCUMENT_GET_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( get_doc.docid.compare( "" ) == 0 ) {
			service.deleteStorageTransactionInterface( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = get_doc.docid;
	}

	DocumentGetAnswer answer;
	answer.docno = storage->documentNumber( docid );
	
	if( answer.docno == 0 ) {
		report_error( ERROR_DOCUMENT_GET_CMD_NO_SUCH_DOCUMENT, "Document doesn't exist" );
		return;
	}
	
	// attributes
	
	strus::AttributeReaderInterface *attributeReader = service.getAttributeReaderInterface( name );
	if( !attributeReader ) {
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_ATTRIBUTE_READER, service.getLastStrusError( ) );
		return;
	}

	// first fetch all possible attribute names which have been attached
	// to all documents
	std::vector<std::string> attrNames = attributeReader->getAttributeNames( );
	std::sort( attrNames.begin( ), attrNames.end( ) );

	// skip to document and iterate over all attribute names and add them if we find
	// them attached to that document
	attributeReader->skipDoc( answer.docno );
	for( std::vector<std::string>::const_iterator it = attrNames.begin( ); it != attrNames.end( ); it++ ) {
		strus::Index h = attributeReader->elementHandle( (*it).c_str( ) );
		std::string value = attributeReader->getValue( h );
		if( value.size( ) > 0 ) {
			answer.attributes.push_back( std::make_pair( *it, value ) );
		}
	}

	// metadata
	
	strus::MetaDataReaderInterface *metadata = service.getMetaDataReaderInterface( name );
	if( !metadata ) {
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_METADATA_READER, service.getLastStrusError( ) );
		return;
	}

	metadata->skipDoc( answer.docno );
	for( strus::Index idx = 0; idx != metadata->nofElements( ); idx++ ) {
		std::string name( metadata->getName( idx ) );
		// TODO: should we report the type too? We have it in index/config already
		// std::string type( metadata->getType( idx ) );
		strus::Index h = metadata->elementHandle( name );
		strus::ArithmeticVariant value = metadata->getValue( h );
		answer.metadata.push_back( std::make_pair( name, value ) );
	}

// TODO: where to know 'word', 'stem' etc. from!?
//	strus::ForwardIteratorInterface *forward = storage->createForwardIterator( "" );
	
//	delete forward;

	cppcms::json::value j;
	j["doc"] = answer;
	
	report_ok( j );	
}

void document::exists_url_cmd( const std::string name, const std::string id )
{
	exists_cmd( name, id, true );
}

void document::exists_payload_cmd( const std::string name )
{
	exists_cmd( name, "", false );
}

void document::exists_cmd( const std::string name, const std::string id, bool docid_in_url )
{
	if( !docid_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}

	struct DocumentGetRequest get_doc;

	if( !docid_in_url ) {
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "doc" ) == cppcms::json::is_object ) {
			try {
				get_doc = p.get<struct DocumentGetRequest>( "doc" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Illegal JSON document payload received" );
				return;
			}
		} else {
			report_error( ERROR_DOCUMENT_GET_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
			return;
		}
	}
	
	if( !get_strus_environment( name ) ) {
		return;
	}

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

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( get_doc.docid.compare( "" ) == 0 ) {
			service.deleteStorageTransactionInterface( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = get_doc.docid;
	}
	
	// translate docid to internal docno
	strus::Index docno = storage->documentNumber( docid );
	bool exists = ( docno > 0 );
			
	cppcms::json::value j;
	j["exists"] = exists;
	
	report_ok( j );
}

} // namespace apps

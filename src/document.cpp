/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <booster/log.h>
#include <booster/locale/format.h>

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/documentTermIteratorInterface.hpp"
#include "strus/constants.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>

#include <boost/algorithm/string/predicate.hpp>

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
	Timer timer;

	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	log_request( );

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

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( ins_doc.docid.compare( "" ) == 0 ) {
			report_error( ERROR_DOCUMENT_INSERT_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = ins_doc.docid;
	}

	// transaction id is optional and additional payload along to 'doc'
	std::string trans_id = "";
	try {
		trans_id = p.get<std::string>( "transaction.id", "" );
	} catch( cppcms::json::bad_value_cast &e ) {
		// be tolerant, transactions are optional
	}
	
	{
		cppcms::json::value j;
		j["document"] = ins_doc;
		std::ostringstream ss;
		if( protocol_pretty_printing ) {
			j.save( ss, cppcms::json::readable );
		} else {
			j.save( ss, cppcms::json::compact );
		}
		BOOSTER_DEBUG( PACKAGE ) << "insert(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << "): " << ss.str( );
	}

	service.lockIndex( name, false );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_INSERT_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageTransactionInterface *transaction;
	if( trans_id == "" ) {
		transaction = service.createStorageTransactionInterface( name );
		if( !transaction ) {
			service.unlockIndex( name );
			report_error( ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_TRANSACTION, service.getLastStrusError( ) );
			return;
		}
	} else {
		transaction = service.getStorageTransactionInterface( name, trans_id );
		if( !transaction ) {
			service.unlockIndex( name );
			report_error( ERROR_DOCUMENT_INSERT_CMD_GET_STORAGE_TRANSACTION, "Referencing illegal transaction, no begin transaction seen before" );
			return;
		}
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
	for( std::vector<std::pair<std::string, strus::NumericVariant> >::const_iterator it = ins_doc.metadata.begin( );
		it != ins_doc.metadata.end( ); it++ ) {
		doc->setMetaData( (*it).first, (*it).second );
	}
		
	// forward index
	unsigned int maxPos = 0;
	for( std::vector<boost::tuple<std::string, std::string, strus::Index> >::const_iterator it = ins_doc.forward.begin( );
		it != ins_doc.forward.end( ); it++ ) {
		unsigned int pos = boost::get<2>( *it );
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
		unsigned int pos = boost::get<2>( *it );
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
		service.unlockIndex( name );
		// we cannot insert a document this way, so discard it
		delete doc;
		// TODO: warning or error?
		// TODO: ignore error in transaction or roll it back?
		// see issue #55
		report_error( ERROR_DOCUMENT_INSERT_TOO_BIG_POSITION, ss.str( ) );
		return;
	}
	
	doc->done( );

	// autocommit if not part of a transaction
	if( trans_id == "" ) {
		transaction->commit( );
		delete transaction;
	}

	service.unlockIndex( name );

	delete doc;

	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "insert(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}	
	BOOSTER_DEBUG( PACKAGE ) << "insert(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << "): " << ss.str( );
	
	report_ok( j );
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
	Timer timer;

	if( !ensure_post( ) ) return;	
	if( !docid_in_url ) {
		if( !ensure_json_request( ) ) return;
	}

	log_request( );

	struct DocumentDeleteRequest del_doc;
	
	std::string trans_id = "";
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

		// transaction id is optional and additional payload along to 'doc'
		try {
			trans_id = p.get<std::string>( "transaction.id", "" );
		} catch( cppcms::json::bad_value_cast &e ) {
			// be tolerant, transactions are optional
		}
	}

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( del_doc.docid.compare( "" ) == 0 ) {
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = del_doc.docid;
	}

	{
		cppcms::json::value j;
		j["document"] = del_doc;
		std::ostringstream ss;
		if( protocol_pretty_printing ) {
			j.save( ss, cppcms::json::readable );
		} else {
			j.save( ss, cppcms::json::compact );
		}
		BOOSTER_DEBUG( PACKAGE ) << "delete(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << "): " << ss.str( );
	}

	service.lockIndex( name, false );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::Index docno = storage->documentNumber( docid );
	if( docno == 0 ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DOCUMENT, "Document to delete doesn't exist" );
		return;
	}

	strus::StorageTransactionInterface *transaction;
	if( trans_id == "" ) {
		transaction = service.createStorageTransactionInterface( name );
		if( !transaction ) {
			service.unlockIndex( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_TRANSACTION, service.getLastStrusError( ) );
			return;
		}
	} else {
		transaction = service.getStorageTransactionInterface( name, trans_id );
		if( !transaction ) {
			service.unlockIndex( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_TRANSACTION, "Referencing illegal transaction, no begin transaction seen before" );
			return;
		}
	}

	transaction->deleteDocument( docid );

	// autocommit if not part of a transaction
	if( trans_id == "" ) {
		transaction->commit( );
		delete transaction;
	}

	service.unlockIndex( name );
	
	cppcms::json::value j;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "delete(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "delete(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << "): " << ss.str( );

	report_ok( j );
}

void document::get_url_cmd( const std::string name, const std::string id )
{
	get_cmd( name, id, true );
}

void document::get_payload_cmd( const std::string name )
{
	get_cmd( name, "", false );
}

typedef boost::tuple<std::string, std::string, strus::Index> FeatureType;

struct FeatureLessThan
{
	bool operator( )( FeatureType const &f1, FeatureType const &f2 )
	{
		if( boost::get<2>( f1 ) < boost::get<2>( f2 ) ) return true;
		return false;
	}
};

void document::get_cmd( const std::string name, const std::string id, bool docid_in_url )
{
	Timer timer;

	if( !docid_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}

	log_request( );
	
	struct DocumentGetRequest get_doc;

	std::string trans_id = "";
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

		// transaction id is optional and additional payload along to 'doc'
		try {
			trans_id = p.get<std::string>( "transaction.id", "" );
		} catch( cppcms::json::bad_value_cast &e ) {
			// be tolerant, transactions are optional
		}
	}

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( get_doc.docid.compare( "" ) == 0 ) {
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = get_doc.docid;
	}

	{
		cppcms::json::value j;
		j["document"] = get_doc;
		std::ostringstream ss;
		if( protocol_pretty_printing ) {
			j.save( ss, cppcms::json::readable );
		} else {
			j.save( ss, cppcms::json::compact );
		}
		BOOSTER_DEBUG( PACKAGE ) << "get(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << name << "): " << ss.str( );
	}

	service.lockIndex( name, false );

	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_GET_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_GET_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	DocumentGetAnswer answer;
	answer.docno = storage->documentNumber( docid );
	
	if( answer.docno == 0 ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_GET_CMD_NO_SUCH_DOCUMENT, "Document doesn't exist" );
		return;
	}
	
	// attributes
	strus::AttributeReaderInterface *attributeReader = service.getAttributeReaderInterface( name );
	if( !attributeReader ) {
		service.unlockIndex( name );
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
		strus::NumericVariant value = metadata->getValue( h );
		answer.metadata.push_back( std::make_pair( name, value ) );
	}

	// get all possible feature types
	std::vector<std::string> termTypes;
	strus::ValueIteratorInterface *valItr = storage->createTermTypeIterator( );
	std::vector<std::string> arr = valItr->fetchValues( FEATURE_ITERATOR_BATCH_SIZE );
	while( arr.size( ) > 0 ) {
		termTypes.insert( termTypes.end( ), arr.begin( ), arr.end( ) );
		arr = valItr->fetchValues( FEATURE_ITERATOR_BATCH_SIZE );
	}
	delete valItr;

	// iterate forward index for every feature type
	for( std::vector<std::string>::const_iterator tit = termTypes.begin( ); tit != termTypes.end( ); tit++ ) {
		strus::ForwardIteratorInterface *forward = storage->createForwardIterator( *tit );
		forward->skipDoc( answer.docno );
		strus::Index pos = 0;
		while( ( pos = forward->skipPos( pos + 1 ) ) != 0 ) {
			std::string value = forward->fetch( );
			FeatureType feature;
			feature = boost::make_tuple( *tit, value, pos );	
			answer.forward.push_back( feature );
		}
		delete forward;
	}
	std::sort( answer.forward.begin( ), answer.forward.end( ), FeatureLessThan( ) );

	// iterate search index for document, this is quiet inefficient
	// by design
	for( std::vector<std::string>::const_iterator tit = termTypes.begin( ); tit != termTypes.end( ); tit++ ) {
		strus::DocumentTermIteratorInterface *fit = storage->createDocumentTermIterator( *tit );
		if( !fit->skipDoc( answer.docno ) ) {
			delete fit;
			continue;
		}
		strus::DocumentTermIteratorInterface::Term term;
		while( fit->nextTerm( term ) ) {
			std::string value = fit->termValue( term.termno );
			strus::PostingIteratorInterface *pit = storage->createTermPostingIterator( *tit, value );
			if( pit->skipDoc( answer.docno ) != answer.docno ) {
				delete fit;
				delete pit;
				continue;
			}
			strus::Index pos = 0;
			while( ( pos = pit->skipPos( pos + 1 ) ) != 0 ) {
				FeatureType feature;
				feature = boost::make_tuple( *tit, value, pos );	
				answer.search.push_back( feature );
			}
			delete pit;
		}
		delete fit;
	}
	std::sort( answer.search.begin( ), answer.search.end( ), FeatureLessThan( ) );

	service.unlockIndex( name );

	// TODO: how to use transactions here?
	
	cppcms::json::value j;
	j["doc"] = answer;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "get(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "get(" << name << ", " << ( ( trans_id == "" ) ? "<implicit>" : trans_id ) << ", " << docid << "): " << ss.str( );
	
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
	Timer timer;

	if( !docid_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}

	log_request( );

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

	service.lockIndex( name, false );
	
	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_DOCUMENT_EXISTS_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		service.unlockIndex( name );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_DOCUMENT_EXISTS_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	// docid can come from the JSON payload or directly in the URL
	std::string docid;
	if( docid_in_url ) {
		docid = id;
	} else {
		if( get_doc.docid.compare( "" ) == 0 ) {
			service.unlockIndex( name );
			report_error( ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED, "docid must be part of the JSON payload as field of 'doc'" );
			return;
		}
		docid = get_doc.docid;
	}
	
	// translate docid to internal docno
	strus::Index docno = storage->documentNumber( docid );
	bool exists = ( docno > 0 );

	service.unlockIndex( name );
			
	cppcms::json::value j;
	j["exists"] = exists;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "exists(" << name << ", <implicit>" << ", " << docid << ", " << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "exists(" << name << ", <implicit>" << ", " << docid << "): " << ss.str( );
	
	report_ok( j );
}

} // namespace apps

#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentInterface.hpp"

namespace apps {

document::document( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/document/insert/(\\w+)", &document::insert_without_id_cmd, this, 1 );
	service.dispatcher( ).assign( "/document/insert/(\\w+)/(\\w+)", &document::insert_cmd, this, 1, 2  );
	service.dispatcher( ).assign( "/document/update/(\\w+)/(\\w+)", &document::update_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)/(\\w+)", &document::delete_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/get/(\\w+)/(\\w+)", &document::get_cmd, this, 1, 2 );
}

void document::insert_without_id_cmd( const std::string name )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

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
	
	cppcms::json::value j;
	// TODO: report back generated internal id
	
	report_ok( j );	
}

void document::insert_cmd( const std::string name, const std::string id  )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	get_strus_environment( name );

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

} // namespace apps

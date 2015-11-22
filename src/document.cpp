#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  

#include "strus/storageTransactionInterface.hpp"
#include "strus/storageClientInterface.hpp"

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

	report_ok( );
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

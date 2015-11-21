#include "document.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  

namespace apps {

document::document( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/document/insert", &document::insert_without_id_cmd, this );
	service.dispatcher( ).assign( "/document/insert/(\\w+)", &document::insert_cmd, this, 1  );
	service.dispatcher( ).assign( "/document/update/(\\w+)", &document::update_cmd, this, 1 );
	service.dispatcher( ).assign( "/document/delete/(\\w+)/(\\w+)", &document::delete_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/document/get/(\\w+)", &document::get_cmd, this, 1 );
}

void document::insert_without_id_cmd( )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	prepare_strus_environment( );

	close_strus_environment( );
			
	report_ok( );
}

void document::insert_cmd( const std::string id  )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	prepare_strus_environment( );

	close_strus_environment( );
			
	report_ok( );
}

void document::update_cmd( const std::string id )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	prepare_strus_environment( );

	close_strus_environment( );
			
	report_ok( );
}

void document::delete_cmd( const std::string name, const std::string id )
{
	if( !ensure_post( ) ) return;	

	prepare_strus_environment( );
	
	close_strus_environment( );
			
	report_ok( );	
}

void document::get_cmd( const std::string id )
{
	prepare_strus_environment( );

	close_strus_environment( );
			
	report_ok( );
}

} // namespace apps

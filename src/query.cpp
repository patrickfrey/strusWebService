#include "query.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  

namespace apps {

query::query( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/query/(\\w++)", &query::query_cmd, this, 1 );
}

void query::query_cmd( const std::string name )
{
	if( !ensure_post( ) ) return;	
	if( !ensure_json_request( ) ) return;

	prepare_strus_environment( name );

	close_strus_environment( );
			
	report_ok( );
}

} // namespace apps

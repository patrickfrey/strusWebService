#include "strusWebService.hpp"
#include "ping.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>

namespace apps {

ping::ping( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/ping", &ping::ping_cmd, this );
}

void ping::ping_cmd( )
{
	response( ).content_type( "application/json" );
	response( ).out( ) << "PONG";
}

} // namespace apps

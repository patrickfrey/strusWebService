#include "strusWebService.hpp"
#include "ping.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

namespace apps {

ping::ping( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/ping", &ping::ping_cmd, this );
}

void ping::ping_cmd( )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	j["ping"] = "pong";
	response( ).out( ) << j << std::endl;
}

} // namespace apps

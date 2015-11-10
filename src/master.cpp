#include "strusWebService.hpp"
#include "master.hpp"

#include <cppcms/service.h>
#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service )
{
}

void master::register_common_pages( )
{
	// catch all for missing handlers
	service.dispatcher( ).assign( ".*", &master::not_found_404, this );
}

void master::not_found_404( )
{
	response( ).content_type( "application/json" );
	int err_code = 404;
	cppcms::json::value j;  
	j["err_code"] = 404;
	j["err_msg"] = "No such API found";
	response( ).out( ) << j;
}

} // namespace apps

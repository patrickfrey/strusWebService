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

void master::report_error( unsigned int code, const std::string &msg )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["err_code"] = code;
	j["err_msg"] = msg;
	response( ).out( ) << j << std::endl;
}

void master::report_ok( )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	response( ).out( ) << j << std::endl;
}

void master::not_found_404( )
{
	report_error( 404, "No API found on this URL." );
}

} // namespace apps

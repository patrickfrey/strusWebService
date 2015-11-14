#include "strusWebService.hpp"
#include "other.hpp"
#include "version.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

// TODO: this should be exported IMHO
//#include "strus/versionStorage.hpp"
#define STRUS_VERSION_STRING "0.2.0"

namespace apps {

other::other( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/ping", &other::ping_cmd, this );
	service.dispatcher( ).assign( "/version", &other::version_cmd, this );
}

void other::ping_cmd( )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	j["ping"] = "pong";
	response( ).out( ) << j << std::endl;
}

void other::version_cmd( )
{
	response( ).content_type( "application/json" );
	cppcms::json::value j;  
	j["result"] = "ok";
	j["version"]["strus"] = STRUS_VERSION_STRING;
	j["version"]["webservice"] = STRUS_WEB_SERVICE_VERSION_STRING;
	response( ).out( ) << j << std::endl;
}

} // namespace apps

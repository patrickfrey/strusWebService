#include "strusWebService.hpp"
#include "other.hpp"
#include "version.hpp"
#include "constants.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

namespace apps {

other::other( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/ping", &other::ping_cmd, this );
	service.dispatcher( ).assign( "/version", &other::version_cmd, this );
}

void other::ping_cmd( )
{
	cppcms::json::value j;
	j["ping"] = "pong";
	
	report_ok( j );
}

void other::version_cmd( )
{
	cppcms::json::value j;
	j["version"]["strus"] = STRUS_VERSION_STRING;
	j["version"]["webservice"] = STRUS_WEB_SERVICE_VERSION_STRING;
	
	report_ok( j );
}

} // namespace apps

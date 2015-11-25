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
	report_error( ERROR_NOT_IMPLEMENTED, "query currently not implemented" );
}

} // namespace apps

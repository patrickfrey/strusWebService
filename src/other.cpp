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
	service.dispatcher( ).assign( "/config", &other::config_cmd, this );	
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

void other::config_cmd( )
{
	cppcms::json::value j;

	strus::QueryProcessorInterface *query_processor = service.getQueryProcessorInterface( );
	if( !query_processor ) {
		report_error( ERROR_OTHER_CMD_CREATE_QUERY_PROCESSOR, service.getLastStrusError( ) );
		return;
	}
	
	ServiceConfiguration config;
	
	config.weighting_functions = query_processor->getFunctionList( strus::QueryProcessorInterface::WeightingFunction );
	config.summarizer_functions = query_processor->getFunctionList( strus::QueryProcessorInterface::SummarizerFunction );
	config.posting_join_operators = query_processor->getFunctionList( strus::QueryProcessorInterface::PostingJoinOperator );
	
	service.deleteQueryProcessorInterface( );

	j["config"] = config;
	report_ok( j );
}

} // namespace apps

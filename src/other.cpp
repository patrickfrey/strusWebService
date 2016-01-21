/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

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

	std::vector<std::string> weighting_functions = query_processor->getFunctionList( strus::QueryProcessorInterface::WeightingFunction );
	for( std::vector<std::string>::const_iterator it = weighting_functions.begin( ); it != weighting_functions.end( ); it++ ) {
		const strus::WeightingFunctionInterface *func = query_processor->getWeightingFunction( *it );
		WeightingFunctionConfiguration weighting_config;
		strus::WeightingFunctionInterface::Description description= func->getDescription( );
		weighting_config.name = *it;
		weighting_config.description = description.text( );
		std::vector<strus::WeightingFunctionInterface::Description::Param> p = description.param( );
		for( std::vector<strus::WeightingFunctionInterface::Description::Param>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			FunctionParameter param( *pit );
			weighting_config.parameter.push_back( param );
		}
		config.weighting_functions.push_back( weighting_config );
	}
		
	std::vector<std::string> summarizers = query_processor->getFunctionList( strus::QueryProcessorInterface::SummarizerFunction );
	for( std::vector<std::string>::const_iterator it = summarizers.begin( ); it != summarizers.end( ); it++ ) {
		const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( *it );
		SummarizerFunctionConfiguration sum_config;
		strus::SummarizerFunctionInterface::Description description = sum->getDescription( );
		sum_config.name = *it;
		sum_config.description = description.text( );
		std::vector<strus::SummarizerFunctionInterface::Description::Param> p = description.param( );
		for( std::vector<strus::SummarizerFunctionInterface::Description::Param>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			FunctionParameter param( *pit );
			sum_config.parameter.push_back( param );
		}
		config.summarizer_functions.push_back( sum_config );
	}

	std::vector<std::string> operators = query_processor->getFunctionList( strus::QueryProcessorInterface::PostingJoinOperator );
	for( std::vector<std::string>::const_iterator it = operators.begin( ); it != operators.end( ); it++ ) {
		const strus::PostingJoinOperatorInterface *join_operator = query_processor->getPostingJoinOperator( *it );
		PostingJoinOperatorConfiguration op_config;
		strus::PostingJoinOperatorInterface::Description description = join_operator->getDescription( );
		op_config.name = *it;
		op_config.description = description.text( );
		config.posting_join_operators.push_back( op_config );
	}
	
	service.deleteQueryProcessorInterface( );

	j["config"] = config;
	report_ok( j );
}

} // namespace apps

/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "strusWebService.hpp"
#include "other.hpp"
#include "version.hpp"
#include "constants.hpp"
#include "strus/versionStorage.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_response.h>
#include <cppcms/json.h>

#include <boost/timer/timer.hpp>

namespace apps {

other::other( strusWebService &service )
	: master( service ), allow_quit_command( false )
{
	service.dispatcher( ).assign( "/ping", &other::ping_cmd, this );
	service.dispatcher( ).assign( "/version", &other::version_cmd, this );
	service.dispatcher( ).assign( "/config", &other::config_cmd, this );	
	service.dispatcher( ).assign( "/quit", &other::quit_cmd, this );
}

void other::set_allow_quit_command( bool _allow_quit_command )
{
	allow_quit_command = _allow_quit_command;
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
	j["version"]["strus"] = STRUS_STORAGE_VERSION_STRING;
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
		strus::FunctionDescription description = func->getDescription( );
		weighting_config.name = *it;
		weighting_config.description = description.text( );
		std::vector<strus::FunctionDescription::Parameter> p = description.parameter( );
		for( std::vector<strus::FunctionDescription::Parameter>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			FunctionParameter param( *pit );
			weighting_config.parameter.push_back( param );
		}
		config.weighting_functions.push_back( weighting_config );
	}
		
	std::vector<std::string> summarizers = query_processor->getFunctionList( strus::QueryProcessorInterface::SummarizerFunction );
	for( std::vector<std::string>::const_iterator it = summarizers.begin( ); it != summarizers.end( ); it++ ) {
		const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( *it );
		SummarizerFunctionConfiguration sum_config;
		strus::FunctionDescription description = sum->getDescription( );
		sum_config.name = *it;
		sum_config.description = description.text( );
		std::vector<strus::FunctionDescription::Parameter> p = description.parameter( );
		for( std::vector<strus::FunctionDescription::Parameter>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
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

void other::quit_cmd( )
{
	cppcms::json::value j;

	if( !allow_quit_command ) {
		not_found_404( );
		return;
	}
	
	report_ok( j );
	
	service.raiseTerminationFlag( );
}

} // namespace apps

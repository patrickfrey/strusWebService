/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "query.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include <booster/log.h>

#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/queryResult.hpp"
#include "strus/metaDataRestrictionInterface.hpp"

#include <vector>
#include <string>
#include <set>
#include <iomanip>

namespace apps {

query::query( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/query/(\\w++)/(.+)", &query::query_url_cmd, this, 1, 2 );
	service.dispatcher( ).assign( "/query/(\\w++)", &query::query_payload_cmd, this, 1 );
}

void query::query_url_cmd( const std::string name, const std::string qry )
{
	query_cmd( name, qry, true );
}

void query::query_payload_cmd( const std::string name )
{
	query_cmd( name, "", false );
}
	
void query::query_cmd( const std::string name, const std::string qry, bool query_in_url )
{
	Timer timer;
	
	if( !handle_preflight_cors( ) ) return;
	if( !query_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}

	log_request( );
	
	struct QueryRequest qry_req; 

	if( !query_in_url ) {
		std::pair<void *, size_t> data = request( ).raw_post_data( );
		std::istringstream is( std::string( reinterpret_cast<char const *>( data.first ), data.second ) );
		cppcms::json::value p;
		if( !p.load( is, true) ) {
			report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Illegal JSON received" );
			return;
		}
		
		if( p.type( "query" ) == cppcms::json::is_object ) {
			try {
				qry_req = p.get<struct QueryRequest>( "query" );
			} catch( cppcms::json::bad_value_cast &e ) {
				report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Illegal JSON document payload received" );
				return;
			}
		} else {
			report_error( ERROR_QUERY_CMD_GET_ILLEGAL_JSON, "Expecting a JSON object as JSON document payload" );
			return;
		}
	} else {
		QueryRequest default_qry_req( qry );
		qry_req = default_qry_req;
	}

	{
		cppcms::json::value j;
		j["query"] = qry_req;
		std::ostringstream ss;
		if( protocol_pretty_printing ) {
			j.save( ss, cppcms::json::readable );
		} else {
			j.save( ss, cppcms::json::compact );
		}
		BOOSTER_DEBUG( PACKAGE ) << "query_request(" << name << "): " << ss.str( );
	}

	service.lockIndex( name, true );
		
	if( !get_strus_environment( name ) ) {
		service.unlockIndex( name );
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		service.unlockIndex( name );
		report_error( ERROR_QUERY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		service.unlockIndex( name );
		report_error( ERROR_QUERY_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::QueryEvalInterface *query_eval = service.getQueryEvalInterface( );
	if( !query_eval ) {
		service.unlockIndex( name );
		report_error( ERROR_QUERY_CMD_CREATE_QUERY_EVAL_INTERFACE, service.getLastStrusError( ) );
		return;
	}

	strus::QueryProcessorInterface *query_processor = service.getQueryProcessorInterface( );
	if( !query_processor ) {
		service.unlockIndex( name );
		report_error( ERROR_QUERY_CMD_CREATE_QUERY_PROCESSOR, service.getLastStrusError( ) );
		service.deleteQueryEvalInterface( );
		return;
	}

	// 1) define query evaluation scheme, we are RESTful, so we do not store
	//    query evaluation schemes anywhere, we do all in one step!
	
	// 1.1) define weighting scheme
	
	std::vector<strus::QueryEvalInterface::FeatureParameter> weighting_parameters;
	for( std::vector<struct WeightingConfiguration>::const_iterator it = qry_req.weighting.begin( ); it != qry_req.weighting.end( ); it++ ) {
		std::string scheme = it->name;
		const strus::WeightingFunctionInterface *wfi = query_processor->getWeightingFunction( scheme );
		if( !wfi ) {
			report_error( ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			service.unlockIndex( name );
			return;
		}
		
		strus::FunctionDescription description= wfi->getDescription( );
		std::vector<strus::FunctionDescription::Parameter> p = description.parameter( );
		std::set<std::string> knownFeatureParams;
		for( std::vector<strus::FunctionDescription::Parameter>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			if( pit->type( ) == strus::FunctionDescription::Parameter::Feature ) {
				knownFeatureParams.insert( pit->name( ) );
			}	
		}
		
		strus::WeightingFunctionInstanceInterface *function = wfi->createInstance( query_processor );
		for( std::vector<std::pair<std::string, struct ParameterValue> >::const_iterator pit = it->params.begin( ); pit != it->params.end( ); pit++ ) {
			switch( pit->second.type ) {
				case PARAMETER_TYPE_STRING:
					if( knownFeatureParams.find( pit->first ) != knownFeatureParams.end( ) ||
						qry_req.isFeatureInQuery( pit->second.s ) ) {
						weighting_parameters.push_back( strus::QueryEvalInterface::FeatureParameter( pit->first, pit->second.s ) );
					} else {
						function->addStringParameter( pit->first, pit->second.s );
					}
					break;
				case PARAMETER_TYPE_NUMERIC:
					function->addNumericParameter( pit->first, pit->second.n );
					break;
				case PARAMETER_TYPE_UNKNOWN:
				default:
					report_error( ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION_PARAMETER, "Unknown type of weighting function parameter, internal error, check query object parsing!" );
					service.deleteQueryEvalInterface( );
					service.deleteQueryProcessorInterface( );
					service.unlockIndex( name );
					return;
			}
		}
		query_eval->addWeightingFunction( scheme, function, weighting_parameters );
	}

	// 1.2) define summarizer configuration
	
	for( std::vector<struct SummarizerConfiguration>::const_iterator it = qry_req.summarizer.begin( ); it != qry_req.summarizer.end( ); it++ ) {
		std::string name = it->name;
		const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( name );
		if( !sum ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			service.unlockIndex( name );
			return;
		}

		strus::FunctionDescription description= sum->getDescription( );
		std::vector<strus::FunctionDescription::Parameter> p = description.parameter( );
		std::set<std::string> knownFeatureParams;
		for( std::vector<strus::FunctionDescription::Parameter>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			if( pit->type( ) == strus::FunctionDescription::Parameter::Feature ) {
				knownFeatureParams.insert( pit->name( ) );
			}	
		}
		
		strus::SummarizerFunctionInstanceInterface *summarizer = sum->createInstance( query_processor );
		if( !summarizer ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			service.unlockIndex( name );
			return;
		}

		std::vector<strus::QueryEvalInterface::FeatureParameter> summarizer_parameters;
		for( std::vector<std::pair<std::string, struct ParameterValue> >::const_iterator pit = it->params.begin( ); pit != it->params.end( ); pit++ ) {
			switch( pit->second.type ) {
				case PARAMETER_TYPE_STRING:
					if( knownFeatureParams.find( pit->first ) != knownFeatureParams.end( ) || qry_req.isFeatureInQuery( pit->second.s ) ) {
						summarizer_parameters.push_back( strus::QueryEvalInterface::FeatureParameter( pit->first, pit->second.s ) );
					} else {
						summarizer->addStringParameter( pit->first, pit->second.s );
					}
					break;
				case PARAMETER_TYPE_NUMERIC:
					summarizer->addNumericParameter( pit->first, pit->second.n );
					break;
				case PARAMETER_TYPE_UNKNOWN:
				default:
					report_error( ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION_PARAMETER, "Unknown type of weighting function parameter, internal error, check query object parsing!" );
					service.deleteQueryEvalInterface( );
					service.deleteQueryProcessorInterface( );
					service.unlockIndex( name );
					return;
			}			
		}
		
		for( std::vector<std::pair<std::string, std::string> >::const_iterator rit = it->resultnames.begin( ); rit != it->resultnames.end( ); rit++ ) {
			summarizer->defineResultName( rit->second, rit->first );
		}		
		
		query_eval->addSummarizerFunction( name, summarizer, summarizer_parameters );
	}

	// 1.3) construct the various feature sets
	
	for( std::vector<std::string>::const_iterator it = qry_req.select.begin( ); it != qry_req.select.end( ); it++ ) {
		query_eval->addSelectionFeature( *it );
	}
	
	for( std::vector<std::string>::const_iterator it = qry_req.restrict.begin( ); it != qry_req.restrict.end( ); it++ ) {
		query_eval->addRestrictionFeature( *it );
	}
	
	for( std::vector<std::string>::const_iterator it = qry_req.exclude.begin( ); it != qry_req.exclude.end( ); it++ ) {
		query_eval->addExclusionFeature( *it );
	}
	
	// 2) specific query
		
	strus::QueryInterface *query = query_eval->createQuery( storage );
	if( !query ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY, service.getLastStrusError( ) );
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		service.unlockIndex( name );
		return;
	}
	
	// 2.1) query arguments
	
	query->setMinRank( qry_req.first_rank );
	query->setMaxNofRanks( qry_req.nof_ranks );

	// 2.2) produce forest of feature trees
	for( std::vector<Feature *>::const_iterator it = qry_req.features.begin( ); it != qry_req.features.end( ); it++ ) {
		(*it)->produceQuery( query_processor, query );
	}
	
	// 2.3) append metadata filtering conditions
	for( std::vector<MetadataRestriction>::const_iterator rit = qry_req.metadata.begin( ); rit != qry_req.metadata.end( ); rit++ ) {
		for( std::vector<MetadataCondition>::const_iterator cit = rit->conditions.begin( ); cit != rit->conditions.end( ); cit++ ) {
			bool newGroup = cit == rit->conditions.begin( );
			query->addMetaDataRestrictionCondition(
				cit->operatorEnum( ), cit->name, cit->numberValue( ), newGroup );
		}
	}
		
	// 3.1) execute query

	strus::QueryResult result = query->evaluate( );
	if( service.hasError( ) ) {
		report_error( ERROR_QUERY_CMD_QUERY_EVALUATE, service.getLastStrusError( ) );
		delete query;
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		service.unlockIndex( name );
		return;
	}

	// 3.2) get result and fill attributes per rank
	// TODO: fill in also per ranklist data (currently we have none)
	
	QueryResponse response;

	response.passes_evaluated = result.evaluationPass( );
	response.documents_ranked = result.nofRanked( );
	response.documents_visited = result.nofVisited( );
	
	for( std::vector<strus::ResultDocument>::const_iterator it = result.ranks( ).begin( ); it != result.ranks( ).end( ); it++ ) {
			Rank rank;
			
			rank.docno = (*it).docno( );
			rank.weight = (*it).weight( );
			
			for( std::vector<strus::SummaryElement>::const_iterator ait = (*it).summaryElements( ).begin( ); ait != (*it).summaryElements( ).end( ); ait++ ) {
				rank.attributes.push_back( std::make_pair( ait->name( ), ait->value( ) ) );
			}

			response.ranks.push_back( rank );
	}
	
	delete query;
	service.deleteQueryEvalInterface( );
	service.deleteQueryProcessorInterface( );

	service.unlockIndex( name );

	cppcms::json::value j;
	j["ranklist"] = response;
	double execution_time = timer.elapsed( );
	j["execution_time"] = execution_time;

	BOOSTER_INFO( PACKAGE ) << "query(" << std::fixed << std::setprecision( 6 ) << execution_time << "s)";
	std::ostringstream ss;
	if( protocol_pretty_printing ) {
		j.save( ss, cppcms::json::readable );
	} else {
		j.save( ss, cppcms::json::compact );
	}
	BOOSTER_DEBUG( PACKAGE ) << "query_response(" << name << "):" << ss.str( );
	
	report_ok( j );
}

} // namespace apps

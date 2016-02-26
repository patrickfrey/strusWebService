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

#include <vector>
#include <string>
#include <set>

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
	if( !query_in_url ) {
		if( !ensure_post( ) ) return;	
		if( !ensure_json_request( ) ) return;
	}
	
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
		BOOSTER_DEBUG( PACKAGE ) << "Query: " << j;
	}
	
	//~ my_object.save(std::cout,cppcms::json::readable);  
	
	
	if( !get_strus_environment( name ) ) {
		return;
	}

	if( !dbi->exists( service.getConfigString( name ) ) ) {
		report_error( ERROR_QUERY_CMD_NO_SUCH_DATABASE, "No search index with that name exists" );
		return;
	}

	strus::DatabaseClientInterface *database = service.getDatabaseClientInterface( name );
	if( !database ) {
		report_error( ERROR_QUERY_CMD_CREATE_DATABASE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::StorageClientInterface *storage = service.getStorageClientInterface( name );
	if( !storage ) {
		report_error( ERROR_QUERY_CMD_CREATE_STORAGE_CLIENT, service.getLastStrusError( ) );
		return;
	}

	strus::MetaDataReaderInterface *metadata = service.getMetaDataReaderInterface( name );
	if( !metadata ) {
		report_error( ERROR_INDEX_CONFIG_CMD_CREATE_METADATA_READER, service.getLastStrusError( ) );
		return;
	}

	strus::QueryEvalInterface *query_eval = service.getQueryEvalInterface( );
	if( !query_eval ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY_EVAL_INTERFACE, service.getLastStrusError( ) );
		return;
	}

	strus::QueryProcessorInterface *query_processor = service.getQueryProcessorInterface( );
	if( !query_processor ) {
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
			return;
		}
		
		strus::WeightingFunctionInterface::Description description= wfi->getDescription( );
		std::vector<strus::WeightingFunctionInterface::Description::Param> p = description.param( );
		std::set<std::string> knownFeatureParams;
		for( std::vector<strus::WeightingFunctionInterface::Description::Param>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			if( pit->type( ) == strus::WeightingFunctionInterface::Description::Param::Feature ) {
				knownFeatureParams.insert( pit->name( ) );
			}	
		}
		
		strus::WeightingFunctionInstanceInterface *function = wfi->createInstance( );
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
					return;
			}
		}
		query_eval->addWeightingFunction( scheme, function, weighting_parameters, it->weight );
	}

	// 1.2) define summarizer configuration
	
	for( std::vector<struct SummarizerConfiguration>::const_iterator it = qry_req.summarizer.begin( ); it != qry_req.summarizer.end( ); it++ ) {
		std::string name = it->name;
		const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( name );
		if( !sum ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			return;
		}

		strus::SummarizerFunctionInterface::Description description= sum->getDescription( );
		std::vector<strus::SummarizerFunctionInterface::Description::Param> p = description.param( );
		std::set<std::string> knownFeatureParams;
		for( std::vector<strus::SummarizerFunctionInterface::Description::Param>::const_iterator pit = p.begin( ); pit != p.end( ); pit++ ) {
			if( pit->type( ) == strus::SummarizerFunctionInterface::Description::Param::Feature ) {
				knownFeatureParams.insert( pit->name( ) );
			}	
		}
		
		strus::SummarizerFunctionInstanceInterface *summarizer = sum->createInstance( query_processor );
		if( !summarizer ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
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
					return;
			}			
		}
		query_eval->addSummarizerFunction( name, summarizer, summarizer_parameters );
	}

	// 1.3) construct the various feature sets
	
	for( std::vector<std::string>::const_iterator it = qry_req.select.begin( ); it != qry_req.select.end( ); it++ ) {
		query_eval->addSelectionFeature( *it );
	}

	// TODO:
	// addRestrictionFeature: keep documents which CONTAIN the feature
	// addExclusionFeature: keep docuemnts which DON'T CONTAIN the feature
	
	// 2) specific query
		
	strus::QueryInterface *query = query_eval->createQuery( storage );
	if( !query ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY, service.getLastStrusError( ) );
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		return;
	}
	
	// 2.1) query arguments
	
	query->setMinRank( qry_req.first_rank );
	query->setMaxNofRanks( qry_req.nof_ranks );

	// 2.2) produce forest of feature trees
	for( std::vector<Feature *>::const_iterator it = qry_req.features.begin( ); it != qry_req.features.end( ); it++ ) {
		(*it)->produceQuery( query_processor, query );
	}
	
	// TODO: defineMetaDataRestriction

	// 3.1) execute query

	strus::QueryResult result = query->evaluate( );
	if( service.hasError( ) ) {
		report_error( ERROR_QUERY_CMD_QUERY_EVALUATE, service.getLastStrusError( ) );
		delete query;
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		return;
	}

	// 3.2) get result and fill attributes per rank
	// TODO: fill in also per ranklist data (currently we have none)
	
	QueryResponse response;
	
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

	cppcms::json::value j;
	j["ranklist"] = response;
	
	report_ok( j );
}

} // namespace apps

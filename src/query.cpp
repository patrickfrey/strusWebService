#include "query.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include "strus/queryInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"

#include <vector>
#include <string>

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

	for( std::vector<struct WeightingConfiguration>::const_iterator it = qry_req.weighting.begin( ); it != qry_req.weighting.end( ); it++ ) {
		std::string scheme = it->name;
		const strus::WeightingFunctionInterface *wfi = query_processor->getWeightingFunction( scheme );
		if( !wfi ) {
			report_error( ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			return;
		}
	
		strus::WeightingFunctionInstanceInterface *function = wfi->createInstance( );
		for( std::vector<std::pair<std::string, struct ParameterValue> >::const_iterator pit = it->params.begin( ); pit != it->params.end( ); pit++ ) {
			switch( pit->second.type ) {
				case PARAMETER_TYPE_STRING:
					// TODO: if the name matches a known feature set, do not add it
					// as string parameter, but as feature parameter
					function->addStringParameter( pit->first, pit->second.s );
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

		// TODO: add feature and other weighting parameters which are not mere
		// constants
		// TODO: map parameters which are specific per query, i.e. the name of the feature set
		std::vector<strus::QueryEvalInterface::FeatureParameter> weighting_parameters;
		weighting_parameters.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "feat" ) );
		query_eval->addWeightingFunction( scheme, function, weighting_parameters, it->weight );
	}

	for( std::vector<struct SummarizerConfiguration>::const_iterator it = qry_req.summarizer.begin( ); it != qry_req.summarizer.end( ); it++ ) {
		std::string name = it->name;
		const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( name );
		if( !sum ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			return;
		}

		strus::SummarizerFunctionInstanceInterface *summarizer = sum->createInstance( query_processor );
		if( !summarizer ) {
			report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
			service.deleteQueryEvalInterface( );
			service.deleteQueryProcessorInterface( );
			return;
		}

		for( std::vector<std::pair<std::string, struct ParameterValue> >::const_iterator pit = it->params.begin( ); pit != it->params.end( ); pit++ ) {
			switch( pit->second.type ) {
				case PARAMETER_TYPE_STRING:
					// TODO: if the name matches a known feature set, do not add it
					// as string parameter, but as feature parameter
					summarizer->addStringParameter( pit->first, pit->second.s );
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

		// TODO: add feature and other weighting parameters which are not mere
		// constants
		std::vector<strus::QueryEvalInterface::FeatureParameter> summarizer_parameters;
		query_eval->addSummarizerFunction( name, summarizer, summarizer_parameters, it->attribute );
	}

	query_eval->addSelectionFeature( "sel" );
	
	// fix term for all queries, we might not use it.
	// addTerm (const std::string &set_, const std::string &type_, const std::string &value_)=0
	// addSelectionFeature: super-set of what get's weighted
	// from that set what remains in there can be restricted with
	// addRestrictionFeature: keep documents which CONTAIN the feature
	// addExclusionFeature: keep docuemnts which DON'T CONTAIN the feature
		
	strus::QueryInterface *query = query_eval->createQuery( storage );
	if( !query ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY, service.getLastStrusError( ) );
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		return;
	}
		
	query->setMinRank( qry_req.first_rank );
	query->setMaxNofRanks( qry_req.nof_ranks );
		
	// TODO: again introspect and make the json tree parser tolerant to expression nodes
	const strus::PostingJoinOperatorInterface *op = query_processor->getPostingJoinOperator( "contains" );	

	query->pushTerm( "word", qry_req.text );
	query->defineFeature( "feat", 1.0 );
	query->pushTerm( "word", qry_req.text );
	query->pushExpression( op, 1, 0, 0 );
	query->defineFeature( "sel", 1.0 );

	// TODOS
	//~ defineMetaDataRestriction
	
	std::vector<strus::ResultDocument> ranklist = query->evaluate( );
	if( service.hasError( ) ) {
		report_error( ERROR_QUERY_CMD_QUERY_EVALUATE, service.getLastStrusError( ) );
		delete query;
		service.deleteQueryEvalInterface( );
		service.deleteQueryProcessorInterface( );
		return;
	}

	QueryResponse response;
	
	for( std::vector<strus::ResultDocument>::const_iterator it = ranklist.begin( ); it != ranklist.end( ); it++ ) {
			Rank rank;
			
			rank.docno = (*it).docno( );
			rank.weight = (*it).weight( );
						
			for( std::vector<strus::ResultDocument::Attribute>::const_iterator ait = (*it).attributes( ).begin( ); ait != (*it).attributes( ).end( ); ait++ ) {
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

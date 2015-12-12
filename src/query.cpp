#include "query.hpp"
#include "strusWebService.hpp"

#include <cppcms/url_dispatcher.h>  
#include <cppcms/http_request.h>

#include "strus/queryInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"

namespace apps {

query::query( strusWebService &service )
	: master( service )
{
	service.dispatcher( ).assign( "/query/(\\w++)/(\\w+)", &query::query_url_cmd, this, 1, 2 );
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
		return;
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
		qry_req.text = qry;
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

	strus::QueryEvalInterface *query_eval = service.getQueryEvalInterface( name );
	if( !query_eval ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY_EVAL_INTERFACE, service.getLastStrusError( ) );
		return;
	}

	strus::QueryProcessorInterface *query_processor = service.getQueryProcessorInterface( name );
	if( !query_processor ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY_PROCESSOR, service.getLastStrusError( ) );
		return;
	}
	
	// TODO: list of possible functions again in an introspection function, use
	// getFunctionList (FunctionType for this.
	
	// TODO: get from query request
	std::string scheme = "BM25";
	//~ WeightingConfig weightingConfig;
	//~ weightingConfig.defineParameter( "k1", 0.75);
	//~ weightingConfig.defineParameter( "b", 2.1);
	//~ weightingConfig.defineParameter( "avgdoclen", 8);
	
	const strus::WeightingFunctionInterface *wfi = query_processor->getWeightingFunction( scheme );
	if( !wfi ) {
		report_error( ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION, service.getLastStrusError( ) );
		return;
	}
	
	strus::WeightingFunctionInstanceInterface *function = wfi->createInstance( );

	// TODO: every weighting scheme must be introspectable and the JSON deserializer
	// must be tolerant to those parameters (do it as for the metadata)
	std::vector<strus::QueryEvalInterface::FeatureParameter> weighting_parameters;
	//~ weighting_parameters.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "feat" ) );
	//~ ArithmeticVariant parameterValue = parseNumericValue( src);
	//~ function->addNumericParameter( parameterName, parameterValue);
	//~ function->addStringParameter( parameterName, parameterValue);
	float weight = 1.0;
	
	query_eval->addWeightingFunction( scheme, function, weighting_parameters, weight );

	// TODO: add summarizers
	const strus::SummarizerFunctionInterface *sum = query_processor->getSummarizerFunction( "attribute" );
	if( !sum ) {
		report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
		return;
	}
	strus::SummarizerFunctionInstanceInterface *summarizer = sum->createInstance( query_processor );
	if( !summarizer ) {
		report_error( ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE, service.getLastStrusError( ) );
		return;
	}
	
	std::vector<strus::QueryEvalInterface::FeatureParameter> summarizer_parameters;
	summarizer->addStringParameter( "name", "docid" );
	
	//~ weighting_parameters.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "feat" ) );
	//~ ArithmeticVariant parameterValue = parseNumericValue( src);
	//~ function->addNumericParameter( parameterName, parameterValue);
	//~ function->addStringParameter( parameterName, parameterValue);

	query_eval->addSummarizerFunction( "docid", summarizer, summarizer_parameters, "docid" );
	
	query_eval->addSelectionFeature( "sel" );
	
	strus::QueryInterface *query = query_eval->createQuery( storage );
	if( !query ) {
		report_error( ERROR_QUERY_CMD_CREATE_QUERY, service.getLastStrusError( ) );
		return;
	}
		
	query->setMinRank( qry_req.first_rank );
	query->setMaxNofRanks( qry_req.nof_ranks );
		
	// TODO: again introspect and make the json tree parser tolerant to expression nodes
	const strus::PostingJoinOperatorInterface *op = query_processor->getPostingJoinOperator( "contains" );	

	// number of terms, range of operator, cardinality = minimal matching subelements in expr-tree
	query->pushTerm( "word", qry_req.text );
	query->defineFeature( "feat", 1.0 );
	query->pushTerm( "word", qry_req.text );
	query->pushExpression( op, 1, 0, 0 );
	query->defineFeature( "sel", 1.0 );

	std::vector<strus::ResultDocument> ranklist = query->evaluate( );
	if( service.hasError( ) ) {
		report_error( ERROR_QUERY_CMD_QUERY_EVALUATE, service.getLastStrusError( ) );
		delete query;
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

	cppcms::json::value j;
	j["ranklist"] = response;
	
	report_ok( j );
}

} // namespace apps

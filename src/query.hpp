#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

#include "strus/arithmeticVariant.hpp"

#include <boost/typeof/typeof.hpp>

#include "constants.hpp"
#include "utils.hpp"

#include "strus/index.hpp"

#include <vector>
#include <string>

struct QueryRequestBase {
};

enum ParameterType {
	PARAMETER_TYPE_UNKNOWN,
	PARAMETER_TYPE_STRING,
	PARAMETER_TYPE_NUMERIC
};
	
struct ParameterValue {
	enum ParameterType type;
	std::string s;
	strus::ArithmeticVariant n;
	
	ParameterValue( )
		: type( PARAMETER_TYPE_UNKNOWN ) { }
		
	ParameterValue( const int &i )
		: type( PARAMETER_TYPE_NUMERIC ),
		n( i ) { }
	
	ParameterValue( const float &f )
		: type( PARAMETER_TYPE_NUMERIC ),
		n( f ) { }
		
};

struct WeightingScheme {
	std::string name;
	std::vector<std::pair<std::string, struct ParameterValue> > params;	
};

struct WeightingParameters {
	struct WeightingScheme scheme;
};

struct SummarizerParameters {
	std::vector<std::string> attributes;
};

struct QueryRequest : public QueryRequestBase {
	std::string text;
	size_t first_rank;
	size_t nof_ranks;
	struct SummarizerParameters summarizer;
	struct WeightingParameters weighting;
	
	QueryRequest( ) : 
		text( "" ),
		first_rank( DEFAULT_QUERY_FIRST_RANK ),
		nof_ranks( DEFAULT_QUERY_NOF_RANKS ),
		summarizer( ), weighting( ) { }
};

struct QueryResponseBase{
};

struct Rank {
	strus::Index docno;
	float weight;
	std::vector<std::pair<std::string, std::string> > attributes;
};

struct QueryResponse : public QueryResponseBase {
	std::vector<Rank> ranks;	
};

namespace apps {

class query : public master {

	public:
		query( strusWebService &service );
		
	private:
		void query_url_cmd( const std::string name, const std::string qry );
		void query_payload_cmd( const std::string name );
		void query_cmd( const std::string name, const std::string qry, bool query_in_url );
};

} // namespace apps

namespace cppcms {
	namespace json {

template<>
struct traits<QueryRequest> {

	static QueryRequest get( value const &v )
	{
		QueryRequest q;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		q.text = v.get<std::string>( "text", "" );		
		q.first_rank = v.get<size_t>( "first_rank", DEFAULT_QUERY_FIRST_RANK );
		q.nof_ranks = v.get<size_t>( "nof_ranks", DEFAULT_QUERY_NOF_RANKS );
		//~ struct SummarizerParameters standard_summarizer;
		//~ standard_summarizer.attributes.push_back( DEFAULT_ATTRIBUTE_DOCID );
		//~ q.summarizer = v.get<struct SummarizerParameters>( "summarizer", standard_summarizer );
		q.summarizer = v.get<struct SummarizerParameters>( "summarizer" );
		struct WeightingParameters standard_weighting;
		standard_weighting.scheme.name = DEFAULT_WEIGHTING_SCHEME;
		standard_weighting.scheme.params.push_back( std::make_pair( "k1", ParameterValue( DEFAULT_BM25_K1 ) ) );
		standard_weighting.scheme.params.push_back( std::make_pair( "b", ParameterValue( DEFAULT_BM25_B ) ) );
		standard_weighting.scheme.params.push_back( std::make_pair( "avgdoclen", ParameterValue( DEFAULT_BM25_AVGDOCLEN ) ) );
		q.weighting = v.get<struct WeightingParameters>( "weighting", standard_weighting );
		return q;
	}
	
	static void set( value &v, QueryRequest const &q )
	{
		v.set( "text", q.text );
		v.set( "first_rank", q.first_rank );
		v.set( "nof_ranks", q.nof_ranks );
		v.set( "summarizer", q.summarizer );
		v.set( "weighting", q.weighting );
	}
};

template<>
struct traits<struct SummarizerParameters> {
	
	static struct SummarizerParameters get( value const &v )
	{
		struct SummarizerParameters p;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		p.attributes = v.get<std::vector<std::string> >( "attributes" );
		return p;
	}
	
	static void set( value &v, struct SummarizerParameters const &p )
	{
		v.set( "attributes", p.attributes );
	}
};

template<>
struct traits<struct WeightingParameters> {
	
	static struct WeightingParameters get( value const &v )
	{
		struct WeightingParameters p;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		p.scheme = v.get<struct WeightingScheme>( "scheme" );
		return p;
	}
	
	static void set( value &v, struct WeightingParameters const &p )
	{
		v.set( "scheme", p.scheme );
	}
};

template<>
struct traits<struct WeightingScheme> {
	
	static struct WeightingScheme get( value const &v )
	{
		struct WeightingScheme s;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		s.name = v.get<std::string>( "name" );
		s.params = v.get<std::vector<std::pair<std::string, struct ParameterValue> > >( "params" );
		return s;
	}
	
	static void set( value &v, struct WeightingScheme const &s )
	{
		v.set( "name", s.name );
		v.set( "params", s.params );
	}
};

template<>
struct traits<std::pair< std::string, struct ParameterValue> > {
	
	static std::pair< std::string, struct ParameterValue> get( value const &v )
	{
		std::pair< std::string, struct ParameterValue> p;
		
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		p.first = v.get<std::string>( "key" );
		value val = v["value"];
		switch( val.type( ) ) {
			case is_boolean:
				// TODO: really? Do we allow this?
				p.second.type = PARAMETER_TYPE_NUMERIC;
				p.second.n = strus::ArithmeticVariant( ( v.get<bool>( "value" ) ) ? 1 : 0 );
				break;
				
			case is_string:
				p.second.type = PARAMETER_TYPE_STRING;
				p.second.s = v.get<std::string>( "value" );
				break;
						
			case is_number:
				p.second.type = PARAMETER_TYPE_NUMERIC;
				if( is_of_type( val, p.second.n.variant.Int ) ) {
					p.second.n = v.get<BOOST_TYPEOF( p.second.n.variant.Int )>( "value" );
				} else if( is_of_type( val, p.second.n.variant.UInt ) ) {
					p.second.n = v.get<BOOST_TYPEOF( p.second.n.variant.UInt )>( "value" );
				} else if( is_of_type( val, p.second.n.variant.Float ) ) {
					p.second.n = v.get<BOOST_TYPEOF( p.second.n.variant.Float )>( "value" );
				} else {
					throw bad_value_cast( );
				}
				break;
				
			case is_undefined:
			case is_null:
				// TODO: how do we map absence
			case is_object:
			case is_array:	
			default:
				throw bad_value_cast( );
		}
				
		return p;
	}
	
	static void set( value &v, std::pair< std::string, struct ParameterValue> const &p )
	{
		v.set( "key", p.first );
		switch( p.second.type ) {
			case PARAMETER_TYPE_STRING:
				v.set( "value", p.second.s );
				break;
			
			case PARAMETER_TYPE_NUMERIC:
				switch( p.second.n.type ) {
					case strus::ArithmeticVariant::Null:
						v.set( "value", cppcms::json::null( ) );
						break;
					
					case strus::ArithmeticVariant::Int:
						v.set( "value", p.second.n.toint( ) );
						break;
						
					case strus::ArithmeticVariant::UInt:
						v.set( "value", p.second.n.touint( ) );
						break;
						
					case strus::ArithmeticVariant::Float:
						v.set( "value", p.second.n.tofloat( ) );
						break;
					
					default:
						throw bad_value_cast( );
				}
				break;
			
			case PARAMETER_TYPE_UNKNOWN:
			default:
				throw bad_value_cast( );
		}
	}
};

template<>
struct traits<QueryResponse> {
	
	static QueryResponse get( value const &v )
	{
		QueryResponse r;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		r.ranks = v.get<std::vector<Rank> >( "ranks" );
		return r;
	}
	
	static void set( value &v, QueryResponse const &r )
	{
		v.set( "ranks", r.ranks );
	}
};

template<>
struct traits<Rank> {
	
	static Rank get( value const &v )
	{
		Rank r;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		r.docno = v.get<strus::Index>( "docno" );
		r.weight = v.get<float>( "weight" );
		r.attributes = v.get<std::vector<std::pair<std::string, std::string> > >( "attributes" );	
		return r;	
	}		
	
	static void set( value &v, Rank const &r )
	{
		v.set( "docno", r.docno );
		v.set( "weight", r.weight );
		v.set( "attributes", r.attributes );
	}
};

} } // namespace cppcms::json

#endif


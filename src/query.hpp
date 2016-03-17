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

#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

#include "strus/arithmeticVariant.hpp"
#include "strus/queryInterface.hpp"
#include "strus/queryProcessorInterface.hpp"

#include "constants.hpp"
#include "utils.hpp"

#include "strus/index.hpp"

#include <vector>
#include <string>

#include <boost/typeof/typeof.hpp>

struct Node {
	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const = 0;
	virtual Node *clone( ) const = 0;
};

struct Term : public Node {
	std::string type;
	std::string value;
	
	Term( ) : type( "" ), value( "" ) { }
	
	Term( const std::string &_type, const std::string &_value )
		: type( _type ), value( _value ) { }
	
	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const
	{
		query->pushTerm( type, value );
	}
	
	virtual Node *clone( ) const
	{
		return new Term( *this );
	}		
};

struct Expression : public Node {
	std::string operator_;
	int range;
	int cardinality;
	std::vector<Term> terms;	
	
	Expression( ) : operator_( "" ), range( 0 ), cardinality( 0 ), terms( ) { }
	
	Expression( const std::string &_operator, int _range, int _cardinality, const std::vector<Term> &_terms )
		: operator_( _operator ), range( _range ), cardinality( _cardinality ),
		terms( _terms ) { }

	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const
	{
		for( std::vector<Term>::const_iterator it = terms.begin( ); it != terms.end( ); it++ ) {
			it->produceQuery( query_processor, query );
		}
			
		const strus::PostingJoinOperatorInterface *op = query_processor->getPostingJoinOperator( operator_ );

		query->pushExpression( op, terms.size( ), range, cardinality );
	}

	virtual Node *clone( ) const
	{
		return new Expression( *this );
	}		
};

struct Feature : public Node {
	std::string name;
	float weight;

	Feature( ) : name( "" ), weight( 0.0 ) { }
	
	Feature( const std::string &_name, float _weight )
		: name( _name ), weight( _weight ) { }	

	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const
	{
	}
	
	virtual Node *clone( ) const
	{
		return new Feature( *this );
	}		
};

struct TermFeature : public Feature {
	Term term;

	TermFeature( ) : term( ) { }
	
	TermFeature( const std::string &_name, const Term &_term, float _weight )
		: Feature( _name, _weight ), term( _term ) { }

	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const
	{
		term.produceQuery( query_processor, query );
		query->defineFeature( name, weight );
	}

	virtual Node *clone( ) const
	{
		return new TermFeature( *this );
	}		
};

struct ExpressionFeature : public Feature {
	Expression expression;
	
	ExpressionFeature( ) : expression( ) { }
	
	ExpressionFeature( const std::string &_name, const Expression &_expression, float _weight )
		: Feature( _name, _weight ), expression( _expression ) { }

	virtual void produceQuery( strus::QueryProcessorInterface *query_processor, strus::QueryInterface *query ) const
	{
		expression.produceQuery( query_processor, query );
		query->defineFeature( name, weight );
	}

	virtual Node *clone( ) const
	{
		return new ExpressionFeature( *this );
	}		
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
	bool b;
	
	ParameterValue( )
		: type( PARAMETER_TYPE_UNKNOWN ) { }
		
	ParameterValue( const int &i )
		: type( PARAMETER_TYPE_NUMERIC ),
		n( i ) { }
	
	ParameterValue( const float &f )
		: type( PARAMETER_TYPE_NUMERIC ),
		n( f ) { }

	ParameterValue( const std::string &_s )
		: type( PARAMETER_TYPE_STRING ),
		s( _s ) { }
};

struct WeightingConfiguration {
	std::string name;
	std::vector<std::pair<std::string, struct ParameterValue> > params;
	float weight;
};

struct SummarizerConfiguration {
	std::string attribute;
	std::string name;
	std::vector<std::pair<std::string, struct ParameterValue> > params;
};

struct QueryRequestBase {
};

struct QueryRequest : public QueryRequestBase {
	size_t first_rank;
	size_t nof_ranks;
	std::vector<struct Feature *> features;
	std::vector<std::string> select;
	std::vector<std::string> restrict;
	std::vector<std::string> exclude;
	std::vector<struct WeightingConfiguration> weighting;
	std::vector<struct SummarizerConfiguration> summarizer;
	
	QueryRequest( ) : 
		first_rank( DEFAULT_QUERY_FIRST_RANK ),
		nof_ranks( DEFAULT_QUERY_NOF_RANKS ),
		features( ), select( ), restrict( ), exclude( ),
		weighting( ), summarizer( ) { }

	QueryRequest( const std::string &_text, size_t _first_rank = DEFAULT_QUERY_FIRST_RANK, size_t _nof_ranks = DEFAULT_QUERY_NOF_RANKS ) :
		first_rank( _first_rank ),
		nof_ranks( _nof_ranks ),
		features( ), select( ), restrict( ), exclude( ),
		weighting( ), summarizer( )
	{
		// TODO: standard query assumes a default type, must be in configuration
		// of the service with a compiled in default being 'word'.
		// assuming a standard normalization for text here. 'feat' and 'sel'
		// can be constants in the code. Maybe the default operator 'contains'
		// should also go to the configuration
		// the weighting scheme and the parameters for the default query should
		// also be configurable

		TermFeature *feature = new TermFeature( "feat", Term( "word", _text ), 1.0 );
		features.push_back( feature );

		std::vector<Term> terms;
		terms.push_back( Term( "word", _text ) );
		ExpressionFeature *expr = new ExpressionFeature( "sel", Expression( "contains", 0, 0, terms ), 1.0 );
		features.push_back( expr );
		
		select.push_back( "sel" );
		
		struct WeightingConfiguration standard_scheme;
		standard_scheme.name = DEFAULT_WEIGHTING_SCHEME;
		standard_scheme.params.push_back( std::make_pair( "k1", ParameterValue( DEFAULT_BM25_K1 ) ) );
		standard_scheme.params.push_back( std::make_pair( "b", ParameterValue( DEFAULT_BM25_B ) ) );
		standard_scheme.params.push_back( std::make_pair( "avgdoclen", ParameterValue( DEFAULT_BM25_AVGDOCLEN ) ) );
		standard_scheme.params.push_back( std::make_pair( "metadata_doclen", ParameterValue( DEFAULT_BM25_METADATA_DOCLEN ) ) );
		standard_scheme.params.push_back( std::make_pair( "match", ParameterValue( "feat" ) ) );
		standard_scheme.weight = 1.0;
		weighting.push_back( standard_scheme );

		struct SummarizerConfiguration standard_summarizer;
		standard_summarizer.attribute = DEFAULT_ATTRIBUTE_DOCID;
		standard_summarizer.name = DEFAULT_SUMMARIZER;
		standard_summarizer.params.push_back( std::make_pair( "name", ParameterValue( DEFAULT_ATTRIBUTE_DOCID ) ) );
		summarizer.push_back( standard_summarizer );
	}
	
	QueryRequest( const QueryRequest &q )
		: first_rank( q.first_rank ),
		nof_ranks( q.nof_ranks ),
		features( q.features ), select( q.select ), restrict( q.restrict ), exclude( q.exclude ),
		weighting( q.weighting ), summarizer( q.summarizer )
	{
		for( std::size_t i = 0; i < q.features.size( ); i++ ) {
			features[i] = dynamic_cast<Feature *>( q.features[i]->clone( ) );
		}
	}
	
	QueryRequest &operator=( const QueryRequest &q )
	{
		first_rank = q.first_rank;
		nof_ranks = q.nof_ranks;
		features = q.features;
		select = q.select;
		restrict = q.restrict;
		exclude = q.exclude;
		weighting = q.weighting;
		summarizer = q.summarizer;
		
		for( std::size_t i = 0; i < q.features.size( ); i++ ){
			features[i] = dynamic_cast<Feature *>( q.features[i]->clone( ) );
		}
		
		return *this;
	}
		
	virtual ~QueryRequest( ) {
		for( std::vector<Feature *>::const_iterator it = features.begin( ); it != features.end( ); it++ ) {
			free( *it );
		}
	}
	
	bool isFeatureInQuery( const std::string &feat ) 
	{
		for( std::vector<Feature *>::const_iterator it = features.begin( ); it != features.end( ); it++ ) {
			if( (*it)->name.compare( feat ) == 0 ) {
				return true;
			}			
		}
		return false;
	}
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
		q.first_rank = v.get<size_t>( "first_rank", DEFAULT_QUERY_FIRST_RANK );
		q.nof_ranks = v.get<size_t>( "nof_ranks", DEFAULT_QUERY_NOF_RANKS );
		
		q.features = v.get<std::vector<struct Feature *> >( "features", std::vector<struct Feature *>( ) );
		q.select = v.get<std::vector<std::string> >( "select", std::vector<std::string>( ) );
		q.restrict = v.get<std::vector<std::string> >( "restrict", std::vector<std::string>( ) );
		q.exclude = v.get<std::vector<std::string> >( "exclude", std::vector<std::string>( ) );
		
		std::vector<struct WeightingConfiguration> standard_weightings;
		struct WeightingConfiguration standard_scheme;
		standard_scheme.name = DEFAULT_WEIGHTING_SCHEME;
		standard_scheme.params.push_back( std::make_pair( "k1", ParameterValue( DEFAULT_BM25_K1 ) ) );
		standard_scheme.params.push_back( std::make_pair( "b", ParameterValue( DEFAULT_BM25_B ) ) );
		standard_scheme.params.push_back( std::make_pair( "avgdoclen", ParameterValue( DEFAULT_BM25_AVGDOCLEN ) ) );
		standard_scheme.params.push_back( std::make_pair( "metadata_doclen", ParameterValue( DEFAULT_BM25_METADATA_DOCLEN ) ) );
		standard_weightings.push_back( standard_scheme );
		q.weighting = v.get<std::vector<struct WeightingConfiguration> >( "weighting", standard_weightings );
		std::vector<struct SummarizerConfiguration> standard_summarizers;
		struct SummarizerConfiguration standard_summarizer;
		standard_summarizer.attribute = DEFAULT_ATTRIBUTE_DOCID;
		standard_summarizer.name = DEFAULT_SUMMARIZER;
		standard_summarizer.params.push_back( std::make_pair( "name", ParameterValue( DEFAULT_ATTRIBUTE_DOCID ) ) );
		standard_summarizers.push_back( standard_summarizer );
		q.summarizer = v.get<std::vector<struct SummarizerConfiguration> >( "summarizer", standard_summarizers );
		return q;
	}
	
	static void set( value &v, QueryRequest const &q )
	{
		v.set( "first_rank", q.first_rank );
		v.set( "nof_ranks", q.nof_ranks );
		v.set( "features", q.features );
		v.set( "select", q.select );
		v.set( "restrict", q.restrict );
		v.set( "exclude", q.exclude );
		v.set( "weighting", q.weighting );
		v.set( "summarizer", q.summarizer );
	}
};

template<>
struct traits<struct Feature *> {
	
	static struct Feature *get( value const &v )
	{
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}

		Expression expr = v.get<Expression>( "expression", Expression( ) );
		Term term = v.get<Term>( "term", Term( ) );

		Feature *f = 0;
		
		if( expr.operator_.compare( "" ) == 0 && term.type.compare( "" ) != 0 ) {
			f = new TermFeature( );
			dynamic_cast<TermFeature *>( f )->term = term;
		}
		if( expr.operator_.compare( "" ) != 0 && term.type.compare( "" ) == 0 ) {
			f = new ExpressionFeature( );
			dynamic_cast<ExpressionFeature *>( f )->expression = expr;
		}
		
		if( f == 0 ) {
			throw bad_value_cast( );
		}
		
		f->name = v.get<std::string>( "name" );
		f->weight = v.get<double>( "weight" );
		
		return f;
	}
	
	static void set( value &v, struct Feature const *f )
	{
		v.set( "name", f->name );
		v.set( "weight", f->weight );		

		const TermFeature *termFeat = dynamic_cast<const TermFeature *>( f );
		if( termFeat != 0 ) {
			v.set( "term", termFeat->term );
		}
		
		const ExpressionFeature *exprFeat = dynamic_cast<const ExpressionFeature *>( f );
		if( exprFeat != 0 ) {
			v.set( "expression", exprFeat->expression );
		}			
	}
};

template<>
struct traits<struct Expression> {
	
	static struct Expression get( value const &v )
	{
		Expression e;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		e.operator_ = v.get<std::string>( "operator" );
		e.range = v.get<int>( "range" );
		e.cardinality = v.get<int>( "cardinality" );
		e.terms = v.get<std::vector<Term> >( "terms" );
		
		return e;
	}
	
	static void set( value &v, struct Expression const e )
	{
		v.set( "operator", e.operator_ );
		v.set( "range", e.range );
		v.set( "cardinality", e.cardinality );
		v.set( "terms", e.terms );
	}
};			

template<>
struct traits<struct Term> {
	
	static struct Term get( value const &v )
	{
		Term t;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}		
		t.type = v.get<std::string>( "type" );
		t.value = v.get<std::string>( "value" );
		
		return t;
	}
	
	static void set( value &v, struct Term const t )
	{
		v.set( "type", t.type );
		v.set( "value", t.value );
	}
};

template<>
struct traits<struct SummarizerConfiguration> {

	static struct SummarizerConfiguration get( value const &v )
	{
		struct SummarizerConfiguration s;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		s.attribute = v.get<std::string>( "attribute" );
		s.name = v.get<std::string>( "name" );
		s.params = v.get<std::vector<std::pair<std::string, struct ParameterValue> > >( "params" );
		return s;
	}
	
	static void set( value &v, struct SummarizerConfiguration const &s )
	{
		v.set( "attribute", s.attribute );
		v.set( "name", s.name );
		v.set( "params", s.params );
	}
};

template<>
struct traits<struct WeightingConfiguration> {
	
	static struct WeightingConfiguration get( value const &v )
	{
		struct WeightingConfiguration s;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		s.name = v.get<std::string>( "name" );
		s.params = v.get<std::vector<std::pair<std::string, struct ParameterValue> > >( "params" );
		s.weight = v.get<double>( "weight" );
		return s;
	}
	
	static void set( value &v, struct WeightingConfiguration const &s )
	{
		v.set( "name", s.name );
		v.set( "params", s.params );
		v.set( "weight", s.weight );
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
			case is_boolean:
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


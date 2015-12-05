#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

#include "constants.hpp"

#include "strus/index.hpp"

#include <vector>

struct QueryRequestBase {
};

struct QueryRequest : public QueryRequestBase {
	std::string text;
	size_t first_rank;
	size_t nof_ranks;
	
	QueryRequest( ) : first_rank( DEFAULT_QUERY_FIRST_RANK ),
		nof_ranks( DEFAULT_QUERY_NOF_RANKS ) { }
};

struct QueryResponseBase{
};

struct Rank {
	strus::Index docno;
	float weight;
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
		
		return q;
	}
	
	static void set( value &v, QueryRequest const &q )
	{
		v.set( "text", q.text );
		v.set( "first_rank", q.first_rank );
		v.set( "nof_ranks", q.nof_ranks );
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
		
	}		
	
	static void set( value &v, Rank const &r )
	{
		v.set( "docno", r.docno );
		v.set( "weight", r.weight );
	}
};

} } // namespace cppcms::json

#endif


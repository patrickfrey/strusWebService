#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

struct QueryRequestBase {
};

struct QueryRequest : public QueryRequestBase {
	std::string text;
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
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		q.text = v.get<std::string>( "text", "" );		
		
		return q;
	}
	
	static void set( value &v, QueryRequest const &q )
	{
		v.set( "text", q.text );
	}
};

} } // namespace cppcms::json

#endif


#ifndef OTHER_HPP
#define OTHER_HPP

#include "master.hpp"

#include <cppcms/json.h>

struct ServiceConfiguration {
	std::vector<std::string> weighting_functions;
	std::vector<std::string> summarizer_functions;
	std::vector<std::string> posting_join_operators;
};

namespace apps {

class other : public master {

	public:
		other( strusWebService &service );
		
	private:
		void ping_cmd( );
		void version_cmd( );
		void config_cmd( );
};

} // namespace apps

namespace cppcms {
	namespace json {
		
template<>
struct traits<ServiceConfiguration> {

	static ServiceConfiguration get( value const &v )
	{
		ServiceConfiguration c;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}		
		c.weighting_functions = v.get<std::vector<std::string> >( "weighting_functions", std::vector<std::string>( ) );
		c.summarizer_functions = v.get<std::vector<std::string> >( "summarizer_functions", std::vector<std::string>( ) );
		c.posting_join_operators = v.get<std::vector<std::string> >( "posting_join_operators", std::vector<std::string>( ) );
		return c;
	}
	
	static void set( value &v, ServiceConfiguration const &c )
	{
		v.set( "weighting_funtions", c.weighting_functions );
		v.set( "summarizer_functions", c.summarizer_functions );
		v.set( "posting_join_operators", c.posting_join_operators );
	}

};

} } // namespace cppcms::json

#endif

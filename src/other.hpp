#ifndef OTHER_HPP
#define OTHER_HPP

#include "master.hpp"

#include <cppcms/json.h>

struct WeightingFunctionConfiguration {
	std::string name;
	std::string description;
	std::vector<std::string> parameter;
};

struct SummarizerFunctionConfiguration {
	std::string name;
	std::string description;
	std::vector<std::string> parameter;
};

struct ServiceConfiguration {
	std::vector<WeightingFunctionConfiguration> weighting_functions;
	std::vector<SummarizerFunctionConfiguration> summarizer_functions;
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
		c.weighting_functions = v.get<std::vector<WeightingFunctionConfiguration> >( "weighting_functions", std::vector<WeightingFunctionConfiguration>( ) );
		c.summarizer_functions = v.get<std::vector<SummarizerFunctionConfiguration> >( "summarizer_functions", std::vector<SummarizerFunctionConfiguration>( ) );
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

template<>
struct traits<WeightingFunctionConfiguration> {
	
	static WeightingFunctionConfiguration get( value const &v )
	{
		WeightingFunctionConfiguration c;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}		
		c.name = v.get<std::string>( "name" );
		c.description = v.get<std::string>( "description" );
		c.parameter = v.get<std::vector<std::string> >( "parameter" );
	}
	
	static void set( value &v, WeightingFunctionConfiguration const &c )
	{
		v.set( "name", c.name );
		v.set( "description", c.description );
		v.set( "parameter", c.parameter );
	}
	
};

template<>
struct traits<SummarizerFunctionConfiguration> {
	
	static SummarizerFunctionConfiguration get( value const &v )
	{
		SummarizerFunctionConfiguration c;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}		
		c.name = v.get<std::string>( "name" );
		c.description = v.get<std::string>( "description" );
		c.parameter = v.get<std::vector<std::string> >( "parameter" );
	}
	
	static void set( value &v, SummarizerFunctionConfiguration const &c )
	{
		v.set( "name", c.name );
		v.set( "description", c.description );
		v.set( "parameter", c.parameter );
	}
	
};

} } // namespace cppcms::json

#endif

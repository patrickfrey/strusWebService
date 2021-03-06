/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef OTHER_HPP
#define OTHER_HPP

#include "master.hpp"

#include <cppcms/json.h>

#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"

enum FunctionType {
	FUNCTION_TYPE_FEATURE,
	FUNCTION_TYPE_ATTRIBUTE,
	FUNCTION_TYPE_METADATA,
	FUNCTION_TYPE_NUMERIC,
	FUNCTION_TYPE_STRING
};

struct FunctionParameter {
	enum FunctionType type;
	std::string name;
	std::string description;
	
	FunctionParameter( ) { }
		
	FunctionParameter( strus::FunctionDescription::Parameter p )
	{
		name = p.name( );
		description = p.text( );
		switch( p.type( ) ) {
			case strus::FunctionDescription::Parameter::Feature:
				type = FUNCTION_TYPE_FEATURE;
				break;
			case strus::FunctionDescription::Parameter::Attribute:
				type = FUNCTION_TYPE_ATTRIBUTE;
				break;
			case strus::FunctionDescription::Parameter::Metadata:
				type = FUNCTION_TYPE_METADATA;
				break;
			case strus::FunctionDescription::Parameter::Numeric:
				type = FUNCTION_TYPE_NUMERIC;
				break;
			case strus::FunctionDescription::Parameter::String:
				type = FUNCTION_TYPE_STRING;
				break;
			}
	}
};

struct WeightingFunctionConfiguration {
	std::string name;
	std::string description;
	std::vector<FunctionParameter> parameter;
};

struct SummarizerFunctionConfiguration {
	std::string name;
	std::string description;
	std::vector<FunctionParameter> parameter;
};

struct PostingJoinOperatorConfiguration {
	std::string name;
	std::string description;
};

struct ServiceConfiguration {
	std::vector<WeightingFunctionConfiguration> weighting_functions;
	std::vector<SummarizerFunctionConfiguration> summarizer_functions;
	std::vector<PostingJoinOperatorConfiguration> posting_join_operators;
};

namespace apps {

class other : public master {

	public:
		other( strusWebService &service );
		void set_allow_quit_command( bool allow_quit_command );
		
	private:
		bool allow_quit_command;
		void ping_cmd( );
		void version_cmd( );
		void config_cmd( );
		void quit_cmd( );
};

} // namespace apps

namespace cppcms {
	namespace json {
		
template<>
struct traits<ServiceConfiguration> {

	static ServiceConfiguration get( value const &v )
	{
		ServiceConfiguration c;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}		
		c.weighting_functions = v.get<std::vector<WeightingFunctionConfiguration> >( "weighting_functions", std::vector<WeightingFunctionConfiguration>( ) );
		c.summarizer_functions = v.get<std::vector<SummarizerFunctionConfiguration> >( "summarizer_functions", std::vector<SummarizerFunctionConfiguration>( ) );
		c.posting_join_operators = v.get<std::vector<PostingJoinOperatorConfiguration> >( "posting_join_operators", std::vector<PostingJoinOperatorConfiguration>( ) );
		return c;
	}
	
	static void set( value &v, ServiceConfiguration const &c )
	{
		v.set( "weighting_functions", c.weighting_functions );
		v.set( "summarizer_functions", c.summarizer_functions );
		v.set( "posting_join_operators", c.posting_join_operators );
	}

};

template<>
struct traits<WeightingFunctionConfiguration> {
	
	static WeightingFunctionConfiguration get( value const &v )
	{
		WeightingFunctionConfiguration c;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}		
		c.name = v.get<std::string>( "name" );
		c.description = v.get<std::string>( "description" );
		c.parameter = v.get<std::vector<FunctionParameter> >( "parameter" );
		return c;
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
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}		
		c.name = v.get<std::string>( "name" );
		c.description = v.get<std::string>( "description" );
		c.parameter = v.get<std::vector<FunctionParameter> >( "parameter" );
		return c;
	}
	
	static void set( value &v, SummarizerFunctionConfiguration const &c )
	{
		v.set( "name", c.name );
		v.set( "description", c.description );
		v.set( "parameter", c.parameter );
	}
	
};

template<>
struct traits<PostingJoinOperatorConfiguration> {
	
	static PostingJoinOperatorConfiguration get( value const &v )
	{
		PostingJoinOperatorConfiguration c;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}		
		c.name = v.get<std::string>( "name" );
		c.description = v.get<std::string>( "description" );
		return c;
	}
	
	static void set( value &v, PostingJoinOperatorConfiguration const &c )
	{
		v.set( "name", c.name );
		v.set( "description", c.description );
	}
	
};

template<>
struct traits<FunctionParameter> {
	
	static FunctionParameter get( value const &v )
	{
		FunctionParameter p;
		if( v.type( ) != is_object ) {
			throw bad_value_cast( );
		}
		std::string s = v.get<std::string>( "type" );
		if( s.compare( "feature" ) == 0 ) {
			p.type = FUNCTION_TYPE_FEATURE;
		} else if( s.compare( "attribute" ) == 0 ) {
			p.type = FUNCTION_TYPE_ATTRIBUTE;
		} else if( s.compare( "metadata" ) == 0 ) {
			p.type = FUNCTION_TYPE_METADATA;
		} else if( s.compare( "numeric" ) == 0 ) {
			p.type = FUNCTION_TYPE_NUMERIC;
		} else if( s.compare( "string" ) == 0 ) {
			p.type = FUNCTION_TYPE_STRING;
		} else {
			throw bad_value_cast( );
		}
		p.name = v.get<std::string>( "name" );
		p.description = v.get<std::string>( "description" );
		return p;
	}
	
	static void set( value &v, FunctionParameter const &p )
	{
		switch( p.type ) {
			case FUNCTION_TYPE_FEATURE:
				v.set( "type", "feature" );
				break;
			case FUNCTION_TYPE_ATTRIBUTE:
				v.set( "type", "attribute" );
				break;
			case FUNCTION_TYPE_METADATA:
				v.set( "type", "metadata" );
				break;
			case FUNCTION_TYPE_NUMERIC:
				v.set( "type", "numeric" );
				break;
			case FUNCTION_TYPE_STRING:
				v.set( "type", "string" );
				break;
			default:
				throw bad_value_cast( );
		}				
		v.set( "name", p.name );
		v.set( "description", p.description );
	}
};

} } // namespace cppcms::json

#endif

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

#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include "master.hpp"

#include <cppcms/json.h>

struct TransactionRequestBase {
	std::string id;
};

struct TransactionBeginRequest : public TransactionRequestBase {
};

struct TransactionCommitRequest : public TransactionRequestBase {
};

struct TransactionRollbackRequest : public TransactionRequestBase {
};

namespace apps {

class transaction : public master {

	public:
		transaction( strusWebService &service );
		
	private:
		void begin_url_cmd( const std::string name, const std::string tid );
		void begin_payload_cmd( const std::string name );
		void begin_cmd( const std::string name, const std::string tid, bool tid_in_url );
		void commit_url_cmd( const std::string name, const std::string tid );
		void commit_payload_cmd( const std::string name );
		void commit_cmd( const std::string name, const std::string tid, bool tid_in_url );
		void rollback_url_cmd( const std::string name, const std::string tid );
		void rollback_payload_cmd( const std::string name );
		void rollback_cmd( const std::string name, const std::string tid, bool tid_in_url );
};

} // namespace apps

namespace cppcms {
	namespace json {

template<>
struct traits<TransactionBeginRequest> {
		
	static TransactionBeginRequest get( value const &v )
	{
		TransactionBeginRequest t;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		t.id = v.get<std::string>( "id", "" );
		return t;
	}
	
	static void set( value &v, TransactionBeginRequest const &t )
	{
		v.set( "id", t.id );
	}
};

template<>
struct traits<TransactionCommitRequest> {
		
	static TransactionCommitRequest get( value const &v )
	{
		TransactionCommitRequest t;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		t.id = v.get<std::string>( "id", "" );
		return t;
	}
	
	static void set( value &v, TransactionCommitRequest const &t )
	{
		v.set( "id", t.id );
	}
};

template<>
struct traits<TransactionRollbackRequest> {
		
	static TransactionRollbackRequest get( value const &v )
	{
		TransactionRollbackRequest t;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		t.id = v.get<std::string>( "id", "" );
		return t;
	}
	
	static void set( value &v, TransactionRollbackRequest const &t )
	{
		v.set( "id", t.id );
	}
};

} } // namespace cppcms::json

#endif

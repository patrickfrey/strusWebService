/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

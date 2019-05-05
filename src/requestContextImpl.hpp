/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 * Copyright (c) 2017,2018 Patrick P. Frey, Andreas Baumann, Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Context for requests disconnected from the application object (yield because of requests to other servers)
#ifndef _STRUS_WEBSERVICE_REQUEST_CONTEXT_IMPL_HPP_INCLUDED
#define _STRUS_WEBSERVICE_REQUEST_CONTEXT_IMPL_HPP_INCLUDED
#include "strus/webRequestAnswer.hpp"
#include "strus/webRequestContent.hpp"
#include <cppcms/application.h>
#include <cppcms/http_context.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <string>

namespace strus {
namespace webservice {

///\brief Forward declaration
class ServiceClosure;

class RequestContextImpl
{
public:
	typedef cppcms::http::context AppContext;
	typedef booster::shared_ptr<cppcms::http::context> AppContextRef;

	RequestContextImpl( AppContext& appContext_, ServiceClosure* serviceClosure_);
	RequestContextImpl( const AppContextRef& appContext_, ServiceClosure* serviceClosure_);

public:
	cppcms::http::response& response() const
	{
		return m_appContext->response();
	}
	cppcms::http::request& request() const
	{
		return m_appContext->request();
	}

	/// \brief Set response content header
	void response_content_header( const char* charset, const char* doctype, std::size_t blobsize);
	/// \brief Set response content
	void response_content( const strus::WebRequestContent& content, bool with_content);
	/// \brief Set response content
	void response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize);
	/// \brief Set response message
	void response_message( const char* messagetype, const char* messagestr);

	/// \brief Report error reply
	void report_error( int httpstatus, int apperror, const char* message);
	/// \brief Report error reply
	void report_error_fmt( int httpstatus, int apperror, const char* fmt, ...);
	/// \brief Report successful protocol only command
	void report_ok( const char* status, int httpstatus, const char* message);
	/// \brief Report content
	void report_answer( const strus::WebRequestAnswer& answer, bool with_content);

	/// \brief Report a message (http status 200)
	void report_message( const std::string& key, const std::string& message);

private:
	AppContext* m_appContext;
	AppContextRef m_appContextOwnership;
	ServiceClosure* m_serviceClosure;
};

}}//namespace strus::webservice
#endif


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
/// \brief Implementation of the cppcms application class for the strus webservice 
#ifndef _STRUS_WEBSERVICE_APPLICATION_IMPL_HPP_INCLUDED
#define _STRUS_WEBSERVICE_APPLICATION_IMPL_HPP_INCLUDED
#include "strus/webRequestHandlerInterface.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "strus/webRequestContent.hpp"
#include "strus/base/unique_ptr.hpp"
#include "webRequestLogger.hpp"
#include <cppcms/application.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <string>

namespace strus {
namespace webservice {

///\brief Forward declaration
class ServiceClosure;

class Application
	:public cppcms::application
{
public:
	explicit Application( cppcms::service& service_, ServiceClosure* serviceImpl_);

private:/*Methods called by dispatcher:*/
	void exec_quit();
	void exec_ping();
	void exec_version( std::string component);
	void exec_version0();
	void exec_request( std::string path);

private:
	// Handler method variants distinguished by number of arguments:
	typedef void(Application::*UrlHandlerMethod1)(std::string);
	typedef void(Application::*UrlHandlerMethod0)();

	/// \brief Map an URL with a main path dir and 1 string argument as subdirectory
	void urlmap( const char* dir, UrlHandlerMethod1 handler, bool more=false);
	/// \brief Map an URL with a main path dir and no string arguments as subdirectories
	void urlmap( const char* dir, UrlHandlerMethod0 handler);

	/// \brief Set response content
	void response_content( const strus::WebRequestContent& content, bool with_content);

	/// \brief Set response content header
	void response_content_header( const char* charset, const char* doctype, std::size_t blobsize);
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

	/// \brief Initialize all dispatchers (called from constructor)
	void init_dispatchers();

	std::string debug_request_description();

private:
	bool handle_preflight_cors();

	/// \brief check if the request is of a certain type and report an error if not
	bool check_request_method( const char* type);
	/// \brief test if the request is of a certain type without any reaction on it
	bool test_request_method( const char* type);

private:
	ServiceClosure* m_service;
};

}}//namespace strus::webservice
#endif



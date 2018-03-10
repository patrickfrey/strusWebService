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
#include <set>
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

	void exec_put_config( std::string path);
	void exec_post( std::string path);
	void exec_debug_post( std::string path);

	void exec_quit();
	void exec_ping();
	void exec_version( std::string component);
	void exec_version0();
	void exec_list( std::string path);
	void exec_list0();
	void exec_view( std::string path);
	void exec_view0();

private:
	// Handler method variants distinguished by number of arguments
	typedef void(Application::*UrlHandlerMethod1)(std::string);
	typedef void(Application::*UrlHandlerMethod0)();

	/// \brief Map an URL with a main path dir and 1 string argument as subdirectory
	void urlmap( const char* dir, UrlHandlerMethod1 handler, bool more=false);
	/// \brief Map an URL with a main path dir and no string arguments as subdirectories
	void urlmap( const char* dir, UrlHandlerMethod0 handler);

	/// \brief Set response content
	void response_content( const strus::WebRequestContent& content);
	/// \brief Set response content
	void response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize);

	/// \brief Report error reply
	void report_fatal();
	/// \brief Report error reply
	void report_error( int httpstatus, int apperror, const char* message);
	/// \brief Report error reply
	void report_error_fmt( int httpstatus, int apperror, const char* fmt, ...);
	/// \brief Report successful protocol only command
	void report_ok( const char* status, int httpstatus, const char* message);
	/// \brief Report content
	void report_answer( const strus::WebRequestAnswer& answer);

	/// \brief Initialize all dispatchers (called from constructor)
	void init_dispatchers();

	/// \brief Common implementation of the put_config_.. methods
	void put_config_internal( const std::string& contexttype, const std::string& contextname, const std::string& schemaname);
	/// \brief Common implementation of the exec_post_.. and exec_debug_post_.. methods
	enum ContentMethod {ContentDebug,ContentExec};
	void exec_content_internal( ContentMethod method, const std::string& contexttype, const std::string& contextname, const std::string& schemaname, const std::string& argument);
	/// \brief Common implementation of the exec_list_.. and exec_view_.. methods
	enum GetMethod {GetList,GetView};
	void exec_get_internal( GetMethod method, const std::string& path);

	std::string debug_request_description();

private:
	bool handle_preflight_cors();

	/// \brief check if the request is of a certain type and report an error if not
	bool check_request_method( const char* type);
	/// \brief test if the request is of a certain type without any reaction on it
	bool test_request_method( const char* type);

private:
	ServiceClosure* m_service;
	std::set<std::string> m_schemeset;
};

}}//namespace strus::webservice
#endif



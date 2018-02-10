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
#include <map>
#include <string>

///\brief Forward declaration
class ServiceClosure;

class Application
	:public cppcms::application
{
public:
	explicit Application( cppcms::service& service_, ServiceClosure* serviceImpl_);

	void exec_post_config( std::string const& new_context, std::string const& from_context, std::string const& schema);
	void exec_post2( std::string const& contextname, std::string const& schemaname);
	void exec_post3( std::string const& contextname, std::string const& schemaname, std::string const& argument);
	void debug_post2( std::string const& contextname, std::string const& schemaname);
	void debug_post3( std::string const& contextname, std::string const& schemaname, std::string const& argument);

	void exec_quit();
	void exec_ping();
	void exec_version();
	void exec_help();

private:
	/// \brief Set response content
	void response_content( const strus::WebRequestContent& content);
	/// \brief Set response content
	void response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize);

	/// \brief Report error reply
	void report_fatal();
	/// \brief Report error reply
	void report_error( int httpstatus, int apperror, const char* message);
	/// \brief Report successful protocol only command
	void report_ok( const char* status, int httpstatus, const char* message);
	/// \brief Report successful protocol only command with a map as argument
	void report_ok( const char* status, int httpstatus, const char* rootelem, const std::map<std::string,std::string>& message);
	/// \brief Report content
	void report_answer( const strus::WebRequestAnswer& answer);

	/// \brief Report error for not found
	void not_found_404();

	/// \brief Initialize all dispatchers (called from constructor)
	void init_dispatchers();

	/// \brief Get the user role from the request
	std::string request_role() const;

	/// \brief Common implementation of the exec_post_.. methods
	void exec_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument);
	/// \brief Common implementation of the debug_post_.. methods
	void debug_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument);

private:
	bool handle_preflight_cors();

private:
	ServiceClosure* m_service;
};

#endif



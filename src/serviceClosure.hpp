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
/// \brief Closure with all objects used by the cppcms service class for the strus webservice 
#ifndef _STRUS_WEBSERVICE_SERVICE_CLOSURE_HPP_INCLUDED
#define _STRUS_WEBSERVICE_SERVICE_CLOSURE_HPP_INCLUDED
#include "webRequestLogger.hpp"
#include "defaultContants.hpp"
#include "strus/webRequestHandlerInterface.hpp"
#include "internationalization.hpp"
#include <cppcms/service.h>
#include <booster/log.h>
#include <cppcms/applications_pool.h>
#include <vector>
#include <string>
#include <algorithm>

/// \class ServiceClosure
/// \brief Closure with all objects used by the cppcms service class for the strus webservice 
class ServiceClosure
{
public:
	ServiceClosure( const cppcms::json::value& config, bool verbose)
		:m_service(0),m_requestLogger(0),m_requestHandler(0)
		,m_cors_enabled(true),m_quit_enabled(false),m_debug_enabled(false),m_pretty_print(false)
	{
		init( config, verbose);
	}
	~ServiceClosure()
	{
		clear();
	}

	//\brief Create service, logging and command handler
	void init( const cppcms::json::value& config, bool verbose);

	void run()
	{
		if (m_service) m_service->run();
	}
	void shutdown()
	{
		if (m_service) m_service->shutdown();
	}
	int threads_no()
	{
		return m_service ? m_service->threads_no() : 0;
	}
	void mount_applications();

	strus::WebRequestLoggerInterface* requestLogger() const
	{
		return m_requestLogger;
	}
	const strus::WebRequestHandlerInterface* requestHandler() const
	{
		return m_requestHandler;
	}

	bool has_preflight_cors_origin( const std::string& origin) const
	{
		return std::find(m_cors_hosts.begin(), m_cors_hosts.end(), origin) != m_cors_hosts.end();
	}
	const std::string& cors_age() const
	{
		return m_cors_age;
	}
	bool cors_enabled() const
	{
		return m_cors_enabled;
	}
	bool quit_enabled() const
	{
		return m_quit_enabled;
	}
	bool debug_enabled() const
	{
		return m_debug_enabled;
	}
	bool pretty_print() const
	{
		return m_pretty_print;
	}

private:
	//\brief Destroy service, logging and command handler if initialized
	void clear();

	void loadHandlerConfiguration( const cppcms::json::value& config);
	void loadCorsConfiguration( const cppcms::json::value& config);
	void loadProtocolConfiguration( const cppcms::json::value& config);

private:
	cppcms::service* m_service;
	strus::WebRequestLogger* m_requestLogger;
	strus::WebRequestHandlerInterface* m_requestHandler;
	std::vector<std::string> m_cors_hosts;
	std::string m_cors_age;
	bool m_cors_enabled;
	bool m_quit_enabled;
	bool m_debug_enabled;
	bool m_pretty_print;
};

#endif



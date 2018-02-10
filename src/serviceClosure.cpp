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
#include "strus/lib/webrequest.hpp"
#include "strus/lib/error.hpp"
#include "serviceClosure.hpp"
#include "applicationImpl.hpp"
#include "defaultContants.hpp"
#include "strus/webRequestHandlerInterface.hpp"
#include "strus/webRequestAnswer.hpp"
#include "strus/webRequestContent.hpp"
#include "strus/errorCodes.hpp"
#include "strus/base/string_format.hpp"
#include "internationalization.hpp"
#include <cppcms/service.h>
#include <booster/log.h>
#include <cppcms/applications_pool.h>

#define STRUS_LOWLEVEL_DEBUG

void ServiceClosure::init( const cppcms::json::value& config, bool verbose)
{
	clear();
	m_service = new cppcms::service( config);

	bool doLogRequests = config.get( "debug.log_requests", DefaultConstants::DO_LOG_REQUESTS());
	int nofThreads = m_service->threads_no();
	int nofProcs = m_service->procs_no();
	if (nofProcs > 1)
	{
		throw strus::runtime_error(_TXT("it is currently not possible to configure more than one process (%d)"), nofProcs);
	}
	std::string requestLogFilename = config.get( "debug.request_file", DefaultConstants::REQUEST_LOG_FILE());
	m_requestLogger = new strus::WebRequestLogger( requestLogFilename, verbose, doLogRequests, nofThreads+1, m_service->process_id(), nofProcs);
	m_requestHandler = strus::createWebRequestHandler( m_requestLogger);
	if (!m_requestHandler) throw std::bad_alloc();

	loadCorsConfiguration( config);
	loadHandlerConfiguration( config);
	loadProtocolConfiguration( config);
}

void ServiceClosure::loadProtocolConfiguration( const cppcms::json::value& config)
{
	m_quit_enabled = config.get( "debug.protocol.quit_enabled", DefaultConstants::DEBUG_PROTOCOL_QUIT_ENABLED());
	m_debug_enabled = config.get( "debug.protocol.debug_enabled", DefaultConstants::DEBUG_PROTOCOL_DEBUG_ENABLED());
	m_pretty_print = config.get( "debug.protocol.pretty_print", DefaultConstants::DEBUG_PROTOCOL_PRETTY_PRINT());
}

void ServiceClosure::loadCorsConfiguration( const cppcms::json::value& config)
{
	m_cors_enabled = config.get( "security.cors.enable", false);
	cppcms::json::array allowed_origins = config.get( "security.cors.allowed_origins", cppcms::json::array());
	cppcms::json::array::const_iterator ai = allowed_origins.begin(), ae = allowed_origins.end();
	for (;ai != ae; ++ai)
	{
		m_cors_hosts.push_back( ai->get_value<std::string>());
	}
	m_cors_age = config.get( "security.cors.age", DefaultConstants::CORS_AGE());
}

static std::runtime_error configuration_error( const strus::WebRequestAnswer& status)
{
	strus::ErrorCode errcode( status.apperror());
	const char* componentname = errorComponentName( errcode.component());
	int operation = errcode.operation();
	int cause = errcode.cause();
	return strus::runtime_error( _TXT("error loading configuration (in %s op %d error code %d): %s"), componentname, operation, cause, status.errorstr());
}

void ServiceClosure::loadHandlerConfiguration( const cppcms::json::value& config)
{
	std::string configstr( config.save());
	strus::WebRequestAnswer status;
	static const char* init_context = "init";
	static const char* init_schema = "init";
	static const char* config_doctype = "json";
	static const char* config_charset = "utf-8";

	strus::WebRequestContent content( config_charset, config_doctype, configstr.c_str(), configstr.size());

	BOOSTER_DEBUG( DefaultConstants::PACKAGE()) << strus::string_format( _TXT( "loading request handler configuration (schema %s)"), init_schema);
	if (!m_requestHandler->loadConfiguration(
		init_context/*destContextName*/, init_context/*destContextPrefix*/, NULL/*srcContext*/, init_schema, content, status))
	{
		throw configuration_error( status);
	}
	cppcms::json::object::const_iterator oi = config.object().begin(), oe = config.object().end();
	for (; oi != oe; ++oi)
	{
		std::string sectionName = oi->first;
		std::string schema = std::string(init_context) + "_" + sectionName;

		// Search subconfiguration schema with a name "init" as prefix concatenated with the title of the configuration section:
		if (m_requestHandler->hasSchema( schema.c_str()))
		{
			std::string destContextName = oi->second.get( "id", sectionName);
			//... the name of the context created by the configuration is the name configured as "id" or the section name if not specified
			const std::string& destContextSchemaPrefix = oi->first;
			//... the prefix of all schemas working on the context created is the configuration section name
			cppcms::json::value subconfig;
			subconfig.set( oi->first, oi->second);
			//... the subconfiguration is the configuration section with the section name as root
			std::string subconfigstr( subconfig.save());
			strus::WebRequestContent subcontent( config_charset, config_doctype, subconfigstr.c_str(), subconfigstr.size());

			BOOSTER_DEBUG( DefaultConstants::PACKAGE()) 
				<< strus::string_format( _TXT( "loading handler sub configuration for %s %s (schema %s, context %s)"),
						destContextSchemaPrefix.c_str(), destContextName.c_str(), schema.c_str(), init_context);
			if (!m_requestHandler->loadConfiguration(
				destContextName.c_str(), destContextSchemaPrefix.c_str(), init_context, schema.c_str(), subcontent, status))
			{
				throw configuration_error( status);
			}
		}
	}
}

void ServiceClosure::mount_applications()
{
	if (m_service) m_service->applications_pool().mount( cppcms::applications_factory<Application>( this));
}

void ServiceClosure::clear()
{
	if (m_service) delete m_service; m_service = 0;
	if (m_requestLogger) delete m_requestLogger; m_requestLogger = 0;
	if (m_requestHandler) delete m_requestHandler; m_requestHandler = 0;
}


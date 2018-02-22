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
#include "strus/base/fileio.hpp"
#include "internationalization.hpp"
#include <cppcms/service.h>
#include <booster/log.h>
#include <cppcms/applications_pool.h>

#undef STRUS_LOWLEVEL_DEBUG

void ServiceClosure::init( const cppcms::json::value& config, bool verbose)
{
	clear();
	m_service = new cppcms::service( config);
	loadHtmlConfiguration( config);

	bool doLogRequests = config.get( "debug.log_requests", DefaultConstants::DO_LOG_REQUESTS());
	int nofThreads = m_service->threads_no();
	int nofProcs = m_service->procs_no();
	if (nofProcs > 1)
	{
		throw strus::runtime_error(_TXT("it is currently not possible to configure more than one process (%d)"), nofProcs);
	}
	std::string requestLogFilename = config.get( "debug.request_file", DefaultConstants::REQUEST_LOG_FILE());
	m_requestLogger = new strus::WebRequestLogger( requestLogFilename, verbose, doLogRequests, nofThreads+1, m_service->process_id(), nofProcs);
	m_requestHandler = strus::createWebRequestHandler( m_requestLogger, m_html_head);
	if (!m_requestHandler) throw std::bad_alloc();

	loadCorsConfiguration( config);
	loadHandlerConfiguration( config);
	loadProtocolConfiguration( config);
}

static std::string css_style_include( const std::string& content)
{
	return std::string("<style>\n") + content + "</style>\n";
}
static std::string css_style_link( const std::string& link)
{
	return strus::string_format( "<link rel=\"stylesheet\" href=\"%s\">\n", link.c_str());
}
static std::string readIncludeContent( const std::string& cfgdir, const std::string& cfgfile)
{
	std::string content;
	std::string filename;
	if (strus::isRelativePath( cfgfile))
	{
		filename = strus::joinFilePath( cfgdir, cfgfile);
		if (filename.empty()) throw std::bad_alloc();
	}
	else
	{
		filename = cfgfile;
	}
	int ec = strus::readFile( filename, content);
	if (ec) throw strus::runtime_error(_TXT("failed to read CSS file '%s': %s"), filename.c_str(), ::strerror(ec));
	return content;
}

void ServiceClosure::loadHtmlConfiguration( const cppcms::json::value& config)
{
	if (config.find( "html.css").is_undefined())
	{
		m_html_head = css_style_include( DefaultConstants::HTML_DEFAULT_STYLE());
	}
	else
	{
		int cnt = 0;
		if (!config.find( "html.css.link").is_undefined())
		{
			m_html_head = css_style_link( config.get( "html.css.link", ""));
			if (m_html_head.empty()) throw std::bad_alloc();
			++cnt;
		}
		if (!config.find( "html.css.include").is_undefined())
		{
			m_html_head = css_style_include( config.get( "html.css.include", DefaultConstants::HTML_DEFAULT_STYLE()));
			++cnt;
		}
		if (!config.find( "html.css.file").is_undefined())
		{
			m_html_head = css_style_include( readIncludeContent( m_configdir, config.get( "html.css.file", ".css")));
			++cnt;
		}
		if (cnt == 0)
		{
			throw std::runtime_error( _TXT("html.css is defined in configuration but neither html.css.link nor html.css.include nor html.css.file are"));
		}
		else if (cnt > 1)
		{
			throw std::runtime_error( _TXT("contradicting definitions of html.css.link or html.css.include or html.css.file"));
		}
	}
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
	static const char* root_context = "context";
	static const char* init_schema = "init";
	static const char* config_doctype = "json";
	static const char* config_charset = "utf-8";

	strus::WebRequestContent content( config_charset, config_doctype, configstr.c_str(), configstr.size());

	BOOSTER_DEBUG( DefaultConstants::PACKAGE()) << strus::string_format( _TXT( "loading request handler configuration (schema %s)"), init_schema);
	if (!m_requestHandler->loadConfiguration(
		root_context/*destContextType*/, root_context/*destContextName*/, NULL/*srcContext*/, init_schema, content, status))
	{
		throw configuration_error( status);
	}
	cppcms::json::object::const_iterator oi = config.object().begin(), oe = config.object().end();
	for (; oi != oe; ++oi)
	{
		const std::string& sectionName = oi->first;

		// Search subconfiguration schema with a name "init" as prefix concatenated with the title of the configuration section:
		if (m_requestHandler->hasSchema( root_context, sectionName.c_str()/*schema*/))
		{
			std::string destContextName = oi->second.get( "id", sectionName);
			//... the name of the context created by the configuration is the name configured as "id" or the section name if not specified
			const std::string& destContextType = oi->first;
			//... the prefix of all schemas working on the context created is the configuration section name
			cppcms::json::value subconfig;
			subconfig.set( sectionName, oi->second);
			//... the subconfiguration is the configuration section with the section name as root
			std::string subconfigstr( subconfig.save());
			strus::WebRequestContent subcontent( config_charset, config_doctype, subconfigstr.c_str(), subconfigstr.size());

			BOOSTER_DEBUG( DefaultConstants::PACKAGE()) 
				<< strus::string_format( _TXT( "loading handler sub configuration for %s %s (schema %s, context %s)"),
						destContextType.c_str(), destContextName.c_str(), sectionName.c_str()/*schema*/, root_context);
			if (!m_requestHandler->loadConfiguration(
				destContextType.c_str(), destContextName.c_str(), root_context, sectionName.c_str()/*schema*/, subcontent, status))
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


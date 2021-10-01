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
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/webRequestAnswer.hpp"
#include "strus/webRequestContent.hpp"
#include "strus/errorCodes.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/fileio.hpp"
#include "internationalization.hpp"
#include <cppcms/service.h>
#include <booster/log.h>
#include <cppcms/applications_pool.h>
#include <stdexcept>

using namespace strus::webservice;

static bool isAlpha( unsigned char ch)
{
	return ((ch|32) >= 'a' && (ch|32) <= 'z');
}

static bool url_has_protocol_prefix( const std::string& url)
{
	char const* si = url.c_str();
	while (isAlpha(*si)) ++si;
	return *si == ':';
}

ServiceClosure::~ServiceClosure()
{
	if (m_service) {m_service->~service(); m_service = 0;}
	if (m_requestLogger) {delete m_requestLogger; m_requestLogger = 0;}
	if (m_requestHandler) {delete m_requestHandler; m_requestHandler = 0;}
	if (m_eventloop) {delete m_eventloop; m_eventloop=0;}
	if (m_errorhnd) {delete m_errorhnd; m_errorhnd = 0;}
}

void ServiceClosure::init( const cppcms::json::value& config, int verbosity)
{
	try
	{
		std::string configstr( config.save());
		std::memset( &m_service_mem, 0, sizeof(m_service_mem)); //... use private nulled mem to supress valgrind UMR message
		m_service = new (&m_service_mem) cppcms::service( config);
		loadHtmlConfiguration( config);
		loadCorsConfiguration( config);
		loadProtocolConfiguration( config);

		m_identifier = config.get( "service.name", DefaultConstants::DEFAULT_SERVICE_NAME());
		m_port = config.get( "service.port", DefaultConstants::DEFAULT_HTTP_PORT());
		m_loglevel = config.get( "logging.level", DefaultConstants::LOGGING_LEVEL());
		int max_idle_time = config.get( "transactions.max_idle_time", DefaultConstants::TRANSACTION_MAX_IDLE_TIME());
		int transactions_per_second = config.get( "transactions.nof_per_sec", DefaultConstants::TRANSACTION_MAP_SLOT_SIZE());
		int nofThreads = m_service->threads_no();
		int nofProcs = m_service->procs_no();
		if (nofProcs > 1)
		{
			throw strus::runtime_error(_TXT("it is currently not possible to configure more than one process (%d)"), nofProcs);
		}
		DebugTraceInterface* dbgtrace = strus::createDebugTrace_standard( nofThreads+1);
		m_errorhnd = strus::createErrorBuffer_standard( NULL, nofThreads+1, dbgtrace);
		if (!m_errorhnd) throw std::runtime_error( _TXT( "error creating error handler"));
		m_put_configdir = config.get( "data.configdir", DefaultConstants::DefaultConstants::AUTOSAVE_CONFIG_DIR());
		m_http_server_name = config.get( "http.server", DefaultConstants::DefaultConstants::HTTP_SERVER_NAME());
		m_http_script_name = config.get( "http.script", DefaultConstants::DefaultConstants::HTTP_SCRIPT_NAME());
		int nofDelegateTotalConnections = config.get( "security.delegate_connections", DefaultConstants::DefaultConstants::MAX_DELEGATE_CONNECTIONS());
		int nofDelegateHostConnections = nofDelegateTotalConnections; //... nof host conn same as total nof conn

		if (!m_http_server_name.empty())
		{
			if (!url_has_protocol_prefix( m_http_server_name))
			{
				m_http_server_url.append( "http://");
			}
			m_http_server_url.append( m_http_server_name);
			m_http_server_url.push_back('/');
			if (!m_http_script_name.empty())
			{
				if (m_http_script_name[0] == '/')
				{
					m_http_server_url.append( m_http_script_name.c_str()+1);
				}
				else
				{
					m_http_server_url.append( m_http_script_name);
				}
				m_http_server_url.push_back('/');
			}
		}
		std::string requestLogFilename = config.get( "debug.request_file", DefaultConstants::REQUEST_LOG_FILE());
		m_requestLogger = new strus::WebRequestLogger( requestLogFilename, verbosity, m_loglevel, nofThreads+1, m_service->process_id(), nofProcs);
		int timeout = std::max( max_idle_time/20, 10);
		m_eventloop = strus::createCurlEventLoop(
					m_requestLogger,
					timeout, nofDelegateTotalConnections, nofDelegateHostConnections,
					m_errorhnd);
		if (!m_eventloop->start())
		{
			throw std::runtime_error( _TXT("failed to start background process for garbage collector"));
		}
		std::string script_dir;
		std::string schema_dir;

		m_requestHandler = strus::createWebRequestHandler(
					m_eventloop, m_requestLogger, m_html_head,
					m_put_configdir, script_dir, schema_dir, m_identifier, m_port,
					m_pretty_print, max_idle_time, transactions_per_second, m_errorhnd);
		if (!m_requestHandler) throw std::runtime_error( m_errorhnd->fetchError());

		WebRequestAnswer answer;
		if (!m_requestHandler->init( configstr.c_str(), configstr.size(), answer))
		{
			const char* msg = answer.errorStr() ? answer.errorStr() : strus::errorCodeToString( answer.appErrorCode());
			throw strus::runtime_error( _TXT("failed to initialize request handler: %s"), msg);
		}
	}
	catch (const std::bad_alloc& err)
	{
		throw std::bad_alloc();
	}
	catch (const std::runtime_error& err)
	{
		if (m_errorhnd)
		{
			const char* msg = m_errorhnd->fetchError();
			BOOSTER_ERROR( DefaultConstants::PACKAGE()) << msg;
		}
		throw strus::runtime_error(_TXT("error initialising service closure: %s"), err.what());
	}
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

void ServiceClosure::mount_applications()
{
	if (m_service) m_service->applications_pool().mount( cppcms::applications_factory<ApplicationImpl>( this));
}

void ServiceClosure::processWaitingRequests()
{
	strus::unique_lock lock( m_mutex_waitingContextList);
	std::vector<WebRequestWaitingContext> waitingContextList_new;
	std::vector<WebRequestWaitingContext>::iterator ci = m_waitingContextList.begin(), ce = m_waitingContextList.end();
	for (; ci != ce; ++ci)
	{
		RequestContextImpl appcontext( ci->httpContext(), this);
		if (ci->requestContext()->execute())
		{
			strus::WebRequestAnswer answer = ci->requestContext()->getAnswer();
			if (answer.ok())
			{
				appcontext.report_answer( answer, true/*do_reply_content*/);
			}
			else
			{
				appcontext.report_error( answer.httpStatus(), answer.appErrorCode(), answer.errorStr());
			}
		}
		else
		{
			waitingContextList_new.push_back( *ci);
		}
	}
	std::swap( m_waitingContextList, waitingContextList_new);
}

void ServiceClosure::logInfoMessages()
{
	std::vector<std::string> infomsgs = m_errorhnd->fetchInfo();
	std::vector<std::string>::const_iterator mi = infomsgs.begin(), me = infomsgs.end();
	for (; mi != me; ++mi)
	{
		BOOSTER_INFO( DefaultConstants::PACKAGE() ) << *mi;
	}
}



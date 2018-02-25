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
#include "defaultContants.hpp"
#include "applicationImpl.hpp"
#include "applicationMessageBuf.hpp"
#include "serviceClosure.hpp"
#include "strus/lib/webrequest.hpp"
#include "strus/lib/error.hpp"
#include "strus/webRequestAnswer.hpp"
#include "strus/base/local_ptr.hpp"
#include "internationalization.hpp"
#include "defaultContants.hpp"
#include "versionWebService.hpp"
#include "strus/versionStorage.hpp"
#include "strus/versionModule.hpp"
#include "strus/versionRpc.hpp"
#include "strus/versionTrace.hpp"
#include "strus/versionAnalyzer.hpp"
#include "strus/versionBase.hpp"
#include "strus/base/string_format.hpp"
#include <cppcms/application.h>
#include <cppcms/http_response.h>
#include <cppcms/http_content_type.h>
#include <cppcms/http_context.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <booster/log.h>
#include <cstring>
#include <cstdarg>
#include <sstream>
#include <iostream>

#undef STRUS_LOWLEVEL_DEBUG

Application::Application( cppcms::service& service_, ServiceClosure* serviceClosure_)
		:cppcms::application(service_),m_service(serviceClosure_)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "CREATE cppcms::application" << std::endl;
#endif
	init_dispatchers();
}

#define CATCH_EXEC_ERROR(REQUEST_TYPE) \
	catch (const std::runtime_error& err)\
	{\
		report_error_fmt( 500, -1, _TXT("error in %s request: %s"), REQUEST_TYPE, err.what());\
	}\
	catch (const std::bad_alloc& err)\
	{\
		report_error_fmt( 500, -1, _TXT("out of memory in %s request"), REQUEST_TYPE);\
	}\
	catch (...)\
	{\
		report_error_fmt( 500, -1, _TXT("uncaught error in %s request"), REQUEST_TYPE);\
	}

void Application::response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize)
{
	char content_header[ 256];
	if ((int)sizeof(content_header)-1 <= std::snprintf( content_header, sizeof(content_header), "%s; charset=%s", doctype, charset))
	{
		report_error( 500/*internal server error*/, -1, _TXT("bad content header"));
	}
	else
	{
		response().set_content_header( content_header);
		response().out().write( blob, blobsize);
	}
}

void Application::response_content( const strus::WebRequestContent& content)
{
	response_content( content.charset(), content.doctype(), content.str(), content.len());
}

void Application::report_fatal()
{
	BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status 500) FATAL";
	response().status( 500/*application error*/);
}

void Application::report_error( int httpstatus, int apperrorcode, const char* message_)
{
	const char* message = message_?message_:"";
	if (apperrorcode > 0)
	{
		BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ", apperr " << apperrorcode << ") " << message;
	}
	else
	{
		BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ") " << message;
	}
	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept(), m_service->html_head());
	if (msgbuf.doctype() == strus::WebRequestContent::Unknown)
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE())
			<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; no content returned"),
							msgbuf.http_accept(), msgbuf.http_accept_charset());
	}
	else
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE())
			<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
							msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());
		response_content( msgbuf.error( httpstatus, apperrorcode, message));
	}
	response().status( httpstatus);
}

void Application::report_error_fmt( int httpstatus, int apperrorcode, const char* fmt, ...)
{
	char buf[ 1024];
	va_list ap;
	va_start(ap, fmt);
	std::vsnprintf( buf, sizeof(buf), fmt, ap);
	report_error( httpstatus, apperrorcode, buf);
	va_end (ap);
}

void Application::report_ok( const char* status, int httpstatus, const char* message)
{
	BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << status << " " << httpstatus << ") " << message;

	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept(), m_service->html_head());
	BOOSTER_DEBUG( DefaultConstants::PACKAGE())
		<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
						msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());

	response_content( msgbuf.info( "ok", message));
	response().status( httpstatus);
}

void Application::report_answer( const strus::WebRequestAnswer& answer)
{
	if (answer.errorstr() || answer.apperror())
	{
		report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
	}
	else
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << answer.httpstatus() << ") ok";
		response().status( answer.httpstatus());
		response_content( answer.content());
	}
}

void Application::not_found_404()
{
	report_error( 404, -1, _TXT( "Not found"));
}

void Application::exec_put_config( std::string contexttype, std::string contextname, std::string schema)
{
	if (!handle_preflight_cors() || !check_request_method("POST")) return;
	put_config_internal( contexttype, contextname, schema);
}

void Application::exec_post3( std::string contexttype, std::string contextname, std::string schemaname)
{
	if (!handle_preflight_cors() || !check_request_method("POST")) return;
	exec_content_internal( ContentExec, contexttype, contextname, schemaname, std::string()/*no argument*/);
}

void Application::exec_post4( std::string contexttype, std::string contextname, std::string schemaname, std::string argument)
{
	if (!handle_preflight_cors() || !check_request_method("POST")) return;
	exec_content_internal( ContentExec, contexttype, contextname, schemaname, argument);
}

void Application::exec_debug_post3( std::string contexttype, std::string contextname, std::string schemaname)
{
	if (!handle_preflight_cors() || !check_request_method("POST")) return;
	exec_content_internal( ContentDebug, contexttype, contextname, schemaname, std::string()/*no argument*/);
}

void Application::exec_debug_post4( std::string contexttype, std::string contextname, std::string schemaname, std::string argument)
{
	if (!handle_preflight_cors() || !check_request_method("POST")) return;
	exec_content_internal( ContentDebug, contexttype, contextname, schemaname, argument);
}

void Application::exec_quit()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		report_ok( "ok", 200, _TXT("service to shutdown"));
		m_service->shutdown();
	}
	CATCH_EXEC_ERROR("quit");
}

void Application::exec_ping()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;	
		report_ok( "ok", 200, _TXT("service is up and running"));
	}
	CATCH_EXEC_ERROR("ping");
}

void Application::exec_list( std::string path)
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	exec_get_internal( GetList, path);
}

void Application::exec_list0()
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	exec_get_internal( GetList, std::string());
}

void Application::exec_view( std::string path)
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	exec_get_internal( GetView, std::string());
}

void Application::exec_view0()
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	exec_get_internal( GetView, std::string());
}

void Application::exec_version( std::string component)
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;

	const char* versionstr = 0;
	if (component == "webservice")		{versionstr = STRUS_WEBSERVICE_VERSION_STRING; }
	else if (component == "module")		{versionstr = STRUS_MODULE_VERSION_STRING;}
	else if (component == "rpc")		{versionstr = STRUS_RPC_VERSION_STRING;}
	else if (component == "trace")		{versionstr = STRUS_TRACE_VERSION_STRING;}
	else if (component == "analyzer")	{versionstr = STRUS_ANALYZER_VERSION_STRING;}
	else if (component == "storage")	{versionstr = STRUS_STORAGE_VERSION_STRING;}
	else if (component == "base")		{versionstr = STRUS_BASE_VERSION_STRING;}
	report_ok( "ok", 200, versionstr);
}

void Application::exec_version0()
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	report_ok( "ok", 200, STRUS_WEBSERVICE_VERSION_STRING);
}

bool Application::handle_preflight_cors()
{
	if (!m_service->cors_enabled())
	{
		// no CORS configured
		return true;
	}
	std::string origin = request( ).getenv( "HTTP_ORIGIN" );
	if (origin.empty()) return true; // ... no Origin header, let's assume we can continue

	if (m_service->has_preflight_cors_origin( origin))
	{
		// though Access-Control-Allow-Origin should allow a
		// list of space separated hosts, in practive only
		// echoing the origin Origin host works
		response( ).content_type( "application/json" );
		response( ).set_header( "Access-Control-Allow-Method", "GET, POST" );
		response( ).set_header( "Access-Control-Allow-Origin", origin );
		response( ).set_header( "Access-Control-Allow-Headers", "content-type" );
		response( ).set_header( "Access-Control-Max-Age", m_service->cors_age());
		return true;
	}
	return false;
}

bool Application::check_request_method( const char* type)
{
	if (request().request_method() != type)
	{
		report_error_fmt( 405/*Method Not Allowed*/, 0, _TXT("expected HTTP method '%s'"), type);
		return false;
	}
	return true;
}

bool Application::test_request_method( const char* type)
{
	return (request().request_method() == type);
}

void Application::put_config_internal( const std::string& contexttype, const std::string& contextname, const std::string& schemaname)
{
	try
	{
		strus::WebRequestAnswer answer;
		std::string http_accept_charset = request().http_accept_charset();
		std::string http_accept = request().http_accept();
		strus::unique_ptr<strus::WebRequestContextInterface> ctx(
			m_service->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), answer));
		if (!ctx.get())
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
		cppcms::http::content_type content_type = request().content_type_parsed();
		std::pair<void*,size_t> content_data = request().raw_post_data();
		std::string doctype = content_type.media_type();
		std::string charset = content_type.charset();
	
		strus::WebRequestContent content( charset.c_str(), doctype.c_str(), (const char*)content_data.first, content_data.second);
	
		BOOSTER_DEBUG( DefaultConstants::PACKAGE())
			<< strus::string_format( _TXT( "loading configuration %s %s (schema %s)"),
				contexttype.c_str(), contextname.c_str(), schemaname.c_str());
	
		if (!m_service->requestHandler()->loadConfiguration(
			contexttype.c_str(), contextname.c_str(), schemaname.c_str(), content, answer))
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
		}
		else if (!m_service->requestHandler()->storeConfiguration(
			contexttype.c_str(), contextname.c_str(), schemaname.c_str(), content, answer))
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
		}
		report_answer( answer);
	}
	CATCH_EXEC_ERROR("PUT")
}

void Application::exec_content_internal( ContentMethod method, const std::string& contexttype, const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	try
	{
		strus::WebRequestAnswer answer;
		std::string http_accept_charset = request().http_accept_charset();
		std::string http_accept = request().http_accept();
		strus::unique_ptr<strus::WebRequestContextInterface> ctx(
			m_service->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), answer));
		if (!ctx.get())
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
		cppcms::http::content_type content_type = request().content_type_parsed();
		std::pair<void*,size_t> content_data = request().raw_post_data();
		std::string doctype = content_type.media_type();
		std::string charset = content_type.charset();

		strus::WebRequestContent content( charset.c_str(), doctype.c_str(), (const char*)content_data.first, content_data.second);

		bool rt = false;
		switch (method)
		{
			case ContentDebug: rt = ctx->debugContent( contexttype.c_str(), contextname.c_str(), schemaname.c_str(), content, answer); break;
			case ContentExec: rt = ctx->executeContent( contexttype.c_str(), contextname.c_str(), schemaname.c_str(), content, answer); break;
		}
		if (rt)
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
									http_accept.c_str(), http_accept_charset.c_str(), doctype.c_str(), charset.c_str());
			report_answer( answer);
		}
		else
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
	}
	CATCH_EXEC_ERROR("POST")
}

void Application::exec_get_internal( GetMethod method, const std::string& path)
{
	try
	{
		if( !handle_preflight_cors()) return;

		strus::WebRequestAnswer answer;
		std::vector<std::string> result;
		std::string http_accept_charset = request().http_accept_charset();
		std::string http_accept = request().http_accept();
		strus::unique_ptr<strus::WebRequestContextInterface> ctx(
			m_service->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), answer));
		if (!ctx.get())
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}

		bool rt = false;
		switch (method)
		{
			case GetList: rt = ctx->executeList( path.c_str(), answer); break;
			case GetView: rt = ctx->executeView( path.c_str(), answer); break;
		}
		if (rt)
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
									http_accept.c_str(), http_accept_charset.c_str(),
									answer.content().doctype(), answer.content().charset());
			report_answer( answer);
		}
		else
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
	}
	CATCH_EXEC_ERROR( "GET")
}

static std::string urlRegex( const char* id, int nn, bool more)
{
	if (more)
	{
		if (nn == 0) throw std::runtime_error(_TXT("bad URL map definition"));
		--nn;
	}
	std::string rt = id[0] ? strus::string_format( "/%s", id) : std::string();
	for (; nn; --nn) rt.append( "/(\\w+)");
	if (more) rt.append( "/(.*)");
	return rt;
}

void Application::urlmap( const char* dir, UrlHandlerMethod0 handler0)
{
	dispatcher().assign( urlRegex( dir, 0, false), handler0, this);
}
void Application::urlmap( const char* dir, UrlHandlerMethod1 handler1, bool more)
{
	dispatcher().assign( urlRegex( dir, 1, more), handler1, this, 1);
}
void Application::urlmap( const char* dir, UrlHandlerMethod2 handler2, bool more)
{
	dispatcher().assign( urlRegex( dir, 2, more), handler2, this, 1, 2);
}
void Application::urlmap( const char* dir, UrlHandlerMethod3 handler3, bool more)
{
	dispatcher().assign( urlRegex( dir, 3, more), handler3, this, 1, 2, 3);
}
void Application::urlmap( const char* dir, UrlHandlerMethod4 handler4, bool more)
{
	dispatcher().assign( urlRegex( dir, 4, more), handler4, this, 1, 2, 3, 4);
}

void Application::init_dispatchers()
{
	urlmap( "ping",		&Application::exec_ping);
	urlmap( "version",	&Application::exec_version0);
	urlmap( "version",	&Application::exec_version);
	urlmap( "view",		&Application::exec_view, true);
	urlmap( "view",		&Application::exec_view0);
	urlmap( "list",		&Application::exec_list, true);
	urlmap( "list",		&Application::exec_list0);
	if (m_service->quit_enabled())
	{
		urlmap( "quit", &Application::exec_quit);
	}
	urlmap( "config",	&Application::exec_put_config);
	urlmap( "debug",	&Application::exec_debug_post4);
	urlmap( "debug",	&Application::exec_debug_post3);
	urlmap( "",		&Application::exec_post4);
	urlmap( "",		&Application::exec_post3);
	urlmap( ".*",		&Application::not_found_404);
}



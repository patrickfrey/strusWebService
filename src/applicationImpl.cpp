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
#include "applicationRequestBuf.hpp"
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

void Application::response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize)
{
	char content_header[ 256];
	if ((int)sizeof(content_header) < std::snprintf( content_header, sizeof(content_header), "%s; charset=%s", doctype, charset))
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
	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept());
	BOOSTER_DEBUG( DefaultConstants::PACKAGE())
		<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
						msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());

	response_content( msgbuf.error( httpstatus, apperrorcode, message));
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

	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept());
	BOOSTER_DEBUG( DefaultConstants::PACKAGE())
		<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
						msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());

	response_content( msgbuf.info( "ok", message));
	response().status( httpstatus);
}

void Application::report_ok( const char* status, int httpstatus, const char* rootelem, const std::map<std::string,std::string>& message)
{
	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept());

	BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << status << " " << httpstatus << ") " << rootelem;
	BOOSTER_DEBUG( DefaultConstants::PACKAGE())
		<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
						msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());

	response_content( msgbuf.info( rootelem, message));
	response().status( httpstatus);
}

void Application::report_ok( const char* status, int httpstatus, const char* rootelem, const char* listelem, const std::vector<std::string>& message)
{
	ApplicationMessageBuf msgbuf( request().http_accept_charset(), request().http_accept());

	BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << status << " " << httpstatus << ") " << rootelem;
	BOOSTER_DEBUG( DefaultConstants::PACKAGE())
		<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
						msgbuf.http_accept(), msgbuf.http_accept_charset(), msgbuf.doctypename(), msgbuf.charset());

	response_content( msgbuf.info( rootelem, listelem, message));
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

void Application::exec_post_config( std::string new_context, std::string from_context, std::string schema)
{
	if( !handle_preflight_cors()) return;

	report_error( 404, -1, _TXT( "Not found"));
	// TODO Implement, not implemented yet
}

void Application::exec_post2( std::string contextname, std::string schemaname)
{
	exec_post_internal( contextname, schemaname, std::string()/*no argument*/);
}

void Application::exec_post3( std::string contextname, std::string schemaname, std::string argument)
{
	exec_post_internal( contextname, schemaname, argument);
}

void Application::exec_debug_post2( std::string contextname, std::string schemaname)
{
	exec_debug_post_internal( contextname, schemaname, std::string()/*no argument*/);
}

void Application::exec_debug_post3( std::string contextname, std::string schemaname, std::string argument)
{
	exec_debug_post_internal( contextname, schemaname, argument);
}

std::string Application::request_role() const
{
	return "nobody";
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

void Application::exec_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("POST")) return;
	
		ApplicationRequestBuf requestBuf(
			m_service, contextname, schemaname, argument,
			request().http_accept_charset(), request().http_accept(), request_role(),
			context().request().content_type_parsed(),
			context().request().raw_post_data());
	
		if (requestBuf.ok())
		{
			requestBuf.execute();
		}
		if (requestBuf.ok())
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
									requestBuf.http_accept(), requestBuf.http_accept_charset(),
									requestBuf.answer().content().doctype(), requestBuf.answer().content().charset());
			report_answer( requestBuf.answer());
		}
		else
		{
			report_error( requestBuf.answer().httpstatus(), requestBuf.answer().apperror(), requestBuf.answer().errorstr());
		}
	}
	CATCH_EXEC_ERROR("POST")
}

void Application::exec_debug_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("POST")) return;
	
		ApplicationRequestBuf requestBuf(
			m_service, contextname, schemaname, argument,
			request().http_accept_charset(), request().http_accept(), request_role(),
			context().request().content_type_parsed(),
			context().request().raw_post_data());
	
		if (requestBuf.ok())
		{
			requestBuf.debug();
		}
		if (requestBuf.ok())
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'; decide for content type '%s; charset=%s'"),
									requestBuf.http_accept(), requestBuf.http_accept_charset(),
									requestBuf.answer().content().doctype(), requestBuf.answer().content().charset());
			report_answer( requestBuf.answer());
		}
		else
		{
			report_error( requestBuf.answer().httpstatus(), requestBuf.answer().apperror(), requestBuf.answer().errorstr());
		}
	}
	CATCH_EXEC_ERROR("POST")
}

void Application::exec_quit()
{
	try
	{
		if( !handle_preflight_cors()) return;
	
		report_ok( "ok", 200, _TXT("service to shutdown"));
		m_service->shutdown();
	}
	CATCH_EXEC_ERROR("quit")
}

void Application::exec_ping()
{
	try
	{
		if( !handle_preflight_cors()) return;
	
		report_ok( "ok", 200, _TXT("service is up and running"));
	}
	CATCH_EXEC_ERROR("ping")
}

void Application::exec_version()
{
	try
	{
		std::map<std::string,std::string> msg;
		msg[ "webservice"] = STRUS_WEBSERVICE_VERSION_STRING;
		msg[ "module"] = STRUS_MODULE_VERSION_STRING;
		msg[ "rpc"] = STRUS_RPC_VERSION_STRING;
		msg[ "trace"] = STRUS_TRACE_VERSION_STRING;
		msg[ "analyzer"] = STRUS_ANALYZER_VERSION_STRING;
		msg[ "storage"] = STRUS_STORAGE_VERSION_STRING;
		msg[ "base"] = STRUS_BASE_VERSION_STRING;
		report_ok( "ok", 200, "version", msg);
	}
	CATCH_EXEC_ERROR("version")
}

static std::vector<std::string> split_path( const std::string& path)
{
	std::vector<std::string> rt;
	std::istringstream pathitr( path);
	std::string pathelem;
	while (std::getline( pathitr, pathelem, '/'))
	{
		if (!pathelem.empty()) rt.push_back( pathelem);
	}
	return rt;
}

void Application::exec_list( std::string path)
{
	try
	{
		if( !handle_preflight_cors()) return;

		std::vector<std::string> pt = split_path( path);
		std::string role = request_role();
		std::vector<std::string> result;
		strus::WebRequestAnswer answer;

		if (m_service->requestHandler()->executeList( pt, role.c_str(), result, answer))
		{
			report_ok( "ok", 200/*http status*/, "list", "elem", result);
		}
		else
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
		}
	}
	CATCH_EXEC_ERROR("list")
}

void Application::exec_view( std::string path)
{
	try
	{
		if( !handle_preflight_cors()) return;

		std::vector<std::string> pt = split_path( path);
		std::string role = request_role();
		std::string http_accept_charset = request().http_accept_charset();
		std::string http_accept = request().http_accept();
		std::vector<std::string> result;
		strus::WebRequestAnswer answer;

		if (m_service->requestHandler()->executeView( pt, role.c_str(), http_accept_charset.c_str(), http_accept.c_str(), answer))
		{
			report_answer( answer);
		}
		else
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
		}
	}
	CATCH_EXEC_ERROR("view")
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

void Application::urlmap( const char* dir, UrlHandlerMethod3 handler, bool more)
{
	dispatcher().assign( urlRegex( dir, 3, more), handler, this, 1, 2, 3);
}
void Application::urlmap( const char* dir, UrlHandlerMethod2 handler, bool more)
{
	dispatcher().assign( urlRegex( dir, 2, more), handler, this, 1, 2);
}
void Application::urlmap( const char* dir, UrlHandlerMethod1 handler, bool more)
{
	dispatcher().assign( urlRegex( dir, 1, more), handler, this, 1);
}
void Application::urlmap( const char* dir, UrlHandlerMethod0 handler)
{
	dispatcher().assign( urlRegex( dir, 0, false), handler, this);
}

void Application::init_dispatchers()
{
	urlmap( "ping",		&Application::exec_ping);
	urlmap( "version",	&Application::exec_version);
	urlmap( "view",		&Application::exec_view, true);
	urlmap( "list",		&Application::exec_list, true);
	if (m_service->quit_enabled())
	{
		urlmap( "quit", &Application::exec_quit);
	}
	urlmap( "config",	&Application::exec_post_config);
	urlmap( "debug",	&Application::exec_debug_post3);
	urlmap( "debug",	&Application::exec_debug_post2);
	urlmap( "",		&Application::exec_post3);
	urlmap( "",		&Application::exec_post2);
	urlmap( ".*",		&Application::not_found_404);
}



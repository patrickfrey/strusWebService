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

#define STRUS_LOWLEVEL_DEBUG

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
	char const* message = message_?message_:"";
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

void Application::exec_post_config( const std::string& new_context, const std::string& from_context, const std::string& schema)
{
	if( !handle_preflight_cors()) return;

	report_error( 404, -1, _TXT( "Not found"));
	// TODO Implement, not implemented yet
}

void Application::exec_post2( const std::string& contextname, const std::string& schemaname)
{
	exec_post_internal( contextname, schemaname, std::string()/*no argument*/);
}

void Application::exec_post3( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	exec_post_internal( contextname, schemaname, argument);
}

void Application::debug_post2( const std::string& contextname, const std::string& schemaname)
{
	exec_post_internal( contextname, schemaname, std::string()/*no argument*/);
}

void Application::debug_post3( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	exec_post_internal( contextname, schemaname, argument);
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

void Application::exec_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	if( !handle_preflight_cors()) return;

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

void Application::debug_post_internal( const std::string& contextname, const std::string& schemaname, const std::string& argument)
{
	if( !handle_preflight_cors()) return;

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

void Application::exec_quit()
{
	if( !handle_preflight_cors()) return;

	report_ok( "ok", 200, _TXT("service to shutdown"));
	m_service->shutdown();
}

void Application::exec_ping()
{
	if( !handle_preflight_cors()) return;

	report_ok( "ok", 200, _TXT("service is up and running"));
}

void Application::exec_version()
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

void Application::exec_help()
{
	if( !handle_preflight_cors()) return;

	report_error( 404, -1, _TXT( "Not found"));
	// TODO Implement, not implemented yet
}

void Application::init_dispatchers()
{
	dispatcher( ).assign( "/ping", &Application::exec_ping, this);
	dispatcher( ).assign( "/version", &Application::exec_version, this);
	dispatcher( ).assign( "/help", &Application::exec_help, this);
	if (m_service->quit_enabled())
	{
		dispatcher( ).assign( "/quit", &Application::exec_quit, this);
	}
	dispatcher().map( "POST", "/config/(\\w+)/(\\w+)/(\\w+)", &Application::exec_post_config, this, 1, 2, 3);
	dispatcher().map( "POST", "/debug/(\\w+)/(\\w+)/(\\w+)", &Application::debug_post3, this, 1, 2, 3);
	dispatcher().map( "POST", "/debug/(\\w+)/(\\w+)", &Application::debug_post2, this, 1, 2);
	dispatcher().map( "POST", "/(\\w+)/(\\w+)/(\\w+)", &Application::exec_post3, this, 1, 2, 3);
	dispatcher().map( "POST", "/(\\w+)/(\\w+)", &Application::exec_post2, this, 1, 2);
	dispatcher().map( ".*", &Application::not_found_404, this);
}



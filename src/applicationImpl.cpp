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
#include "serviceClosure.hpp"
#include "pathIter.hpp"
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

using namespace strus::webservice;

Application::Application( cppcms::service& service_, ServiceClosure* serviceClosure_)
		:cppcms::application(service_),m_service(serviceClosure_)
{
	init_dispatchers();
}

#define CATCH_EXEC_ERROR() \
	catch (const std::runtime_error& err)\
	{\
		report_error_fmt( 500, -1, _TXT("error in request: %s"), err.what());\
	}\
	catch (const std::bad_alloc& err)\
	{\
		report_error_fmt( 500, -1, _TXT("out of memory in request"));\
	}\
	catch (...)\
	{\
		report_error_fmt( 500, -1, _TXT("uncaught error in request"));\
	}

void Application::response_content_header( const char* charset, const char* doctype, std::size_t blobsize)
{
	char content_header[ 256];
	if ((int)sizeof(content_header)-1 <= std::snprintf( content_header, sizeof(content_header), "%s; charset=%s", doctype, charset))
	{
		report_error( 500/*internal server error*/, -1, _TXT("bad content header"));
	}
	else
	{
		response().content_length( blobsize);
		response().set_content_header( content_header);
	}
}

void Application::response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize)
{
	response_content_header( charset, doctype, blobsize + 1/*'\n'*/);
	response().out().write( blob, blobsize);
	response().out() << "\n";
}

void Application::response_content( const strus::WebRequestContent& content, bool with_content)
{
	if (with_content && !content.empty())
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE())
			<< strus::string_format( _TXT("response content type '%s', charset '%s'"), content.doctype(), content.charset());
		response_content( content.charset(), content.doctype(), content.str(), content.len());
	}
	else
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE()) << _TXT("no response content");
	}
}

void Application::response_message( const char* messagetype, const char* messagestr)
{
	if (messagetype)
	{
		response().erase_header( messagetype);
		if (messagestr)
		{
			response().set_header( messagetype, messagestr);
		}
	}
}

void Application::report_error( int httpstatus, int apperrorcode, const char* message)
{
	if (message)
	{
		response().status( httpstatus);
		if (apperrorcode > 0)
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ", apperr " << apperrorcode << ") " << message;
		}
		else
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ") " << message;
		}
		response().out() << message << std::endl << std::flush;
	}
	else
	{
		response().status( httpstatus);
		if (apperrorcode > 0)
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ", apperr " << apperrorcode << ")";
		}
		else
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << "(status " << httpstatus << ")";
		}
		response().out() << response().status_to_string( httpstatus) << std::endl << std::flush;
	}
	response().finalize();
}

void Application::report_error_fmt( int httpstatus, int apperrorcode, const char* fmt, ...)
{
	char buf[ 1024];
	va_list ap;
	va_start(ap, fmt);
	std::size_t len = std::vsnprintf( buf, sizeof(buf), fmt, ap);
	if (len >= sizeof(buf)) buf[ sizeof(buf)-1] = 0;
	report_error( httpstatus, apperrorcode, buf);
	va_end (ap);
	response().finalize();
}

void Application::report_ok( const char* status, int httpstatus, const char* message)
{
	BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << status << " " << httpstatus << ") " << message;

	response().status( httpstatus, message);
	response().finalize();
}

void Application::report_answer( const strus::WebRequestAnswer& answer, bool with_content)
{
	if (answer.errorstr() || answer.apperror())
	{
		report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
	}
	else if (answer.messagetype() && answer.messagestr())
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << answer.messagetype() << "=\"" << answer.messagestr() << "\" (status " << answer.httpstatus() << ") OK";
		response().status( answer.httpstatus());
		response_message( answer.messagetype(), answer.messagestr());
	}
	else
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << answer.httpstatus() << ") OK";
		response().status( answer.httpstatus());
		response_content( answer.content(), with_content);
	}
	response().finalize();
}

void Application::exec_quit()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		report_ok( "OK", 200, _TXT("service to shutdown"));
		m_service->shutdown();
	}
	CATCH_EXEC_ERROR();
}

void Application::exec_ping()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;	
		report_message( _TXT("service is up and running"));
	}
	CATCH_EXEC_ERROR();
}

void Application::exec_version( std::string component)
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;

	const char* versionstr = 0;
	if (component == "webservice")		{versionstr = STRUS_WEBSERVICE_VERSION_STRING;}
	else if (component == "module")		{versionstr = STRUS_MODULE_VERSION_STRING;}
	else if (component == "rpc")		{versionstr = STRUS_RPC_VERSION_STRING;}
	else if (component == "trace")		{versionstr = STRUS_TRACE_VERSION_STRING;}
	else if (component == "analyzer")	{versionstr = STRUS_ANALYZER_VERSION_STRING;}
	else if (component == "storage")	{versionstr = STRUS_STORAGE_VERSION_STRING;}
	else if (component == "base")		{versionstr = STRUS_BASE_VERSION_STRING;}
	report_message( versionstr);
}

void Application::exec_version0()
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	report_message( STRUS_WEBSERVICE_VERSION_STRING);
}

void Application::exec_service_identifier()
{
	if (!handle_preflight_cors() || !check_request_method("GET")) return;
	report_message( m_service->identifier());
}

bool Application::handle_preflight_cors()
{
	if (!m_service->cors_enabled())
	{
		// no CORS configured
		return true;
	}
	std::string origin = request().getenv( "HTTP_ORIGIN" );
	if (origin.empty()) return true; // ... no Origin header, let's assume we can continue

	if (m_service->has_preflight_cors_origin( origin))
	{
		// though Access-Control-Allow-Origin should allow a
		// list of space separated hosts, in practive only
		// echoing the origin Origin host works
		response().content_type( "application/json" );
		response().set_header( "Access-Control-Allow-Method", "GET, POST" );
		response().set_header( "Access-Control-Allow-Origin", origin );
		response().set_header( "Access-Control-Allow-Headers", "content-type" );
		response().set_header( "Access-Control-Max-Age", m_service->cors_age());
		return true;
	}
	return false;
}

std::string Application::debug_request_description()
{
	std::string remote_host = request().remote_host();
	std::string method = request().request_method();
	std::string path_info = request().path_info();
	return strus::string_format(_TXT("Method %s path '%s' from '%s'"), method.c_str(), path_info.c_str(), remote_host.c_str());
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

static std::string json_value_encode( const std::string& value)
{
	std::string rt;
	char const* vi = value.c_str();
	const char* ve = vi+value.size();
	char const* entity = 0;
	while (vi != ve)
	{
		char const* start = vi;
		for (; vi != ve; ++vi)
		{
			switch (*vi)
			{
				case '\n': entity = "n"; break;
				case '\r': entity = "r"; break;
				case '\b': entity = "b"; break;
				case '\f': entity = "f"; break;
				case '\t': entity = "t"; break;
				case '"': entity = "\\\""; break;
				case '\\': entity = "\\\\"; break;
				default: continue;
			}
			break;
		}
		rt.append( vi, vi-start);
		if (entity)
		{
			rt.append( vi, entity);
			entity = 0;
			++vi;
		}
	}
	return rt;
}

static std::string form_tojson( const cppcms::http::request::form_type& form)
{
	std::string content = "{\n\"get\":{";
	cppcms::http::request::form_type::const_iterator fi = form.begin(), fe = form.end();
	for (int fidx=0; fi != fe; ++fi,++fidx)
	{
		if (fidx) content.push_back(',');
		content.append("\n\t\"");
		content.append( fi->first);
		content.append("\":");

		cppcms::http::request::form_type::const_iterator fn = fi;
		++fn;
		if (fn != fe && fn->first == fi->first)
		{
			for (; fn != fe && fn->first == fi->first; ++fn){}
			content.append(" [");
			for (int eidx=0; fi != fn; ++fi,++eidx)
			{
				if (eidx) content.push_back(',');
				content.push_back('\"');
				content.append( json_value_encode( fi->second));
				content.push_back('\"');
			}
			content.append("]");
			--fi;//... compensate outer loop iterator increment in this case where we already are on the next element
		}
		else
		{
			content.push_back('\"');
			content.append( json_value_encode( fi->second));
			content.push_back('\"');
		}
	}
	content.append( "\n}}\n");
	return content;
}

void Application::report_message( const std::string& message)
{
	strus::WebRequestAnswer answer;
	std::string http_accept_charset = request().http_accept_charset();
	std::string http_accept = request().http_accept();
	std::string html_base_href;
	if (!m_service->http_server_url().empty())
	{
		html_base_href = m_service->http_server_url();
	}
	else
	{
		html_base_href = std::string("http://") + request().http_host() + "/";
	}
	strus::unique_ptr<strus::WebRequestContextInterface> ctx(
		m_service->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), html_base_href.c_str(), answer));
	if (!ctx.get())
	{
		report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
	}
	else if (ctx->getMessageAnswer( message, answer))
	{
		BOOSTER_DEBUG( DefaultConstants::PACKAGE())
			<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'"), http_accept.c_str(), http_accept_charset.c_str());
		report_answer( answer, true/*do_reply_content*/);
	}
	else
	{
		report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
	}
}

void Application::exec_request( std::string path)
{
	try
	{
		// Set default locale:
		context().locale( "en_US.UTF-8"); 

		// Interpret request and create request context data:
		bool do_reply_content = true;
		std::string request_method = request().request_method();
		if (request_method == "HEAD")
		{
			do_reply_content = false;
			request_method = "GET";
		}
		BOOSTER_DEBUG( DefaultConstants::PACKAGE()) << debug_request_description();

		strus::WebRequestAnswer answer;
		std::string http_accept_charset = request().http_accept_charset();
		std::string http_accept = request().http_accept();
		std::string html_base_href;
		if (!m_service->http_server_url().empty())
		{
			html_base_href = m_service->http_server_url();
		}
		else
		{
			html_base_href = std::string("http://") + request().http_host() + "/";
		}
		if (!path.empty())
		{
			html_base_href.append( path);
			if (path[ path.size()-1] != '/')
			{
				html_base_href.push_back( '/');
			}
		}
		strus::unique_ptr<strus::WebRequestContextInterface> ctx(
			m_service->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), html_base_href.c_str(), answer));
		if (!ctx.get())
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
		cppcms::http::content_type content_type;
		std::pair<void*,size_t> content_data;
		std::string doctype;
		std::string charset;
		strus::WebRequestContent content;
		std::string contentstr;

		content_data = request().raw_post_data();
		if (content_data.second == 0/*empty content*/)
		{
			if (!request().get().empty())
			{
				contentstr = form_tojson( request().get());

				content.setDoctype( "application/json");
				content.setCharset( "UTF-8");
				content.setContent( contentstr.c_str(), contentstr.size());
			}
		}
		else
		{
			if (!request().get().empty())
			{
				report_error( 400/*bad request*/, strus::ErrorCodeInvalidArgument, _TXT("mixing URL parameters passed with non empty request content"));
				return;
			}
			content_type = request().content_type_parsed();
			doctype = content_type.media_type();
			charset = content_type.charset();

			content.setDoctype( doctype.c_str());
			content.setCharset( charset.empty() ? "UTF-8" : charset.c_str());
			content.setContent( (const char*)content_data.first, content_data.second);
		}
		if (content.empty())
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
				<< strus::string_format( _TXT("%s Request without content"), request_method.c_str());
		}
		else
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
				<< strus::string_format(
					_TXT("%s Request content type '%s', charset '%s'"),
					request_method.c_str(), content.doctype(), content.charset());
		}
		// Execute request:
		if (ctx->executeRequest( request_method.c_str(), path.c_str(), content, answer))
		{
			BOOSTER_DEBUG( DefaultConstants::PACKAGE())
				<< strus::string_format( _TXT("HTTP Accept: '%s', Accept-Charset: '%s'"), http_accept.c_str(), http_accept_charset.c_str());
			report_answer( answer, do_reply_content);
		}
		else
		{
			report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
			return;
		}
	}
	CATCH_EXEC_ERROR()
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

void Application::init_dispatchers()
{
	// Define request URL map:
	urlmap( "ping",		&Application::exec_ping);
	urlmap( "version",	&Application::exec_version0);
	urlmap( "version",	&Application::exec_version);
	urlmap( "id",		&Application::exec_service_identifier);
	if (m_service->quit_enabled())
	{
		urlmap( "quit", &Application::exec_quit);
	}
	urlmap( "",		&Application::exec_request, true);
}



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
#include "requestContextImpl.hpp"
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

ApplicationImpl::ApplicationImpl( cppcms::service& service_, ServiceClosure* serviceClosure_)
		:cppcms::application(service_),m_serviceClosure(serviceClosure_)
{
	init_dispatchers();
}

#define CATCH_EXEC_ERROR() \
	catch (const std::runtime_error& err)\
	{\
		RequestContextImpl appcontext( context(), m_serviceClosure);\
		appcontext.report_error_fmt( 500, -1, _TXT("error in request: %s"), err.what());\
	}\
	catch (const std::bad_alloc& err)\
	{\
		RequestContextImpl appcontext( context(), m_serviceClosure);\
		appcontext.report_error_fmt( 500, -1, _TXT("out of memory in request"));\
	}\
	catch (...)\
	{\
		RequestContextImpl appcontext( context(), m_serviceClosure);\
		appcontext.report_error_fmt( 500, -1, _TXT("uncaught error in request"));\
	}

void ApplicationImpl::exec_quit()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		RequestContextImpl appcontext( context(), m_serviceClosure);
		appcontext.report_ok( "OK", 200, _TXT("service to shutdown"));
		m_serviceClosure->shutdown();
	}
	CATCH_EXEC_ERROR();
}

void ApplicationImpl::exec_ping()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;	
		RequestContextImpl appcontext( context(), m_serviceClosure);		
		appcontext.report_message( "reply", _TXT("service is up and running"));
	}
	CATCH_EXEC_ERROR();
}

void ApplicationImpl::exec_version( std::string component)
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		RequestContextImpl appcontext( context(), m_serviceClosure);		
	
		const char* versionstr = 0;
		if (component == "webservice")		{versionstr = STRUS_WEBSERVICE_VERSION_STRING;}
		else if (component == "module")		{versionstr = STRUS_MODULE_VERSION_STRING;}
		else if (component == "rpc")		{versionstr = STRUS_RPC_VERSION_STRING;}
		else if (component == "trace")		{versionstr = STRUS_TRACE_VERSION_STRING;}
		else if (component == "analyzer")	{versionstr = STRUS_ANALYZER_VERSION_STRING;}
		else if (component == "storage")	{versionstr = STRUS_STORAGE_VERSION_STRING;}
		else if (component == "base")		{versionstr = STRUS_BASE_VERSION_STRING;}
		appcontext.report_message( "version", versionstr);
	}
	CATCH_EXEC_ERROR();
}

void ApplicationImpl::exec_version0()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		RequestContextImpl appcontext( context(), m_serviceClosure);		
		appcontext.report_message( "version", STRUS_WEBSERVICE_VERSION_STRING);
	}
	CATCH_EXEC_ERROR();
}

void ApplicationImpl::exec_service_identifier()
{
	try
	{
		if (!handle_preflight_cors() || !check_request_method("GET")) return;
		RequestContextImpl appcontext( context(), m_serviceClosure);		
		appcontext.report_message( "name", m_serviceClosure->identifier());
	}
	CATCH_EXEC_ERROR();
}

bool ApplicationImpl::handle_preflight_cors()
{
	if (!m_serviceClosure->cors_enabled())
	{
		// no CORS configured
		return true;
	}
	std::string origin = request().getenv( "HTTP_ORIGIN" );
	if (origin.empty()) return true; // ... no Origin header, let's assume we can continue

	if (m_serviceClosure->has_preflight_cors_origin( origin))
	{
		// though Access-Control-Allow-Origin should allow a
		// list of space separated hosts, in practive only
		// echoing the origin Origin host works
		response().content_type( "application/json" );
		response().set_header( "Access-Control-Allow-Method", "GET, POST" );
		response().set_header( "Access-Control-Allow-Origin", origin );
		response().set_header( "Access-Control-Allow-Headers", "content-type" );
		response().set_header( "Access-Control-Max-Age", m_serviceClosure->cors_age());
		return true;
	}
	return false;
}

std::string ApplicationImpl::debug_request_description()
{
	std::string remote_host = request().remote_host();
	std::string method = request().request_method();
	std::string path_info = request().path_info();
	return strus::string_format(_TXT("Method %s path '%s' from '%s'"), method.c_str(), path_info.c_str(), remote_host.c_str());
}

bool ApplicationImpl::check_request_method( const char* type)
{
	if (request().request_method() != type)
	{
		RequestContextImpl appcontext( context(), m_serviceClosure);
		appcontext.report_error_fmt( 405/*Method Not Allowed*/, 0, _TXT("expected HTTP method '%s'"), type);
		return false;
	}
	return true;
}

bool ApplicationImpl::test_request_method( const char* type)
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

void ApplicationImpl::exec_request( std::string path)
{
	try
	{
		RequestContextImpl appcontext( context(), m_serviceClosure);		

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
		if (!m_serviceClosure->http_server_url().empty())
		{
			html_base_href = m_serviceClosure->http_server_url();
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
			m_serviceClosure->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), html_base_href.c_str(), answer));
		if (!ctx.get())
		{
			appcontext.report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
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
				appcontext.report_error( 400/*bad request*/, strus::ErrorCodeInvalidArgument, _TXT("mixing URL parameters passed with non empty request content"));
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
			appcontext.report_answer( answer, do_reply_content);
		}
		else
		{
			appcontext.report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
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

void ApplicationImpl::urlmap( const char* dir, UrlHandlerMethod0 handler0)
{
	dispatcher().assign( urlRegex( dir, 0, false), handler0, this);
}
void ApplicationImpl::urlmap( const char* dir, UrlHandlerMethod1 handler1, bool more)
{
	dispatcher().assign( urlRegex( dir, 1, more), handler1, this, 1);
}

void ApplicationImpl::init_dispatchers()
{
	// Define request URL map:
	urlmap( "ping",		&ApplicationImpl::exec_ping);
	urlmap( "version",	&ApplicationImpl::exec_version0);
	urlmap( "version",	&ApplicationImpl::exec_version);
	urlmap( "id",		&ApplicationImpl::exec_service_identifier);
	if (m_serviceClosure->quit_enabled())
	{
		urlmap( "quit", &ApplicationImpl::exec_quit);
	}
	urlmap( "",		&ApplicationImpl::exec_request, true);
}



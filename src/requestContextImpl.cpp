/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 * Copyright (c) 2017,2018,2019 Patrick P. Frey, Andreas Baumann, Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Context for requests disconnected from the application object (yield because of requests to other servers)
#include "requestContextImpl.hpp"
#include "serviceClosure.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/unique_ptr.hpp"
#include "strus/webRequestContextInterface.hpp"
#include <cppcms/application.h>

using namespace strus;
using namespace strus::webservice;

RequestContextImpl::RequestContextImpl( const AppContextRef& appContext_, ServiceClosure* serviceClosure_)
	:m_appContextOwnership(appContext_),m_serviceClosure(serviceClosure_)
{
	m_appContext = m_appContextOwnership.get();
}

RequestContextImpl::RequestContextImpl( AppContext& appContext_, ServiceClosure* serviceClosure_)
	:m_appContextOwnership(),m_serviceClosure(serviceClosure_)
{
	m_appContext = &appContext_;
}

void RequestContextImpl::response_content_header( const char* charset, const char* doctype, std::size_t blobsize)
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

void RequestContextImpl::response_content( const char* charset, const char* doctype, const char* blob, std::size_t blobsize)
{
	response_content_header( charset, doctype, blobsize + 1/*'\n'*/);
	response().out().write( blob, blobsize);
	response().out() << "\n";
}

void RequestContextImpl::response_content( const strus::WebRequestContent& content, bool with_content)
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

void RequestContextImpl::response_message( const char* messagetype, const char* messagestr)
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

void RequestContextImpl::report_error( int httpstatus, int apperrorcode, const char* message)
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

void RequestContextImpl::report_error_fmt( int httpstatus, int apperrorcode, const char* fmt, ...)
{
	char buf[ 1024];
	va_list ap;
	va_start(ap, fmt);
	std::size_t len = std::vsnprintf( buf, sizeof(buf), fmt, ap);
	if (len >= sizeof(buf)) buf[ sizeof(buf)-1] = 0;
	report_error( httpstatus, apperrorcode, buf);
	va_end (ap);
}

void RequestContextImpl::report_ok( const char* status, int httpstatus, const char* message)
{
	BOOSTER_DEBUG( DefaultConstants::PACKAGE() ) << "(status " << status << " " << httpstatus << ") " << message;

	response().status( httpstatus, message);
	response().finalize();
}

void RequestContextImpl::report_answer( const strus::WebRequestAnswer& answer, bool with_content)
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

void RequestContextImpl::report_message( const std::string& key, const std::string& message)
{
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
	strus::unique_ptr<strus::WebRequestContextInterface> ctx(
		m_serviceClosure->requestHandler()->createContext( http_accept_charset.c_str(), http_accept.c_str(), html_base_href.c_str(), answer));
	if (!ctx.get())
	{
		report_error( answer.httpstatus(), answer.apperror(), answer.errorstr());
	}
	else if (ctx->getMessageAnswer( key, message, answer))
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





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
/// \brief Abstraction for a server to client message of a cppcms application for strus with its data
#include "applicationMessageBuf.hpp"
#include "strus/webRequestContent.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "strus/webRequestHandlerInterface.hpp"
#include "strus/errorCodes.hpp"
#include "strus/lib/webrequest.hpp"
#include "strus/lib/error.hpp"
#include "strus/base/string_format.hpp"
#include "internationalization.hpp"
#include <map>
#include <string>
#include <sstream>
#include <iostream>

static void escJsonOutput( char* buf, std::size_t buflen)
{
	char* si = buf;
	char* se = buf + buflen;
	for (; si != se; ++si)
	{
		switch (*si)
		{
			case '\t':
			case '\n':
			case '\r':
			case '\b':
			case '\0':
			case '\\': *si = ' '; break;
			case '\"': *si = '\''; break;
			case '\1': *si = '\"'; break;
			default: break;
		}
	}
}

static bool printHtmlHeader( char* msgbuf, std::size_t msgbufsize, std::size_t& msgbufpos, const char* charset, const char* html_head)
{
	msgbufpos = std::snprintf( msgbuf, msgbufsize, "<!DOCTYPE html>\n<head>\n<head>\n<body>\n<meta>\n<charset=\"%s\">\n</meta>\n%s</head>\n<body>", charset, html_head);
	return msgbufpos+1 < msgbufsize;
}

static void printBufErrorMessage( char* msgbuf, std::size_t msgbufsize, std::size_t& msgbufpos, strus::WebRequestContent::Type doctype, const char* charset, int httpstatus, int apperrorcode, const char* message, const char* html_head)
{
	strus::ErrorCode apperr( apperrorcode > 0 ? apperrorcode : 0);
	switch (doctype)
	{
		case strus::WebRequestContent::Unknown:
			msgbuf[0] = 0;
			msgbufpos = 0;
			break;

		case strus::WebRequestContent::JSON:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "{\n\t\1status\1:%d,\n\t\1error\1: {\n\t\1code\1:%d,\n\t\1component\1:%s,\n\t\1op\1:%d,\n\t\1no\1:%d,\n\t\1msg\1:\1%s\1\n}}\n",
				httpstatus, apperrorcode,
				strus::errorComponentName( apperr.component()), (int)apperr.operation(), (int)apperr.cause(), message);
			escJsonOutput( msgbuf, (msgbufpos >= msgbufsize) ? (msgbufsize-1) : msgbufpos);
			break;

		case strus::WebRequestContent::XML:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "<?xml version=\"1.0\" encoding=\"%s\"?>\n<error><status>%d</status><code>%d<code><component>%s</component><op>%d</op><errno>%d</errno><msg>%s</msg></error>\n",
				charset, httpstatus, apperrorcode,
				strus::errorComponentName( apperr.component()), (int)apperr.operation(), (int)apperr.cause(), message);
			break;

		case strus::WebRequestContent::TEXT:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "%d %d %s\n", httpstatus, apperrorcode, message);
			break;

		case strus::WebRequestContent::HTML:
			if (!printHtmlHeader( msgbuf, msgbufsize, msgbufpos, charset, html_head))
			{
				throw std::runtime_error(_TXT("buffer overflow"));
			}
			msgbufpos += std::snprintf(
				msgbuf+msgbufpos, msgbufsize-msgbufpos, "<div class=\"error\">\n<div class=\"status\">Status: %d</div>\n<div class=\"apperr\">Application error code: %d</div>\n<div class=\"component\">Component: %s</div>\n<div class=\"operation\">Operation: %d</div>\n<div class=\"errno\">Errno: %d</div>\n<div class=\"message\">Message: %s</div>\n</div>\n</body>\n</html>\n",
				httpstatus, apperrorcode,
				strus::errorComponentName( apperr.component()), (int)apperr.operation(), (int)apperr.cause(), message);
			break;
	}
	if (msgbufpos >= msgbufsize)
	{
		throw std::runtime_error(_TXT("buffer overflow"));
	}
}

static void printBufInfoMessage( char* msgbuf, std::size_t msgbufsize, std::size_t& msgbufpos, strus::WebRequestContent::Type doctype, const char* charset, const char* status, const char* message, const char* html_head)
{
	switch (doctype)
	{
		case strus::WebRequestContent::Unknown:
			throw std::runtime_error(_TXT("unknown content type for output"));

		case strus::WebRequestContent::JSON:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "{\n\t\1status\1: \1%s\1,\n\t\1message\1:\1%s\1,\n}\n", status, message);
			escJsonOutput( msgbuf, (msgbufpos > msgbufsize) ? (msgbufsize-1) : msgbufpos);
			break;
		case strus::WebRequestContent::XML:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "<?xml version=\"1.0\" encoding=\"%s\"?>\n<done><status>%s</status><msg>%s</msg></done>\n", charset, status, message);
			break;

		case strus::WebRequestContent::TEXT:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "%s %s\n", status, message);
			break;

		case strus::WebRequestContent::HTML:
			if (!printHtmlHeader( msgbuf, msgbufsize, msgbufpos, charset, html_head))
			{
				throw std::runtime_error(_TXT("buffer overflow"));
			}
			msgbufpos += std::snprintf(
				msgbuf+msgbufpos, msgbufsize-msgbufpos, "<div class=\"info\">\n<div class=\"status\">Status: %s</div>\n<div class=\"message\">%s</div>\n</div>\n</body>\n</html>\n",
				status, message);
			break;
	}
	if (msgbufpos >= msgbufsize)
	{
		throw std::runtime_error(_TXT("buffer overflow"));
	}
}

ApplicationMessageBuf::ApplicationMessageBuf( const std::string& accepted_charset_, const std::string& accepted_doctype_, const char* html_head_)
	:m_accepted_charset(accepted_charset_),m_accepted_doctype(accepted_doctype_),m_html_head(html_head_)
{
	m_doctype = strus::selectAcceptedContentType( m_accepted_doctype.c_str());
	m_doctypename = strus::WebRequestContent::typeName( m_doctype);
	m_charset = strus::selectAcceptedCharset( m_accepted_charset.c_str());
}

strus::WebRequestContent ApplicationMessageBuf::error( int httpstatus, int apperrorcode, const char* message)
{
	std::size_t msgbufpos = 0;
	printBufErrorMessage( m_msgbuf, sizeof(m_msgbuf), msgbufpos, m_doctype, m_charset, httpstatus, apperrorcode, message, m_html_head);

	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf_conv, sizeof(m_msgbuf_conv), msglen_conv, m_msgbuf, msgbufpos);

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}

strus::WebRequestContent ApplicationMessageBuf::info( const char* status, const char* message)
{
	std::size_t msgbufpos = 0;
	printBufInfoMessage( m_msgbuf, sizeof(m_msgbuf), msgbufpos, m_doctype, m_charset, status, message, m_html_head);

	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf_conv, sizeof(m_msgbuf_conv), msglen_conv, m_msgbuf, msgbufpos);

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}



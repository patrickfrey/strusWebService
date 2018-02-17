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

static void printBufErrorMessage( char* msgbuf, std::size_t msgbufsize, std::size_t& msgbufpos, strus::WebRequestContent::Type doctype, const char* charset, int httpstatus, int apperrorcode, const char* message)
{
	strus::ErrorCode apperr( apperrorcode > 0 ? apperrorcode : 0);
	switch (doctype)
	{
		case strus::WebRequestContent::Unknown:
			throw std::runtime_error(_TXT("unknown content type for output"));

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
				msgbuf, msgbufsize, "STATUS %d\nAPPERR %d\nCOMPONENT %s\nOP %d\nERRNO %d\nMESSAGE %s\n\n",
				httpstatus, apperrorcode,
				strus::errorComponentName( apperr.component()), (int)apperr.operation(), (int)apperr.cause(), message);
			break;

		case strus::WebRequestContent::HTML:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "<html><head></head><body><h1>Error</h1>\n<ul>\n<li>Code: %d</li>\n<li>Status: %d</li>\n<li>Component: %s</li>\n<li>Operation: %d</li>\n<li>Number: %d</li>\n<li>Message: <i>%s</i></li>\n</ul>\n</body>\n</html>\n",
				httpstatus, apperrorcode,
				strus::errorComponentName( apperr.component()), (int)apperr.operation(), (int)apperr.cause(), message);
			break;
	}
	if (msgbufpos >= msgbufsize)
	{
		throw std::runtime_error(_TXT("buffer overflow"));
	}
}

static void printBufInfoMessage( char* msgbuf, std::size_t msgbufsize, std::size_t& msgbufpos, strus::WebRequestContent::Type doctype, const char* charset, const char* status, const char* message)
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
				msgbuf, msgbufsize, "STATUS %s\nMESSAGE %s\n\n", status, message);
			break;

		case strus::WebRequestContent::HTML:
			msgbufpos = std::snprintf(
				msgbuf, msgbufsize, "<html><head><title>strus request</title></head><body><h1>Done</h1>\n<ul>\n<li>Status: %s</li>\n<li>Message: %s</li>\n</ul>\n</body>\n</html>\n", status, message);
			break;
	}
	if (msgbufpos >= msgbufsize)
	{
		throw std::runtime_error(_TXT("buffer overflow"));
	}
}

static std::string dictMessage( strus::WebRequestContent::Type doctype, const char* charset, const char* rootelem, const std::map<std::string,std::string>& dict)
{
	std::ostringstream out;
	std::map<std::string,std::string>::const_iterator di = dict.begin(), de = dict.end();
	switch (doctype)
	{
		case strus::WebRequestContent::Unknown:
			throw std::runtime_error(_TXT("Unknown content type for output"));

		case strus::WebRequestContent::JSON:
			out << "{\n\1" << rootelem << "\1:{";
			for (; di != de; ++di)
			{
				out << "\n\t\1" << di->first << "\1:\1" << di->second << "\1";
			}
			out << "\n}}\n";
			break;

		case strus::WebRequestContent::XML:
			out << "<?xml version=\"1.0\" encoding=\"" << charset << "\"?>\n<" << rootelem << ">\n";
			for (; di != de; ++di)
			{
				out << "\n\t<" << di->first << ">" << di->second << "</" << di->first << ">";
			}
			out << "\n</" << rootelem << ">\n";
			break;

		case strus::WebRequestContent::TEXT:
			for (; di != de; ++di)
			{
				out << di->first << ": " << di->second << "\n";
			}
			break;

		case strus::WebRequestContent::HTML:
			out << "<html><head><title>" << rootelem << "</title></head><body><h1>" << rootelem << "</h1>\n<ul>";
			for (; di != de; ++di)
			{
				out << "\n\t<li>" << di->first << ": " << di->second << "</li>";
			}
			out << "\n</ul></body></html>\n";
			break;
	}
	return out.str();
}

static std::string listMessage( strus::WebRequestContent::Type doctype, const char* charset, const char* rootelem, const char* listelem, const std::vector<std::string>& list)
{
	std::ostringstream out;
	std::vector<std::string>::const_iterator li = list.begin(), le = list.end();
	switch (doctype)
	{
		case strus::WebRequestContent::Unknown:
			throw std::runtime_error(_TXT("Unknown content type for output"));

		case strus::WebRequestContent::JSON:
			out << "{\n\1" << rootelem << "\1:[";
			for (; li != le; ++li)
			{
				out << "\n\t\1" << *li << "\1";
			}
			out << "\n]}\n";
			break;

		case strus::WebRequestContent::XML:
			out << "<?xml version=\"1.0\" encoding=\"" << charset << "\"?>\n<" << rootelem << ">\n";
			for (; li != le; ++li)
			{
				out << "\n\t<" << listelem << ">" << *li << "</" << listelem << ">";
			}
			out << "\n</" << rootelem << ">\n";
			break;

		case strus::WebRequestContent::TEXT:
			for (; li != le; ++li)
			{
				out << *li << "\n";
			}
			break;

		case strus::WebRequestContent::HTML:
			out << "<html><head><title>" << rootelem << "</title></head><body><h1>" << rootelem << "</h1>\n<ul>";
			for (; li != le; ++li)
			{
				out << "\n\t<li>" << *li << "</li>";
			}
			out << "\n</ul></body></html>\n";
			break;
	}
	return out.str();
}

ApplicationMessageBuf::ApplicationMessageBuf( const std::string& accepted_charset_, const std::string& accepted_doctype_)
	:m_accepted_charset(accepted_charset_),m_accepted_doctype(accepted_doctype_)
{
	m_doctype = strus::selectAcceptedContentType( m_accepted_doctype.c_str());
	m_doctypename = strus::WebRequestContent::typeName( m_doctype);
	m_charset = strus::selectAcceptedCharset( m_accepted_charset.c_str());
}

strus::WebRequestContent ApplicationMessageBuf::error( int httpstatus, int apperrorcode, const char* message)
{
	std::size_t msgbufpos = 0;
	printBufErrorMessage( m_msgbuf, sizeof(m_msgbuf), msgbufpos, m_doctype, m_charset, httpstatus, apperrorcode, message);

	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf_conv, sizeof(m_msgbuf_conv), msglen_conv, m_msgbuf, msgbufpos);

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}

strus::WebRequestContent ApplicationMessageBuf::info( const char* status, const char* message)
{
	std::size_t msgbufpos = 0;
	printBufInfoMessage( m_msgbuf, sizeof(m_msgbuf), msgbufpos, m_doctype, m_charset, status, message);

	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf_conv, sizeof(m_msgbuf_conv), msglen_conv, m_msgbuf, msgbufpos);

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}

strus::WebRequestContent ApplicationMessageBuf::info( const char* rootelem, const std::map<std::string,std::string>& message)
{
	std::string msgstr = dictMessage( m_doctype, m_charset, rootelem, message);
	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf, sizeof(m_msgbuf) + sizeof(m_msgbuf_conv), msglen_conv, msgstr.c_str(), msgstr.size());

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}

strus::WebRequestContent ApplicationMessageBuf::info( const char* rootelem, const char* listelem, const std::vector<std::string>& message)
{
	std::string msgstr = listMessage( m_doctype, m_charset, rootelem, listelem, message);
	std::size_t msglen_conv = 0;
	const char* msg = strus::convertContentCharset( m_charset, m_msgbuf, sizeof(m_msgbuf) + sizeof(m_msgbuf_conv), msglen_conv, msgstr.c_str(), msgstr.size());

	return strus::WebRequestContent( m_charset, m_doctypename, msg, msglen_conv);
}



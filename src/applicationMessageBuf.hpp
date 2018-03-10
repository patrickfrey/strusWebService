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
#ifndef _STRUS_WEBSERVICE_APPLICATION_MESSAGE_BUF_HPP_INCLUDED
#define _STRUS_WEBSERVICE_APPLICATION_MESSAGE_BUF_HPP_INCLUDED
#include "strus/webRequestContent.hpp"
#include <map>
#include <string>
#include <vector>

namespace strus {
namespace webservice {

/// \class ApplicationMessageBuf
/// \brief Abstraction for a server to client message of a cppcms application for strus with its data
class ApplicationMessageBuf
{
public:
	ApplicationMessageBuf( const std::string& accepted_charset_, const std::string& accepted_doctype_, const char* html_head_);

	strus::WebRequestContent error( int httpstatus, int apperrorcode, const char* message);
	strus::WebRequestContent info( const char* status, const char* message);

	strus::WebRequestContent::Type doctype() const	{return m_doctype;}
	const char* doctypename() const			{return m_doctypename;}
	const char* charset() const			{return m_charset;}
	const char* http_accept() const			{return m_accepted_doctype.c_str();}
	const char* http_accept_charset() const		{return m_accepted_charset.c_str();}

private:
	char m_msgbuf[ 4096 + 1024];
	char m_msgbuf_conv[ 2*4096 + 1024];
	/// \remark MessageBuf::info assumes that m_msgbuf and m_msgbuf_conv are defined in this order and without elements in between !!
	std::string m_accepted_charset;
	std::string m_accepted_doctype;
	strus::WebRequestContent::Type m_doctype;
	const char* m_doctypename;
	const char* m_charset;
	const char* m_html_head;
};

}}//namespace strus::webservice
#endif

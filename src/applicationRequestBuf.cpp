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
/// \brief Abstraction for a request of a cppcms application for strus with its data
#include "applicationRequestBuf.hpp"
#include "serviceClosure.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "strus/webRequestHandlerInterface.hpp"
#include "strus/lib/webrequest.hpp"
#include "strus/lib/error.hpp"
#include "strus/errorCodes.hpp"
#include "internationalization.hpp"

ApplicationRequestBuf::ApplicationRequestBuf(
		const ServiceClosure* service,
		const std::string& contextname,
		const std::string& schemaname,
		const std::string& argument,
		const std::string& accepted_charset_,
		const std::string& accepted_doctype_,
		const std::string& role_,
		const cppcms::http::content_type& content_type,
		const std::pair<void*,size_t>& content_data)
	:m_context(),m_content(),m_answer(),m_accepted_charset(accepted_charset_),m_accepted_doctype(accepted_doctype_),m_charset(),m_doctype(),m_role(role_)
{
	m_context.reset( service->requestHandler()->createContext( 
			contextname.c_str(), schemaname.c_str(), m_role.c_str(),
			m_accepted_charset.c_str(), m_accepted_doctype.c_str(), m_answer));
	if (!m_context.get())
	{
		return;
	}
	if (!argument.empty())
	{
		if (!m_context->addVariable( "ARG", argument)) throw std::bad_alloc();
	}
	m_doctype = content_type.media_type();
	m_charset = content_type.charset();

	m_content = strus::WebRequestContent( m_charset.c_str(), m_doctype.c_str(), (const char*)content_data.first, content_data.second);
}

void ApplicationRequestBuf::execute()
{
	m_context->execute( m_content, m_answer);
}

void ApplicationRequestBuf::debug()
{
	m_context->debug( m_content, m_answer);
}



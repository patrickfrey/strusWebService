/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the context of a delegated request (sub request to another server)
/// \file "webRequestDelegateContext.cpp"
#include "webRequestDelegateContext.hpp"
#include "internationalization.hpp"
#include "strus/base/string_format.hpp"

using namespace strus;

WebRequestDelegateContext::~WebRequestDelegateContext(){}
void WebRequestDelegateContext::putAnswer( const WebRequestAnswer& status)
{
	unsigned int rc = m_requestContext.refcnt();
	if (m_answer.ok())
	{
		if (status.ok() && status.httpstatus() >= 200 && status.httpstatus() < 300)
		{
			m_requestContext->executeSchemaPartialRequest( answerSchema.c_str(), status.content(), m_answer);
		}
		else
		{
			m_answer = status;
			std::string msg( strus::string_format( _TXT( "delegate request to %s failed"), m_url.c_str()));
			m_answer.explain( msg.c_str());
		}
	}
	if (rc <= 1)
	{
		if (m_answer.ok())
		{
			//... last delegate request reply
			m_requestContext->executeSchemaPartialRequest( answerSchema.c_str(), WebRequestContent(), m_answer);
		}
		RequestContextImpl appcontext( *m_httpContext, m_serviceClosure);
		appcontext.report_answer( m_answer, true);
		m_httpContext->complete_response();
	}
}



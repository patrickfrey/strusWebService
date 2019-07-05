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
#include "serviceClosure.hpp"
#include "internationalization.hpp"
#include "requestContextImpl.hpp"
#include "strus/base/string_format.hpp"

using namespace strus;

void WebRequestDelegateContext::putAnswer( const WebRequestAnswer& status)
{
	unsigned int rc = m_requestContext.refcnt();
	bool success = true;
	if (!m_answer.ok()) return;

	if (status.ok() && status.httpstatus() >= 200 && status.httpstatus() < 300)
	{
		if (!m_requestContext->returnDelegateRequestAnswer( m_schema.c_str(), status.content(), m_answer))
		{
			success = false;
		}
	}
	else
	{
		m_answer = status;
		success = false;
	}
	if (success)
	{
		if (rc <= 1)
		{
			//... last delegate request reply
			m_answer = m_requestContext->getRequestAnswer();
			webservice::RequestContextImpl appcontext( *m_httpContext, m_serviceClosure);
			appcontext.report_answer( m_answer, true);
			m_httpContext->complete_response();
		}
	}
	else
	{
		webservice::RequestContextImpl appcontext( *m_httpContext, m_serviceClosure);
		std::string msg( strus::string_format( _TXT( "delegate request to %s failed"), m_url.c_str()));
		m_answer.explain( msg.c_str());
		appcontext.report_answer( m_answer, true);
		m_httpContext->complete_response();
	}
	m_httpContext.reset();
	m_requestContext.reset();
}



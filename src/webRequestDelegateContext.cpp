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

void WebRequestDelegateContext::handleSuccess()
{
	WebRequestAnswer answer( m_requestContext->getRequestAnswer());
	webservice::RequestContextImpl appcontext( *m_httpContext, m_serviceClosure);
	appcontext.report_answer( answer, true);
	m_httpContext->complete_response();

	strus::WebRequestLoggerInterface* logger = m_serviceClosure->requestLogger();
	if (0 != (logger->logMask() & WebRequestLoggerInterface::LogConnectionEvents))
	{
		logger->logConnectionState( "completed delegate requests", m_requestCount);
	}
	*m_counter = 0;
}

void WebRequestDelegateContext::handleFailure( const WebRequestAnswer& status)
{
	WebRequestAnswer answer( status);
	webservice::RequestContextImpl appcontext( *m_httpContext, m_serviceClosure);
	std::string msg( strus::string_format( _TXT( "delegate request to %s failed"), m_url.c_str()));
	answer.explain( msg.c_str());
	appcontext.report_answer( answer, true);
	m_httpContext->complete_response();

	strus::WebRequestLoggerInterface* logger = m_serviceClosure->requestLogger();
	if (0 != (logger->logMask() & WebRequestLoggerInterface::LogConnectionEvents))
	{
		logger->logConnectionState( "failed delegate request", answer.httpstatus());
	}
	*m_counter = 0;
}

void WebRequestDelegateContext::putAnswer( const WebRequestAnswer& status)
{
	if (*m_counter <= 0) return;

	if (status.ok() && status.httpstatus() >= 200 && status.httpstatus() < 300)
	{
		WebRequestAnswer answer;
		if (m_requestContext->pushDelegateRequestAnswer( m_schema.c_str(), status.content(), answer))
		{
			*m_counter -= 1;
			if (!*m_counter) handleSuccess();
		}
		else
		{
			handleFailure( answer);
		}
	}
	else
	{
		handleFailure( status);
	}
	m_httpContext.reset();
	m_requestContext.reset();
}



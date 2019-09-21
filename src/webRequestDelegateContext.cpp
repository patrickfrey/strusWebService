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
#include "strus/errorCodes.hpp"

using namespace strus;

void WebRequestDelegateContext::issueDelegateRequests(
		webservice::ServiceClosure* serviceClosure_,
		const booster::shared_ptr<cppcms::http::context>& httpContext_,
		const strus::Reference<WebRequestContextInterface>& requestContext_,
		const std::vector<WebRequestDelegateRequest>& delegateRequests)
{
	strus::shared_ptr<int> counter = strus::make_shared<int>();
	*counter = delegateRequests.size();

	std::vector<strus::WebRequestDelegateRequest>::const_iterator di = delegateRequests.begin(), de = delegateRequests.end();
	std::vector<strus::Reference<strus::WebRequestDelegateContext> > receivers;

	for (; di != de; ++di)
	{
		strus::Reference<strus::WebRequestDelegateContext> receiver(
			new strus::WebRequestDelegateContext( serviceClosure_, httpContext_, requestContext_, counter, di->url(), di->receiverSchema()));
		receivers.push_back( receiver);
	}
	int didx = 0;
	di = delegateRequests.begin();

	for (; di != de; ++di,++didx)
	{
		std::string delegateContent( di->contentstr(), di->contentlen());
		strus::Reference<strus::WebRequestDelegateContext> receiver = receivers[ didx];

		if (!serviceClosure_->requestHandler()->delegateRequest( di->url(), di->method(), delegateContent, receiver.release()))
		{
			*counter = 0;
			httpContext_->complete_response();
			break;
		}
	}
}

void WebRequestDelegateContext::handleSuccess()
{
	std::vector<WebRequestDelegateRequest> followDelegateRequests = m_requestContext->getFollowDelegateRequests();
	if (followDelegateRequests.empty())
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
	}
	else
	{
		issueDelegateRequests( m_serviceClosure, m_httpContext, m_requestContext, followDelegateRequests);
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



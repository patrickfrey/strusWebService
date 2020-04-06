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
#include "strus/base/string_format.hpp"
#include "strus/errorCodes.hpp"

using namespace strus;

void WebRequestDelegateContext::issueDelegateRequests(
		webservice::ServiceClosure* serviceClosure_,
		const webservice::HttpContextRef& httpContext_,
		strus::Reference<WebRequestContextInterface>& requestContext_,
		const std::vector<WebRequestDelegateRequest>& delegateRequests)
{
	strus::shared_ptr<int> counter = strus::make_shared<int>();
	int requestCount = *counter = delegateRequests.size();

	std::vector<strus::WebRequestDelegateRequest> followDelegateRequests;
	std::vector<strus::WebRequestDelegateRequest>::const_iterator di = delegateRequests.begin(), de = delegateRequests.end();
	std::size_t didx = 0;
AGAIN:
	for (; di != de; ++di,++didx)
	{
		std::string delegateContentStr( di->contentstr(), di->contentlen());
		if (di->url())
		{
			strus::WebRequestDelegateContext* receiver
				= new strus::WebRequestDelegateContext( serviceClosure_, httpContext_, requestContext_, counter, di->url(), di->receiverSchema());
			if (!serviceClosure_->requestHandler()->delegateRequest( di->url(), di->method(), delegateContentStr, receiver))
			{
				*counter = 0;
				webservice::RequestContextImpl appcontext( httpContext_, serviceClosure_);
				appcontext.report_error_fmt( 500/*httpstatus*/, strus::ErrorCodeDelegateRequestFailed,  _TXT("delegate request send to %s failed"), di->url());
				httpContext_->complete_response();
				signalTermination( serviceClosure_);

				strus::WebRequestLoggerInterface* logger = serviceClosure_->requestLogger();
				if (0 != (logger->logMask() & WebRequestLoggerInterface::LogConnectionEvents))
				{
					logger->logConnectionState( "failed to send delegate request", 500/*httpstatus*/);
				}
				break;
			}
		}
		else
		{
			// ... without receiver feed to itself with the receiver schema:
			WebRequestContent delegateContent( "utf-8", "application/json", delegateContentStr.c_str(), delegateContentStr.size());
			WebRequestAnswer delegateAnswer( delegateContent);
			if (requestContext_->putDelegateRequestAnswer( di->receiverSchema(), delegateAnswer))
			{
				*counter -= 1;
				if (*counter == 0)
				{
					if (didx+1 != delegateRequests.size())
					{
						WebRequestAnswer answer( 500, ErrorCodeLogicError, _TXT("race: bad counter value for delegate requests"));
						webservice::RequestContextImpl appcontext( httpContext_, serviceClosure_);
						appcontext.report_error( answer.httpStatus(), answer.appErrorCode(), answer.errorStr());
						httpContext_->complete_response();
						signalTermination( serviceClosure_);
					}
					followDelegateRequests = requestContext_->getDelegateRequests();
					if (followDelegateRequests.empty())
					{
						completeResponse( serviceClosure_, httpContext_, requestContext_, requestCount);
					}
					else
					{
						di = followDelegateRequests.begin(), de = followDelegateRequests.end();
						didx = 0;
						requestCount = *counter = delegateRequests.size();
						goto AGAIN;
					}
				}
			}
			else
			{
				WebRequestAnswer answer = requestContext_->getAnswer();
				webservice::RequestContextImpl appcontext( httpContext_, serviceClosure_);
				appcontext.report_error( answer.httpStatus(), answer.appErrorCode(), answer.errorStr());
				*counter = 0;
				httpContext_->complete_response();
				signalTermination( serviceClosure_);
				break;
			}
		}
	}
}

void WebRequestDelegateContext::completeResponse(
		webservice::ServiceClosure* serviceClosure_,
		const webservice::HttpContextRef& httpContext_,
		strus::Reference<WebRequestContextInterface>& requestContext_,
		int requestCount_)
{
	(void)requestContext_->complete();
	WebRequestAnswer answer( requestContext_->getAnswer());
	webservice::RequestContextImpl appcontext( httpContext_, serviceClosure_);
	appcontext.report_answer( answer, true);
	httpContext_->complete_response();
	signalTermination( serviceClosure_);

	strus::WebRequestLoggerInterface* logger = serviceClosure_->requestLogger();
	if (0 != (logger->logMask() & WebRequestLoggerInterface::LogConnectionEvents))
	{
		logger->logConnectionState( "completed delegate requests", requestCount_);
	}
}

void WebRequestDelegateContext::completeResponse()
{
	completeResponse( m_serviceClosure, m_httpContext, m_requestContext, m_requestCount);
}

void WebRequestDelegateContext::handleSuccess()
{
	std::vector<WebRequestDelegateRequest> followDelegateRequests = m_requestContext->getDelegateRequests();
	if (followDelegateRequests.empty())
	{
		completeResponse();
	}
	else
	{
		issueDelegateRequests( m_serviceClosure, m_httpContext, m_requestContext, followDelegateRequests);
	}
}

void WebRequestDelegateContext::handleFailure( const WebRequestAnswer& status)
{
	WebRequestAnswer answer( status);
	webservice::RequestContextImpl appcontext( m_httpContext, m_serviceClosure);
	std::string msg( strus::string_format( _TXT( "delegate request to %s failed"), m_url.c_str()));
	answer.explain( msg.c_str());
	appcontext.report_answer( answer, true);
	m_httpContext->complete_response();
	signalTermination( m_serviceClosure);

	strus::WebRequestLoggerInterface* logger = m_serviceClosure->requestLogger();
	if (0 != (logger->logMask() & WebRequestLoggerInterface::LogConnectionEvents))
	{
		logger->logConnectionState( "failed delegate request", answer.httpStatus());
	}
	*m_counter = 0;
}

void WebRequestDelegateContext::putAnswer( const WebRequestAnswer& status)
{
	if (*m_counter <= 0) return;

	if (status.ok() && status.httpStatus() >= 200 && status.httpStatus() < 300)
	{
		if (m_requestContext->putDelegateRequestAnswer( m_schema.c_str(), status))
		{
			*m_counter -= 1;
			if (*m_counter == 0) handleSuccess();
		}
		else
		{
			handleFailure( m_requestContext->getAnswer());
		}
	}
	else
	{
		m_requestContext->putDelegateRequestAnswer( m_schema.c_str(), status);
		handleFailure( status);
	}
}

void WebRequestDelegateContext::signalTermination( webservice::ServiceClosure* serviceClosure_)
{
	if (serviceClosure_->doneRunRequest() && serviceClosure_->haltedForMaintenance())
	{
		serviceClosure_->processWaitingRequests();
	}
}



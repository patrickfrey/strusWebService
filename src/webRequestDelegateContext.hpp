/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the context of a delegated request (sub request to another server)
/// \file "webRequestDelegateContext.hpp"
#ifndef _STRUS_WEB_REQUEST_DELEGATE_CONTEXT_IMPL_HPP_INCLUDED
#define _STRUS_WEB_REQUEST_DELEGATE_CONTEXT_IMPL_HPP_INCLUDED
#include "strus/webRequestDelegateContextInterface.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/shared_ptr.hpp"
#include "serviceClosure.hpp"
#include <cppcms/application.h>
#include <iostream>
#include <string>

namespace strus {

class WebRequestDelegateContext
	:public WebRequestDelegateContextInterface
{
public:
	WebRequestDelegateContext(
			webservice::ServiceClosure* serviceClosure_,
			const booster::shared_ptr<cppcms::http::context>& httpContext_,
			const strus::Reference<WebRequestContextInterface>& requestContext_,
			const strus::shared_ptr<int>& counter_,
			const std::string& url_,
			const std::string& schema_)
		:m_serviceClosure(serviceClosure_)
		,m_httpContext(httpContext_),m_requestContext(requestContext_)
		,m_counter(counter_),m_requestCount(*counter_)
		,m_url(url_),m_schema(schema_)
	{}

	virtual ~WebRequestDelegateContext(){}

	virtual void putAnswer( const WebRequestAnswer& status);

public:
	static void issueDelegateRequests(
			webservice::ServiceClosure* serviceClosure_,
			const booster::shared_ptr<cppcms::http::context>& httpContext_,
			const strus::Reference<WebRequestContextInterface>& requestContext_,
			const std::vector<WebRequestDelegateRequest>& delegateRequests);

private:
	void handleFailure( const WebRequestAnswer& status);
	void handleSuccess();

private:
	webservice::ServiceClosure* m_serviceClosure;			//< service closure needed by webserver context
	booster::shared_ptr<cppcms::http::context> m_httpContext;	//< httpContext that hold the webserver connection context
	strus::Reference<WebRequestContextInterface> m_requestContext;	//< request context that gets the answer
	strus::shared_ptr<int> m_counter;				//< counter of open requests for book-keeping and determining when the request can be completed
	int m_requestCount;						//< count of requests
	std::string m_url;						//< url of the request for error messages
	std::string m_schema;						//< schema used to intrepret the answer of the request
};

}//namespace
#endif


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
#include "serviceClosure.hpp"
#include <cppcms/application.h>
#include <iostream>
#include <string>

namespace strus {

class WebRequestDelegateContext
	:public WebRequestDelegateContextInterface
{
public:
	class AliveFlag :public booster::shared_ptr<bool>
	{
	public:
		AliveFlag() :booster::shared_ptr<bool>( new bool(true)){}
	};

public:
	WebRequestDelegateContext(
			webservice::ServiceClosure* serviceClosure_,
			const booster::shared_ptr<cppcms::http::context>& httpContext_,
			const strus::Reference<WebRequestContextInterface>& requestContext_,
			const AliveFlag& alive_,
			const std::string& url_,
			const std::string& schema_)
		:m_serviceClosure(serviceClosure_)
		,m_httpContext(httpContext_),m_requestContext(requestContext_),m_alive(alive_)
		,m_url(url_),m_schema(schema_)
	{}

	virtual ~WebRequestDelegateContext(){}

	virtual void putAnswer( const WebRequestAnswer& status);

private:
	void handleFailure( const WebRequestAnswer& status);
	void handleSuccess();

private:
	webservice::ServiceClosure* m_serviceClosure;			//< service closure needed by webserver context
	booster::shared_ptr<cppcms::http::context> m_httpContext;	//< httpContext that hold the webserver connection context
	strus::Reference<WebRequestContextInterface> m_requestContext;	//< request context that gets the answer
	AliveFlag m_alive;						//< flag that if false marks if a connection has already been resumed and must not be touched anymore
	std::string m_url;						//< url of the request for error messages
	std::string m_schema;						//< schema used to intrepret the answer of the request
};

}//namespace
#endif


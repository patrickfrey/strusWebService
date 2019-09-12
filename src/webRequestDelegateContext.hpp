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
		,m_answer()
	{
		/*[-]*/std::cerr << "+++ create delegate context " << m_httpContext.use_count() << std::endl;
	}

	virtual ~WebRequestDelegateContext()
	{
		/*[-]*/std::cerr << "+++ delete delegate context " << m_httpContext.use_count() << std::endl;
	}
	virtual void putAnswer( const WebRequestAnswer& status);

private:
	webservice::ServiceClosure* m_serviceClosure;
	booster::shared_ptr<cppcms::http::context> m_httpContext;
	strus::Reference<WebRequestContextInterface> m_requestContext;
	AliveFlag m_alive;
	std::string m_url;
	std::string m_schema;
	WebRequestAnswer m_answer;
};

}//namespace
#endif


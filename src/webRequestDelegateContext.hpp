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
#include "strus/WebRequestDelegateContextInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/thread.hpp"
#include "serviceClosure.hpp"
#include <cppcms/application.h>
#include <string>

namespace strus {

class WebRequestDelegateContext
	:public WebRequestDelegateContextInterface
{
public:
	WebRequestDelegateContext(
			ServiceClosure* serviceClosure_,
			const booster::shared_ptr<http::context> httpContext_,
			const strus::Reference<WebRequestContextInterface>& requestContext_,
			const std::string& url_,
			const std::string& answerSchema_,
			const std::string& completeSchema_)
		:m_serviceClosure(serviceClosure_),m_httpContext(httpContext_),m_requestContext(requestContext_)
		,m_url(url_),m_answerSchema(answerSchema_),m_completeSchema(completeSchema_)
		,m_answer(){}

	~WebRequestDelegateContext(){}
	virtual void putAnswer( const WebRequestAnswer& status);

private:
	ServiceClosure* m_serviceClosure;
	booster::shared_ptr<http::context> m_httpContext;
	strus::Reference<WebRequestContextInterface> m_requestContext;
	std::string m_url;
	std::string m_answerSchema;
	std::string m_completeSchema;
	WebRequestAnswer m_answer;
};

}//namespace
#endif


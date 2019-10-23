/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Context of a queued request, waiting for some event to occurr (e.g. resources/data exclusively available)
/// \file "webRequestWaitingContext.hpp"
#ifndef _STRUS_WEB_REQUEST_WAITING_CONTEXT_IMPL_HPP_INCLUDED
#define _STRUS_WEB_REQUEST_WAITING_CONTEXT_IMPL_HPP_INCLUDED
#include "strus/webRequestDelegateContextInterface.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "requestContextImpl.hpp"
#include "strus/reference.hpp"
#include "strus/base/thread.hpp"
#include "serviceClosure.hpp"
#include <cppcms/application.h>
#include <iostream>
#include <string>
#include <utility>

namespace strus {

class WebRequestWaitingContext
{
public:
	WebRequestWaitingContext(
			webservice::ServiceClosure* serviceClosure_,
			const webservice::HttpContextRef& httpContext_,
			const strus::Reference<WebRequestContextInterface>& requestContext_,
			const strus::WebRequestContent& content_)
		:m_serviceClosure(serviceClosure_),m_httpContext(httpContext_),m_requestContext(requestContext_),m_content(content_),m_contentstr(content_.str(),content_.len())
	{
		m_content.setContent( m_contentstr.c_str(), m_contentstr.size());
	}
	WebRequestWaitingContext( const WebRequestWaitingContext& o)
		:m_serviceClosure(o.m_serviceClosure),m_httpContext(o.m_httpContext),m_requestContext(o.m_requestContext),m_content(o.m_content),m_contentstr(o.m_contentstr)
	{
		m_content.setContent( m_contentstr.c_str(), m_contentstr.size());
	}
#if __cplusplus >= 201103L
	WebRequestWaitingContext( WebRequestWaitingContext&& o)
		:m_serviceClosure(o.m_serviceClosure),m_httpContext(std::move(o.m_httpContext)),m_requestContext(std::move(o.m_requestContext)),m_content(std::move(o.m_content)),m_contentstr(std::move(o.m_contentstr)){}
	WebRequestWaitingContext& operator=( WebRequestWaitingContext&& o)
		{m_serviceClosure=o.m_serviceClosure;m_httpContext=std::move(o.m_httpContext);m_requestContext=std::move(o.m_requestContext);m_content=std::move(o.m_content);m_contentstr=std::move(o.m_contentstr); return *this;}
#endif
	virtual ~WebRequestWaitingContext(){}

	webservice::ServiceClosure* serviceClosure()			{return m_serviceClosure;}
	webservice::HttpContextRef httpContext()			{return m_httpContext;}
	strus::Reference<WebRequestContextInterface> requestContext()	{return m_requestContext;}
	const strus::WebRequestContent& content() const			{return m_content;}

private:
	webservice::ServiceClosure* m_serviceClosure;			//< service closure needed by webserver context
	webservice::HttpContextRef m_httpContext;			//< httpContext that hold the webserver connection context
	strus::Reference<WebRequestContextInterface> m_requestContext;	//< request context executed
	strus::WebRequestContent m_content;
	std::string m_contentstr;
};

}//namespace
#endif


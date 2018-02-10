# strusWebService

This is a web service exposing the strus API via a HTTP/JSON protocol.
It resembles in many ways how Elasticsearch is built.

The implementation is done in C++ and uses the
[CppCMS](http://cppcms.com/wikipp/en/page/main) 
framework to implement the actual web service.

For easier use to access the web service from Java
there is a small API at [strusJavaApi](https://github.com/Eurospider/strusJavaApi).

Currently only functionality from the [strus](https://github.com/patrickfrey/strus) API
is exposed, not from other parts of strus (e. g. strusAnalyzer).

A small presentation of ideas is available
[here](http://eurospider.github.io/strusWebService/doc/slides/strus-webservice.html).

Roadmap is available [here](ROADMAP.md).


# strusWebService based on strusBindings

This clone of the project relies on a different API. It uses papuga to map XML/JSON requests to calls of the [strusBindings API (example Lua)](http://www.project-strus.net/luaBindingsDoc.htm) instead of doing the job based on the API of the projects strus and strusAnalyzer.




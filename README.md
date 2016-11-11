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
[here](http://www.andreasbaumann.cc/presentations/struswebservice.html).

Roadmap is available [here](ROADMAP.md).

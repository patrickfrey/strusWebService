# strusWebService

This is a web service exposing the strus API as a HTTP/JSON protocol.
It resembles in many ways Elasticsearch.

The implementation is done in C++ and uses the
[CppCMS](http://cppcms.com/wikipp/en/page/main) 
framework to implement the actual web service.

For easier use to access the web service from Java
there is a small API at [strusJavaApi](https://github.com/Eurospider/strusJavaApi).

Currently only functionallity from the [strus](https://github.com/patrickfrey/strus) API
is exposed, not of other parts of strus (e. g. strusAnalyzer).

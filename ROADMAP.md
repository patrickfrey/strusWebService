# strusWebService - Roadmap

This document describes the current status and the upcoming milestones of the strusWebService project.

*Updated: Fri, 30 Dec 2016 13:48:02 GMT*

#### Milestone Summary

| Status | Milestone | Goals | ETA |
| :---: | :--- | :---: | :---: |
| 🚀 | **[0.0.5](#0.0.5)** | 24 / 24 | Sat Dec 31 2016 |
| 🚀 | **[0.0.6](#0.0.6)** | 0 / 0 | Fri Mar 31 2017 |
| 🚀 | **[blue sky](#blue-sky)** | 0 / 30 | Sun Dec 31 2017 |

## Milestones and Goals

#### 0.0.5

> 

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**24 / 24** goals completed **(100%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**Sat Dec 31 2016**

| Status | Goal | Labels | Repository |
| :---: | :--- | --- | --- |
| ✔ | [renaming of indexes](https://github.com/Eurospider/strusWebService/issues/84) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [Logrotation](https://github.com/Eurospider/strusWebService/issues/83) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [Running out of strus error buffers](https://github.com/Eurospider/strusWebService/issues/78) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [debug option to record all requests and write them to disk (and the answers)](https://github.com/Eurospider/strusWebService/issues/75) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [periodic check for open transactions](https://github.com/Eurospider/strusWebService/issues/71) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [handle broken pipes in transactions](https://github.com/Eurospider/strusWebService/issues/70) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [SIGHUP reopens new and new application objects running in parallel](https://github.com/Eurospider/strusWebService/issues/69) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [swapping indexes while inserting kills the service](https://github.com/Eurospider/strusWebService/issues/68) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [segfault in transaction commit](https://github.com/Eurospider/strusWebService/issues/67) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [delete index crashes the web service](https://github.com/Eurospider/strusWebService/issues/62) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [introduce global locking for certain operations](https://github.com/Eurospider/strusWebService/issues/61) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [Closing the index crashes the server](https://github.com/Eurospider/strusWebService/issues/57) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [segfault in long transaction](https://github.com/Eurospider/strusWebService/issues/54) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [segfault while inserting and issueing a close index in the middle of a transaction](https://github.com/Eurospider/strusWebService/issues/53) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [stack trace in web service log on segfault or fatal error missing](https://github.com/Eurospider/strusWebService/issues/51) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [explicit close and open of index](https://github.com/Eurospider/strusWebService/issues/49) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [open transactions are not closed when the webservice shuts down](https://github.com/Eurospider/strusWebService/issues/47) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [running out of file descriptors](https://github.com/Eurospider/strusWebService/issues/46) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [web service gets stuck](https://github.com/Eurospider/strusWebService/issues/36) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [daemonizing for Fedora, Debian, Ubuntu, SuSE](https://github.com/Eurospider/strusWebService/issues/33) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [Weird error message if the service is already running](https://github.com/Eurospider/strusWebService/issues/20) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [OpenSuseBuild failing](https://github.com/Eurospider/strusWebService/issues/14) |`bug`, `enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [insert, update, delete, commit, rollback should report a 'number of documents affected'](https://github.com/Eurospider/strusWebService/issues/11) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ✔ | [add support restriction and exclusion feature sets](https://github.com/Eurospider/strusWebService/issues/2) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |


#### 0.0.6

> 

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**0 / 0** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**Fri Mar 31 2017**

| Status | Goal | Labels | Repository |
| :---: | :--- | --- | --- |


#### blue sky

> Will not happen now or in the near future. Open for discussion and scheduling.

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**0 / 30** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**Sun Dec 31 2017**

| Status | Goal | Labels | Repository |
| :---: | :--- | --- | --- |
| ❌ | [during startup there is a lock race](https://github.com/Eurospider/strusWebService/issues/87) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [crash in live system](https://github.com/Eurospider/strusWebService/issues/82) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [CPU usage is pretty high](https://github.com/Eurospider/strusWebService/issues/74) |`bug`, `enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [Suddenly high memory usage](https://github.com/Eurospider/strusWebService/issues/72) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [Busy after restart of web service](https://github.com/Eurospider/strusWebService/issues/66) |`question`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [better error reporting in log](https://github.com/Eurospider/strusWebService/issues/64) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [cannot query only with restriction features](https://github.com/Eurospider/strusWebService/issues/56) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [have an auto-commit parameter for dumb clients](https://github.com/Eurospider/strusWebService/issues/52) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [strusUtilities and strusRpcServer cannot run on the storages managed by the webservice](https://github.com/Eurospider/strusWebService/issues/44) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [valgrind shows tons of errors](https://github.com/Eurospider/strusWebService/issues/43) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [re-registration of query weighting function results in out-of-memory](https://github.com/Eurospider/strusWebService/issues/40) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [summarizer names are ignored](https://github.com/Eurospider/strusWebService/issues/37) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [delete index after inserting documents fails](https://github.com/Eurospider/strusWebService/issues/35) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [the democlient is quite broken and not up-to-date](https://github.com/Eurospider/strusWebService/issues/32) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [query tests are failing](https://github.com/Eurospider/strusWebService/issues/31) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [add browsing support and 'all match' type of queries](https://github.com/Eurospider/strusWebService/issues/30) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [creating a storage with illegal metadata configuration creates an index without metadata](https://github.com/Eurospider/strusWebService/issues/29) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [weighting functions: adaption to new scalar function model](https://github.com/Eurospider/strusWebService/issues/27) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [add support for metadata configuration changes](https://github.com/Eurospider/strusWebService/issues/26) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [default_create_parameters in configuration are also used for reading](https://github.com/Eurospider/strusWebService/issues/25) |`enhancement`, `question`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [implement update document](https://github.com/Eurospider/strusWebService/issues/24) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [add introspection of storage statistics](https://github.com/Eurospider/strusWebService/issues/23) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [no built-in authentication](https://github.com/Eurospider/strusWebService/issues/16) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [architecture redesign in core](https://github.com/Eurospider/strusWebService/issues/13) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [copy-paste-code in JSON serializers/deserializers](https://github.com/Eurospider/strusWebService/issues/10) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [passing features as weighting and summarizer parameters is done with heuristics](https://github.com/Eurospider/strusWebService/issues/9) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [bad error handling in JSON messages](https://github.com/Eurospider/strusWebService/issues/7) |`bug`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [transactions work locally only per index](https://github.com/Eurospider/strusWebService/issues/6) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [ACLs are not handled](https://github.com/Eurospider/strusWebService/issues/5) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |
| ❌ | [storage implementation cannot be loaded from a module](https://github.com/Eurospider/strusWebService/issues/4) |`enhancement`| <a href=https://github.com/Eurospider/strusWebService>Eurospider/strusWebService</a> |




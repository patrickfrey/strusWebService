curl -d "@examples/query.orig.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"
curl -d "@examples/query.orig.xml" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"

curl -d "@examples/query.orig.json" -i -H "Accept: application/xml" -H "Content-Type: application/json; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"
curl -d "@examples/query.orig.xml" -i -H "Accept: application/xml" -H "Content-Type: application/xml; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"

curl -d "@examples/query.orig.json" -i -H "Accept: application/json" -H "Content-Type: application/json; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"
curl -d "@examples/query.orig.xml" -i -H "Accept: application/json" -H "Content-Type: application/xml; charset=UTF-8" -X POST  "http://127.0.0.1:8080/queryorig/storage/storage"


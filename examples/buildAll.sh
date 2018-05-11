#!/bin/sh

echo "--- Create analyzer ---"
curl -d "@examples/docanalyzer.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "http://127.0.0.1:8080/docanalyzer/reuters"

echo "--- Create storage ---"
curl -d "@examples/storage.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "http://127.0.0.1:8080/storage/reuters"

echo "--- Create inserter ---"
curl -d "@examples/inserter.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "http://127.0.0.1:8080/inserter/reuters"

echo "--- Insert documents ---"
TRANSACTION=`curl -H "Accept: text/plain" -X POST "http://127.0.0.1:8080/inserter/reuters/transaction"`
for dd in `find tests/data/reuters/doc/ -name "*.xml"`; do
	echo "--- Insert $dd ---"
	curl -d "@$dd" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"
done
curl -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"


#!/bin/sh

SERVER="http://127.0.0.1:8080"

echo "--- Create content statistics analyzer ---"
curl -d "@examples/contentstats.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER/contentstats/reuters"

echo "--- Analyze documents ---"
TRANSACTION=`curl -H "Accept: text/plain" -X POST "$SERVER/contentstats/reuters/transaction"`
for dd in `find tests/data/reuters/doc/ -name "reut2-000.xml"`; do
	echo "--- Collect statistics $dd ---"
	curl -d "@$dd" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"
done
echo "transaction identifier: $TRANSACTION"

echo "--- Create analyzer ---"
curl -d "@examples/docanalyzer.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER/docanalyzer/reuters"

echo "--- Create storage ---"
curl -d "@examples/storage.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X POST "$SERVER/storage/reuters"

echo "--- Create inserter ---"
curl -d "@examples/inserter.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER/inserter/reuters"

echo "--- Insert documents ---"
TRANSACTION=`curl -H "Accept: text/plain" -X POST "$SERVER/inserter/reuters/transaction"`
for dd in `find tests/data/reuters/doc/ -name "*.xml"`; do
	echo "--- Insert $dd ---"
	curl -d "@$dd" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"
done
curl -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"

echo "--- Query ---"
curl -d "@examples/query.orig.json" -i -H "Accept: application/json" -H "Content-Type: application/json; charset=UTF-8" -X GET  "$SERVER/storage/reuters"


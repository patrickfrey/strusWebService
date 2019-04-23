#!/usr/bin/perl
# -----------------------------------------------------
# Strus Webservice CLient for processing JSON in Perl 
# -----------------------------------------------------
use strict;
use warnings;

use Strus::Client;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <method/proc> <url> <content>\n";
    exit 1;
}
if ($ARGV[0] eq "document")
{
	my $storageurl = $ARGV[1];
	Strus::Client::printResult( Strus::Client::selectResult( Strus::Client::issueRequest( "GET", $storageurl, undef), ("storage","sindex","stem","value")), "\n")
}
else
{
	Strus::Client::printResult( Strus::Client::issueRequest( $ARGV[0], $ARGV[1], $ARGV[2]), "\n");
}



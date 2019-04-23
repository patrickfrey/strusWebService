#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <url> <featuretype>\n";
    exit 1;
}
my $storageurl = $ARGV[0];
my $feattype = $ARGV[1];
my @reqresult = Strus::Client::readResult(
			Strus::Client::selectResult(
				Strus::Client::issueRequest( "GET", $storageurl, undef), ("storage","sindex",$feattype,"value")), '@');
foreach my $value( @reqresult)
{
	print "$value\n";
}



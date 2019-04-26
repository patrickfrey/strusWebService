#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <docurl> <featuretype>\n";
    exit 1;
}
my $docurl = $ARGV[0];
my $feattype = $ARGV[1];

my @reqresult = Strus::Client::issueRequest( "GET", $docurl, undef);
if (!defined $reqresult[1])
{
	print STDERR "ERR $reqresult[0]\n";
}
else
{
	my @result = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage","sindex",$feattype,"value")), '@');
	foreach my $value( @result)
	{
		next if ($value eq "");
		print "$value\n";
	}
}



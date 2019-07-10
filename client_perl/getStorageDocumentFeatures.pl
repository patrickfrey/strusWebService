#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;
use URI::Encode;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <storageurl> <docid> <featuretype>\n";
    exit 1;
}
my $storageurl = $ARGV[0];
my $docid = $ARGV[1];
my $feattype = $ARGV[2];

my $uri     = URI::Encode->new( { encode_reserved => 0 } );
my $docurl = "$storageurl/doc/" . $uri->encode($docid);

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



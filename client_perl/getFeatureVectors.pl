#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <url> <inputfeatures>\n";
    exit 1;
}
my $storageurl = $ARGV[0];
my @featurevalues = Strus::Client::readInput( $ARGV[1], '@');
my @result = ();
push( @result, ":vstorage","(",":feature","(");

foreach my $featurevalue( @featurevalues) {
	my $url = "$storageurl/feature/$featurevalue";
	print "URL $url\n";
	my @featuretypes = Strus::Client::readResult( 
					Strus::Client::selectResult( 
						Strus::Client::issueRequest( "GET", $url, undef), ("vstorage","link")), '@');
	foreach my $featuretype( @featuretypes)
	{
		print "STEP 1 $featuretype\n";
		my $vecurl = "$url/$featuretype";
		my @vector = Strus::Client::readResult( 
					Strus::Client::selectResult( 
						Strus::Client::issueRequest( "GET", $vecurl, undef), ("vstorage","value")), '@');
		push( @result, "(", ":name", "=$featurevalue", ":type", "=$featuretype", ":vector", "(" );
		foreach my $scalar (@vector) {
			push( @result, "=$scalar" );
		}
		push( @result, ")", ")" );
	}
}

push( @result, ")",")" );

print "STEP 2 @result\n";
print( Strus::Client::serializationToJson( @result));


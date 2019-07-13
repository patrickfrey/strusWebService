#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;
use URI::Encode;

my $uri = URI::Encode->new( { encode_reserved => 1 } );
sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

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
	$featurevalue = trim( $featurevalue);
	next if ($featurevalue eq "");

	my $url = "$storageurl/feature/" . $uri->encode( $featurevalue);
	my @valresult = Strus::Client::issueRequest( "GET", $url, undef);
	if (!defined $valresult[1])
	{
		print STDERR "ERR $valresult[0] feature value '$featurevalue'\n";
	}
	else
	{
		my @featuretypes = Strus::Client::readResult( Strus::Client::selectResult( $valresult[1], ("vstorage","link")), '@');
		foreach my $featuretype( @featuretypes)
		{
			$featuretype = trim( $featuretype);
			next if ($featuretype eq "");

			my $vecurl = "$url/" . $uri->encode( $featuretype);
			my @vecresult = Strus::Client::issueRequest( "GET", $vecurl, undef);
			if (!defined $vecresult[1])
			{
				print STDERR "ERR $vecresult[0] feature vector $featuretype '$featurevalue'\n";
			}
			else
			{
				my @vector = Strus::Client::readResult( Strus::Client::selectResult( $vecresult[1], ("vstorage","value")), '@');
				if ($#vector < 0)
				{
					push( @result, "(", ":name", "=$featurevalue", ":type", "=$featuretype", ")");
				}
				else
				{
					push( @result, "(", ":name", "=$featurevalue", ":type", "=$featuretype", ":vector", "(" );
					foreach my $scalar (@vector) {
						push( @result, "=$scalar" );
					}
					push( @result, ")", ")" );
				}
			}
		}
	}
}

push( @result, ")",")" );

print( Strus::Client::serializationToJson( @result));


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
my @result = Strus::Client::issueRequest( $ARGV[0], $ARGV[1], $ARGV[2]);
if (!defined $result[1])
{
	print STDERR "ERROR $result[0]\n";
	exit $result[0];
}
else
{
	print STDERR "OK $result[0]\n";
	Strus::Client::printResult( $result[1], "\n");
	print "\n";
	exit 0;
}



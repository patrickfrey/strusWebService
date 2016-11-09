#!/usr/bin/perl -w

use strict;
use warnings;
use open qw(:std :utf8);

binmode STDOUT, ':utf8';

my $line;
my $line2;
my $count;
my $tid = "";
my $f;

$count = 0;
while( 1 ) {
  $line = <>;
  last if( !defined( $line ) );
  if( $line =~ /transaction\/begin/ ) {
	  $line2 = <>;
	  $line2 =~ /"id":"([^"]+)"/;
	  $tid = $1;
	  close $f if( defined $f );
	  open $f, ">:encoding(UTF-8)", "trans_$count.log";
	  print $f "POST\n";
	  print $f $line;
	  print $f $line2;
	  $count++;
  } else {
	  print $f $line if defined( $f );
  }
}

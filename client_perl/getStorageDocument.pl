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
    print "usage: $0 <storageurl> <docid>\n";
    exit 1;
}
my $storageurl = $ARGV[0];
my $docid = $ARGV[1];
my $uri     = URI::Encode->new( { encode_reserved => 0 } );
my $docurl = "$storageurl/doc/" . $uri->encode($docid);

my @result = ();

sub getTermTypes {
	my ($storageurl) = @_;
	my @reqresult = Strus::Client::issueRequest( "GET", "$storageurl/termtype", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $storageurl\n";
		exit $reqresult[0];
	}
	return Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage","value")), '@');
}
my @termtypes = getTermTypes( $storageurl);


sub printResultSearchIndex {
	my ($docurl) = @_;
	my $sindexurl = "$docurl/sindex";
	my @reqresult = Strus::Client::issueRequest( "GET", "$sindexurl", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $sindexurl\n";
		exit $reqresult[0];
	}
	push( @result, ":searchindex","(" );
	foreach my $termtype( @termtypes) {
		next if ($termtype eq "");
		my @termvalues = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage",$termtype,"value")), '@');
		foreach my $termvalue( @termvalues)
		{
			next if ($termvalue eq "");
			my $postingsurl = "$sindexurl/$termtype/$termvalue";
			my @poreqresult = Strus::Client::issueRequest( "GET", "$postingsurl", undef);
			if (!defined $poreqresult[1])
			{
				print STDERR "ERR $poreqresult[0] $postingsurl\n";
				exit $poreqresult[0];
			}
			my @positions = Strus::Client::readResult( Strus::Client::selectResult( $poreqresult[1], ("storage","value")), '@');
			foreach my $position( @positions) {
				push( @result, "(", ":type", "=$termtype", ":value", "=$termvalue", ":pos", "=$position", ")");
			}
		}
	}
	push( @result, ")" );
}

sub printResultForwardIndex {
	my ($docurl) = @_;
	push( @result, ":forwardindex","(" );
	foreach my $termtype( @termtypes) {
		next if ($termtype eq "");
		my $findexurl = "$docurl/findex/$termtype/list";
		my @reqresult = Strus::Client::issueRequest( "GET", "$findexurl", undef);
		if (!defined $reqresult[1])
		{
			print STDERR "ERR $reqresult[0] $findexurl\n";
		}
		else
		{
			my @termvalues = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage","value","value")), '@');
			my @positions = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage","value","pos")), '@');
			if ($#termvalues != $#positions)
			{
				print STDERR "ERR 500 mismatch\n";
				exit 1;
			}
			for (my $ti=0; $ti<=$#termvalues; $ti++)
			{
				my $termvalue = $termvalues[ $ti];
				my $position = $positions[ $ti];
				push( @result, "(", ":type", "=$termtype", ":value", "=$termvalue", ":pos", "=$position", ")");
			}
		}
	}
	push( @result, ")" );
}

sub printResultAttributes {
	my ($docurl) = @_;
	push( @result, ":attribute","(" );

	my $attributeurl = "$docurl/attribute";
	my @reqresult = Strus::Client::issueRequest( "GET", "$attributeurl", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $attributeurl\n";
	}
	else
	{
		my %attributemap = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage")), '%');
		foreach my $attributekey( keys %attributemap)
		{
			next if ($attributekey eq "");
			my $attributeval = $attributemap{ $attributekey};
			push( @result, "(", ":name", "=$attributekey", ":value", "=$attributeval", ")");
		}
	}
	push( @result, ")" );
}

sub printResultMetadata {
	my ($docurl) = @_;
	push( @result, ":metadata","(" );

	my $metadataurl = "$docurl/metadata";
	my @reqresult = Strus::Client::issueRequest( "GET", "$metadataurl", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $metadataurl\n";
	}
	else
	{
		my %metadatamap = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage")), '%');
		foreach my $metadatakey( keys %metadatamap)
		{
			next if ($metadatakey eq "");
			my $metadataval = $metadatamap{ $metadatakey};
			push( @result, "(", ":name", "=$metadatakey", ":value", "=$metadataval", ")");
		}
	}
	push( @result, ")" );
}

sub printResultAccess {
	my ($docurl) = @_;

	my $accessurl = "$docurl/access";
	my @reqresult = Strus::Client::issueRequest( "GET", "$accessurl", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $accessurl\n";
	}
	else
	{
		my @usernames = Strus::Client::readResult( Strus::Client::selectResult( $reqresult[1], ("storage","value")), '@');
		if ($#usernames >= 0)
		{
			push( @result, ":access","(" );
			foreach my $username( @usernames)
			{
				next if ($username eq "");
				push( @result, "=$username" );
			}
			push( @result, ")" );
		}
	}
}


push( @result, ":storage", "(", ":document", "(" );
push( @result, ":id", "=$docid" );

printResultSearchIndex( $docurl);
printResultForwardIndex( $docurl);
printResultAttributes( $docurl);
printResultMetadata( $docurl);
printResultAccess( $docurl);
push( @result, ")", ")");

print( Strus::Client::serializationToJson( @result));




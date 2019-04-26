#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;

# Main:
if ($#ARGV <= 0) {
    print "usage: $0 <storageurl> <docid>\n";
    exit 1;
}
my $storageurl = $ARGV[0];
my $docid = $ARGV[1];

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

sub printResultSearchIndex {
	my ($storageurl, $docid) = @_;
	my $sindexurl = "$storageurl/doc/$docid/sindex";
	my @reqresult = Strus::Client::issueRequest( "GET", "$sindexurl", undef);
	if (!defined $reqresult[1])
	{
		print STDERR "ERR $reqresult[0] $sindexurl\n";
		exit $reqresult[0];
	}
	push( @result, ":searchindex","(" );
	my @termtypes = getTermTypes( $storageurl);
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
	my ($storageurl, $docid) = @_;
	push( @result, ":forwardindex","(" );
	my @termtypes = getTermTypes( $storageurl);
	foreach my $termtype( @termtypes) {
		next if ($termtype eq "");
		my $findexurl = "$storageurl/doc/$docid/findex/$termtype/list";
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
	my ($storageurl, $docid) = @_;
	push( @result, ":attribute","(" );

	my $attributeurl = "$storageurl/doc/$docid/attribute";
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
	my ($storageurl, $docid) = @_;
	push( @result, ":metadata","(" );

	my $metadataurl = "$storageurl/doc/$docid/metadata";
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
	my ($storageurl, $docid) = @_;

	my $accessurl = "$storageurl/doc/$docid/access";
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

printResultSearchIndex( $storageurl, $docid);
printResultForwardIndex( $storageurl, $docid);
printResultAttributes( $storageurl, $docid);
printResultMetadata( $storageurl, $docid);
printResultAccess( $storageurl, $docid);
push( @result, ")", ")");

print( Strus::Client::serializationToJson( @result));




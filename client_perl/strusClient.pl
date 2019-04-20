#!/usr/bin/perl
# -------------------------------------------------------------------------------------------------------------------------------
# CLient in perl inspired from examples in https://developer.atlassian.com/server/fisheye-crucible/writing-a-rest-client-in-perl/
# -------------------------------------------------------------------------------------------------------------------------------
use strict;
use REST::Client;
use JSON;
use Data::Dumper;
use MIME::Base64;

if ($#ARGV <= 0) {
    print "usage: $0 <method> <url> $#ARGV\n";
    exit 1;
}
my $method = $ARGV[0];
my $url = $ARGV[1];

my $headers = {Accept => 'application/json'};
my $client = REST::Client->new();
my $server;
my $path;

if ($url =~ /^(http:\/\/[_0-9a-z\.\-]*[:][0-9]{3,5})([\/].*)$/)
{
	$server = $1;
	$path = $2;
}
elsif ($url =~ /^(http:\/\/[_0-9a-z\.\-]*)([\/].*)$/)
{
	$server = $1;
	$path = $2;
}
elsif ($url =~ /^([_0-9a-z\.\-]*[:][0-9]{3,5})([\/].*)$/)
{
	$server = $1;
	$path = $2;
}
elsif ($url =~ /^([_0-9a-z\.\-]*)([\/].*)$/)
{
	$server = $1;
	$path = $2;
}
else
{
	print "invalid url: $url\n";
	exit 1;
}

print "METHOD: '$method'\n";
print "SERVER: '$server'\n";
print "PATH: '$path'\n";

$client->setHost( $server);
if ($method eq "GET")
{
	$client->GET(
		$path, 
		$headers
	);
}
elsif ($method eq "DELETE")
{
	$client->DELETE(
		$path, 
		$headers
	);
}
else
{
	print "method not supported: $method\n";
	exit 1;
}

sub iterateData {
	my ($node, $indent) = @_;
	my $nodetype = ref($node);
	if ($nodetype eq "HASH") {
		my %nodemap = %$node;
		foreach my $key (keys %nodemap)
		{
			my $subnode = $nodemap{$key};
			print "$indent $key:";
			iterateData( $subnode, $indent . "   ");
		}
	}
	elsif ($nodetype eq "ARRAY") {
		my @nodear = @$node;
		for my $subnode (@nodear) {
			iterateData( $subnode, $indent);
		}
		print "\n";
	}
	elsif ($nodetype eq "LVALUE") {
		my $nodeval = $$node;
		print "$indent$nodeval";
	}
	else
	{
		print "$indent$node";
	}
}

# print $client->responseContent();

my $response = from_json($client->responseContent());
iterateData( $response, "\n");





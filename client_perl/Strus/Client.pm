#!/usr/bin/perl
# -----------------------------------------------------
# Strus Webservice CLient for processing JSON in Perl 
# -----------------------------------------------------
use strict;
use warnings;

package Strus::Client;
use REST::Client;
use JSON;
use Data::Dumper;
use MIME::Base64;
use LWP::UserAgent;

sub readInput {
	my ($content,$type) = @_;
	if ($content)
	{
		if (substr( $content, 0, 1) eq '@')
		{
			my $filename = substr( $content, 1);
			open my $fh, '<', $filename or die "error opening $filename: $!";
			$content = do { local $/; <$fh> };
		}
		elsif ($content eq '-')
		{
			$content = "";
			while (<STDIN>) {
				$content .= $_;
			}
		}
	}
	if (!defined $type)
	{
		return $content;
	}
	elsif ($type eq '@' or $type eq "ARRAY")
	{
		return  split /\n/, $content;
	}
	elsif ($type eq '%' or $type eq "HASH")
	{
		my %rt = {};
		for my $line (split /\n/, $content) {
			my ($key, $value) = split /\t/, $line, 2;
			$rt{ $key} = $value;
		}
		return %rt;
	}
	else
	{
		print "undefined input type: $type\n";
		exit 1;
	}
}

sub readResult {
	my ($content,$type) = @_;
	if (!defined $type)
	{
		if (!defined $content)
		{
			return undef;
		}
		if (ref($content) eq "LVALUE")
		{
			return $$content;
		}
		else
		{
			return $content;
		}
	}
	elsif ($type eq '@' or $type eq "ARRAY")
	{
		if (!defined $content)
		{
			return ();
		}
		if (ref($content) eq "ARRAY")
		{
			return @$content;
		}
		elsif (ref($content) eq "LVALUE")
		{
			return split /\n/, $$content;
		}
		else
		{
			return split /\n/, $content;
		}
	}
	elsif ($type eq '%' or $type eq "HASH")
	{
		if (!defined $content)
		{
			return {};
		}
		if (ref($content) eq "HASH")
		{
			return %$content;
		}
		elsif (ref($content) eq "LVALUE")
		{
			my %rt = {};
			for my $line (split /\n/, $$content) {
				my ($key, $value) = split /\t/, $line, 2;
				$rt{ $key} = $value;
			}
			return %rt;
		}
		else
		{
			my %rt = {};
			for my $line (split /\n/, $content) {
				my ($key, $value) = split /\t/, $line, 2;
				$rt{ $key} = $value;
			}
			return %rt;
		}
	}
	else
	{
		print "undefined result type: $type\n";
		exit 1;
	}
}

sub issueRequest {
	my ($method, $url, $content) = @_;
	if ($content)
	{
		$content = readInput( $content);
	}
	my $headers = {Accept => 'application/json'};
	my $client = REST::Client->new();
	my $server;
	my $path;
	
	my $ua = LWP::UserAgent->new;
	$ua->agent("strus/0.0");
	
	my $req = HTTP::Request->new( $method => $url );
	$req->content_type('application/json');
	$req->header( Accept => 'application/json' );
	if ($content) {
		$req->content( $content);
	}
	my $res = $ua->request($req);

	my $rescode = 200;
	if ($res->status_line =~ /^([0-9]+)[ ]/)
	{
		$rescode = $1;
	}
	if ($res->is_success) {
		return ($rescode, from_json( $res->content));
	} else {
		return ($rescode, undef);
	}
}

sub printResult {
	my ($node, $indent) = @_;
	if (!defined $indent) {
		$indent = "";
	}
	my $nodetype = ref($node);
	if ($nodetype eq "HASH") {
		my %nodemap = %$node;
		foreach my $key (keys %nodemap)
		{
			my $subnode = $nodemap{$key};
			print "$indent $key:";
			printResult( $subnode, $indent . "   ");
		}
	}
	elsif ($nodetype eq "ARRAY") {
		my @nodear = @$node;
		for my $subnode (@nodear) {
			print $indent . "-";
			printResult( $subnode, $indent . "   ");
		}
		print "\n";
	}
	elsif ($nodetype eq "LVALUE") {
		my $nodeval = $$node;
		print "$indent$$nodeval";
	}
	else
	{
		print "$indent$node";
	}
}

sub jsonEncodeValue {
	my ($str) = @_;
	 
	$str =~ s/([\\][bfnrt"\\])/\\\\$1/g;
	return $str;
}

sub serializationToJson {
	my (@serialization) = @_;
	my $name = undef;
	my $rt = "{";
	my $indent = "\n";
	my @stk = ();
	for (my $idx=0; $idx <= $#serialization; $idx++) {
		my $elem = $serialization[ $idx];
		my $tag = substr( $elem, 0, 1);
		my $val = jsonEncodeValue( substr( $elem, 1));
		if ($tag eq "(")
		{
			if (defined $name)
			{
				my $next_tag = "";
				if ($idx +1 <= $#serialization)
				{
					$next_tag = substr( $serialization[ $idx+1], 0, 1);
				}
				if ($next_tag eq "(" or $next_tag eq "=")
				{
					push( @stk, 1);
					$rt .= "$indent\"$name\": \[";
				}
				else
				{
					push( @stk, 0);
					$rt .= "$indent\"$name\": \{";
				}
				$name = undef;
			}
			else
			{
				push( @stk, 0);
				$rt .= "$indent\{";
			}
			$indent .= "  ";
		}
		elsif ($tag eq "=")
		{
			my $sep = "";
			if ($idx +1 <= $#serialization)
			{
				my $next_tag = substr( $serialization[ $idx+1], 0, 1);
				if ($next_tag ne ')') {
					$sep = ',';
				}
			}
			if (defined $name)
			{
				$rt .= "$indent\"$name\":\"$val\"$sep";
				$name = undef;
			}
			else
			{
				$rt .= "\"$val\"$sep";
			}
		}
		elsif ($tag eq ":")
		{
			$name = $val;
		}
		elsif ($tag eq ")")
		{
			my $sep = "";
			my $next_tag = "";
			if ($idx +1 <= $#serialization)
			{
				$next_tag = substr( $serialization[ $idx+1], 0, 1);
				if ($next_tag ne ')') {
					$sep = ',';
				}
			}
			$indent = substr( $indent, 0, -2);
			if ($stk[$#stk] == 1)
			{
				$rt .= "\]$sep";
			}
			else
			{
				$rt .= "\}$sep";
			}
			pop( @stk);
			$name = undef;
		}
		else
		{
			print "unknown tag: $tag\n";
			exit 1;
		}
	}
	$rt .= "\n}\n";
	return $rt;
}

sub selectResult {
	my ($node, @selar) = @_;

	my $nodetype = ref($node);
	if ($nodetype eq "HASH") {
		if ($#selar < 0)
		{
			return $node;
		}
		my $firstsel = $selar[ 0];
		my @choice = split /,/, $firstsel;
		my @result = ();
		foreach my $selkey( @choice) {
			my %nodemap = %$node;
			my $found = undef;
			foreach my $nodekey (keys %nodemap)
			{
				if ($selkey eq $nodekey) {
					$found = 1;
					my $subnode = $nodemap{$nodekey};
					push( @result, selectResult( $subnode, @selar[ 1 .. $#selar] ));
				}
			}
			if (!defined $found)
			{
				push( @result, undef);
			}
		}
		if ($#result < 0)
		{
			return undef;
		}
		elsif ($#result == 0)
		{
			return $result[0];
		}
		else
		{
			return \@result;
		}
	}
	elsif ($nodetype eq "ARRAY") {
		my @result = ();
		my @nodear = @$node;
		for my $subnode (@nodear) {
			push( @result, selectResult( $subnode, @selar));
		}
		return \@result;
	}
	elsif ($nodetype eq "LVALUE") {
		if ($#selar >= 0)
		{
			return undef;
		}
		return $$node;
	}
	else
	{
		return $node;
	}
}

1;

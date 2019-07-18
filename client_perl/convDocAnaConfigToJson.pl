#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

use Strus::Client;
use JSON;

my $section = undef;
my %sectionNameMap = (
    "content" => "content",
    "document" => "document",
    "searchindex" => "search",
    "forwardindex"  => "forward",
    "metadata"  => "metadata",
    "attribute"  => "attribute",
    "patternlexem" => "lexem",
    "aggregator"  => "aggregator",
    "patternmatch" => "NOT_IMPLEMENTED"
);

my $indentstr = "";
my $json = JSON->new->allow_nonref;
my @cntstk = ();

sub STARTDOC {
	print( "{");
}
sub ENDDOC {
	print( "\n}");
}
sub NAME {
	my ($tagname) = @_;
	print( "\n$indentstr\"$tagname\": ");
}
sub OPEN {
	print( "{");
	$indentstr = $indentstr . "\t";
	push( @cntstk, 0);
}
sub CLOSE {
	$indentstr = substr( $indentstr, 0, -1);
	print( "\n$indentstr}");
	pop( @cntstk);
}
sub OPENAR {
	print( "[");
	$indentstr = $indentstr . "\t";
	push( @cntstk, 0);
}
sub CLOSEAR {
	$indentstr = substr( $indentstr, 0, -1);
	print( "\n$indentstr]");
	pop( @cntstk);
}
sub DELIM {
	my $cnt = $cntstk[$#cntstk]; 
	if ($cnt >= 1) { print( ","); }
	++$cntstk[$#cntstk];
}
sub VALUE {
	my ($val) = @_;
	if ($val =~ m/^[\-]{0,1}[0-9]+$/ )
	{
		print( $val);
	}
	elsif ($val =~ m/^[\-]{0,1}[0-9]+[\.][0-9]{0,24}$/ )
	{
		print( $val);
	}
	else
	{
		my $encval = $json->encode( $val);
		print( $encval);
	}
}

my $g_src = "";
my $g_token = "";
my $g_linecnt = 0;

sub lexerAddSource
{
	my ($s) = @_;
	$g_src .= $s;
}
sub lexerStartToken
{
	$g_token = "";
}
sub lexerSkipSpaces
{
	if ($g_src =~ m/^[ \t]+(.*)$/)
	{
		$g_src = $1;
	}
}
sub lexerTok
{
	my ($t,$s) = @_;
	$g_token .= $t;
	$g_src = $s;
}

sub lexerParseTokenList_;
sub lexerParseToken_
{
	lexerSkipSpaces();
	if ($g_src =~ m/^([a-zA-Z0-9_\.]+)(.*)$/)
	{
		lexerTok( $1, $2);
		if ($g_src =~ m/^([\(])(.*)$/)
		{
			lexerTok( $1, $2);
			lexerSkipSpaces();
			if ($g_src =~ m/^([\)])(.*)/)
			{
				lexerTok( $1, $2);
			}
			else
			{
				lexerParseTokenList_();
			}
		}
	}
	elsif ($g_src =~ m/^([\/].*)$/)
	{
		lexerTok( $1, "");
	}
	elsif ($g_src =~ m/^([\"][^\"]*[\"])(.*)$/)
	{
		lexerTok( $1, $2);
	}
	elsif ($g_src =~ m/^([-][>])(.*)$/)
	{
		lexerTok( $1, $2);
	}
	elsif ($g_src =~ m/^([\{\}\=\:\^])(.*)$/)
	{
		lexerTok( $1, $2);
	}
	elsif ($g_src =~ m/\S/)
	{
		die "syntax error at line $g_linecnt >> $g_src\n";
	}
	else
	{
		die "unexpected end of command at line $g_linecnt\n";
	}
	return $g_token;
}
sub lexerParseTokenList_
{
	lexerParseToken_();
	if ($g_src =~ m/^([\)])(.*)/)
	{
		lexerTok( $1, $2);
	}
	elsif ($g_src =~ m/^([\,])(.*)/)
	{
		lexerTok( $1, $2);
		lexerParseTokenList_();
	}
	else
	{
		die "syntax error at line $g_linecnt >> $g_src\n";
	}
}
sub lexerParseToken
{
	lexerStartToken();
	lexerParseToken_();
	return $g_token;
}
sub lexerEof
{
	if ($g_src =~ m/\S/)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

sub fetchArg
{
	my ($src) = @_;
	if ($src =~ m/^\s*([a-zA-Z0-9_\.]+)(.*)$/)
	{
		return ($1,$2);
	}
	elsif ($src =~ m/^\s*[\"]([^\"]*)[\"](.*)$/)
	{
		return ($1,$2);
	}
	else
	{
		die "unexpected argument in function at line $g_linecnt >> $src";
	}
}
sub fetchComma
{
	my ($src) = @_;
	if ($src =~ m/^(\s*[,])(.*)$/)
	{
		return $2;
	}
	else
	{
		die "comma expected as separator of function argument list at line $g_linecnt >> $src";
	}
}

sub DEF_FUNCTION
{
	my ($def) = @_;
	my $nam = undef;
	my @arg = ();

	if ($def =~m/([a-zA-Z0-9_]+)[\(](.*)$/)
	{
		$nam = $1;
		$def = $2;
		$def =~ s/[\)]$//;
		if ($def =~ m/\S/)
		{
			my ($aa,$def) = fetchArg( $def);
			push @arg, $aa;
			while ($def =~ m/\S/)
			{
				$def = fetchComma( $def);
				my ($aa,$def) = fetchArg( $def);
				push @arg, $aa;
			}
		}
	}
	else
	{
		$nam = $def;
	}
	DELIM(); NAME("name"); VALUE($nam); 
	if ($#arg >= 0)
	{
		DELIM(); NAME("arg"); OPENAR();
		for my $aa( @arg) {
			DELIM(); VALUE( $aa);
		}
		CLOSEAR();
	}
}

STARTDOC();
NAME( "analyzer"); OPEN();
NAME( "doc"); OPEN();
DELIM(); NAME( "class"); OPEN(); NAME( "mimetype"); VALUE("application/xml"); CLOSE();
my $in_feature_section = 0;

while (<>)
{
	++$g_linecnt;
	chomp;
	my $ln = $_;
	if (m/^[\#]/)
	{
		next; # ... comment
	}
	elsif (m/^[\[]([^\]]*)[\]]$/)
	{
		my $sectionName = lc( $1);
		if ($g_src =~ m/\S/)
		{
			die "line not terminated at line $g_linecnt >> $g_src";
		}
		if (defined $section)
		{
			CLOSEAR();
		}
		$section = $sectionNameMap{ $sectionName};
		unless (defined $section) {
			die "unknown section class name '$sectionName' at line $g_linecnt\n";
		}
		if ($section eq "NOT_IMPLEMENTED")
		{
			die "not implemented section class name '$sectionName' at line $g_linecnt\n";
		}
		if ($section eq "patternmatch" || $section eq "content" || $section eq "document")
		{
			if ($in_feature_section)
			{
				CLOSE();
				$in_feature_section = 0;
			}
		}
		else
		{
			unless ($in_feature_section)
			{
				DELIM(); NAME("feature"); OPEN();
				$in_feature_section = 1;
			}
		}
		DELIM(); NAME( $section); OPENAR();
	}
	elsif (m/^(.*)[\;]$/)
	{
		$ln = $1;
		lexerAddSource( $ln);
		if (!defined $section)
		{
			$section = "search";
			DELIM(); NAME( $section); OPENAR();
		}
		if ($section eq "forward" || $section eq "search" || $section eq "attribute" || $section eq "metadata" || $section eq "lexem")
		{
			my $name = lexerParseToken();
			my $priority = undef;
			my $opr = lexerParseToken();
			if ($opr eq '^')
			{
				$priority = lexerParseToken();
				$opr = lexerParseToken();
			}
			if ($opr ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '=' >> $opr"; }
			my @normalizers = ();
			my $normalizer = lexerParseToken();
			push( @normalizers, $normalizer);
			my $tokenizer = lexerParseToken();
			while ($tokenizer eq ":")
			{
				$normalizer = lexerParseToken();
				push( @normalizers, $normalizer);
				$tokenizer = lexerParseToken();
			}
			my $expression = lexerParseToken();
			my %options;
			my $nofOptions = 0;
			if ($expression eq '{') {
				my $dl = ',';
				while ($dl eq ',') {
					my $optnam = lexerParseToken();
					if (lexerParseToken() ne "=") { die "syntax error in $optnam option at line $g_linecnt"; }
					my $optval = lexerParseToken();
					$options{ $optnam} = $optval;
					++$nofOptions;
					$dl = lexerParseToken();
				}
				if ($dl ne "}") { die "syntax error at end of options at line $g_linecnt, expected '}' >> $dl"; }
				$expression = lexerParseToken();
			}
			if (!lexerEof()) { die "unexpected tokens at end of expression at line $g_linecnt >> $g_src"; }

			DELIM(); OPEN();
			DELIM(); NAME( "type"); VALUE( $name); 
			DELIM(); NAME( "tokenizer"); OPEN(); DEF_FUNCTION( $tokenizer); CLOSE();
			DELIM(); NAME( "normalizer"); OPENAR(); 
			foreach $normalizer( @normalizers)
			{
				DELIM(); OPEN(); DEF_FUNCTION( $normalizer); CLOSE();
			}
			CLOSEAR();
			if (defined $priority) {
				DELIM(); NAME( "option"); VALUE( $priority);
			}
			if ($nofOptions != 0)
			{
				DELIM(); NAME( "option");
				OPEN();
				foreach my $optkey (keys %options) {
					DELIM(); NAME( $optkey); VALUE( $options{ $optkey} );
				}
				CLOSE();
			}
			DELIM(); NAME( "select"); VALUE( $expression);
			CLOSE();
		}
		elsif ($section eq "aggregator")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $function = lexerParseToken();
			DELIM(); OPEN(); 
			DELIM(); NAME( "type"); VALUE( $name); 
			DELIM(); NAME( "function"); OPEN();

			if ($function =~ m/^([a-zA-Z0-9_]+)$/)
			{
				my $funcname = $1;
				DELIM(); NAME("name"); VALUE( $funcname);
			}
			elsif ($function =~ m/^([a-zA-Z0-9_]+)[\(](.*)[\)]$/)
			{
				my $funcname = $1;
				my @arg = split /[\,]/, $2;
				DELIM(); NAME("name"); VALUE( $funcname); 
				DELIM(); NAME("arg"); OPENAR();
				for my $aa( @arg) {
					DELIM(); VALUE( $aa);
				}
				CLOSEAR();
			}
			CLOSE();
			CLOSE();
		}
		elsif ($section eq "content")
		{
			my $documentClass = lexerParseToken();
			my $expression = lexerParseToken();
			my $encoding = undef;
			my $mimetype = undef;
			my $schema = undef;
			if ($documentClass =~ m/^([^=]*)$/)
			{
				$mimetype = $1;
			}
			else
			{
				if ($documentClass =~ m/(content|mimetype)[=]([^;]*)$/)
				{
					$mimetype = $2;
				}
				if ($documentClass =~ m/(encoding|charset)[=]([^;]*)$/)
				{
					$encoding = $2;
				}
				if ($documentClass =~ m/schema[=]([^;]*)$/)
				{
					$schema = $1;
				}
			}
			DELIM(); OPEN();
			DELIM(); NAME( "select"); VALUE( $expression); 
			if (defined $mimetype)
			{
				DELIM(); NAME( "mime"); VALUE( $mimetype);
			}
			else
			{
				die "MIME type not defined in document class at line $g_linecnt\n";
			}
			if (defined $encoding)
			{
				DELIM(); NAME( "encoding"); VALUE( $encoding);
			}
			if (defined $schema)
			{
				DELIM(); NAME( "schema"); VALUE( $schema);
			}
			CLOSE();
		}
		elsif ($section eq "document")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $expression = lexerParseToken();

			DELIM(); OPEN();
			DELIM(); NAME( "name"); VALUE( $name); 
			DELIM(); NAME( "select"); VALUE( $expression); 
			CLOSE();
		}
		else
		{
			die "unhandled section class name '$section' at line $g_linecnt\n";
		}
		$g_src = "";
	}
	else
	{
		lexerAddSource( $ln);
	}
}
if (defined $section)
{
	CLOSEAR();
}
if ($in_feature_section)
{
	CLOSE();
}
CLOSE();
CLOSE();
ENDDOC();


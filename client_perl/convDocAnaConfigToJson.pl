#!/usr/bin/perl
# ------------------------------------------
# Get the document features of a given type 
# ------------------------------------------
use strict;
use warnings;

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
    "field"  => "field",
    "structure"  => "structure",
    "patternmatch" => "NOT_IMPLEMENTED"
);

my $indentstr = "";
my $json = JSON->new->allow_nonref;
my @cntstk = (1);

my %outputmap = ();
my $curchan = "search";

sub print_chan
{
	my ($arg) = @_;
	$outputmap{ $curchan} .= $arg;
}
sub STARTDOC {
	print_chan( "{");
}
sub ENDDOC {
	print_chan( "\n}");
}
sub NAME {
	my ($tagname) = @_;
	print_chan( "\n$indentstr\"$tagname\": ");
}
sub OPEN {
	print_chan( "{");
	$indentstr = $indentstr . "\t";
	push( @cntstk, 0);
}
sub CLOSE {
	$indentstr = substr( $indentstr, 0, -1);
	print_chan( "\n$indentstr}");
	pop( @cntstk);
}
sub OPENAR {
	print_chan( "[");
	$indentstr = $indentstr . "\t";
	push( @cntstk, 0);
}
sub CLOSEAR {
	$indentstr = substr( $indentstr, 0, -1);
	print_chan( "\n$indentstr]");
	pop( @cntstk);
}
sub DELIM {
	my $cnt = $cntstk[$#cntstk]; 
	if ($cnt >= 1) { print_chan( ","); }
	++$cntstk[$#cntstk];
}
sub INIT_DELIM {
	if (exists $outputmap{ $curchan})
	{
		DELIM();
	}
}
sub VALUE {
	my ($val) = @_;
	if ($val =~ m/^[\-]{0,1}[0-9]+$/ )
	{
		print_chan( $val);
	}
	elsif ($val =~ m/^[\-]{0,1}[0-9]+[\.][0-9]{0,24}$/ )
	{
		print_chan( $val);
	}
	else
	{
		my $encval = $json->encode( $val);
		print_chan( $encval);
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
	elsif ($g_src =~ m/^([\/\@][^;,{} ]*)(.*)$/)
	{
		lexerTok( $1, $2);
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

# --- Parse input:
while (<>)
{
	++$g_linecnt;
	chomp;
	my $ln = $_;

	if (m/^\s*[\#]/)
	{
		next; # ... comment
	}
	elsif (m/^[\[]([^\]]*)[\]](.*)$/)
	{
		my $sectionName = lc( $1);
		my $rest = $2;
		$rest =~ s/^\s+|\s+$//g;
		if ($rest ne "") {
			die "line not terminated at line $g_linecnt >> $rest";
		}
		$curchan = $sectionNameMap{ $sectionName};
		unless (defined $curchan) {
			die "unknown section class name '$sectionName' at line $g_linecnt\n";
		}
		if ($curchan eq "NOT_IMPLEMENTED")
		{
			die "not implemented section class name '$sectionName' at line $g_linecnt\n";
		}
	}
	elsif (m/^(.*)[\;]$/)
	{
		$ln = $1;
		lexerAddSource( $ln);
		if ($curchan eq "forward" || $curchan eq "search" || $curchan eq "attribute" || $curchan eq "metadata" || $curchan eq "lexem")
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

			INIT_DELIM(); OPEN();
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
		elsif ($curchan eq "aggregator")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $function = lexerParseToken();
			INIT_DELIM(); OPEN(); 
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

			if (!lexerEof()) { die "missing ';' at end of aggregator definition at line $g_linecnt >> $g_src"; }
		}
		elsif ($curchan eq "content")
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
			INIT_DELIM(); OPEN();
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

			if (!lexerEof()) { die "missing ';' at end of content definition at line $g_linecnt >> $g_src"; }
		}
		elsif ($curchan eq "document")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $expression = lexerParseToken();

			INIT_DELIM(); OPEN();
			DELIM(); NAME( "name"); VALUE( $name); 
			DELIM(); NAME( "select"); VALUE( $expression); 
			CLOSE();

			if (!lexerEof()) { die "missing ';' at end of document definition at line $g_linecnt >> $g_src"; }
		}
		elsif ($curchan eq "field")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $scopeexpr = lexerParseToken();
			my $selectexpr = lexerParseToken();
			my $idexpr = "";
			if (!lexerEof()) {$idexpr = lexerParseToken();}
			if (!lexerEof()) { die "missing ';' at end of field definition at line $g_linecnt >> $g_src"; }

			INIT_DELIM(); OPEN();
			DELIM(); NAME( "name"); VALUE( $name); 
			DELIM(); NAME( "scope"); VALUE( $scopeexpr); 
			DELIM(); NAME( "select"); VALUE( $selectexpr); 
			if ($idexpr ne "") {DELIM(); NAME( "key"); VALUE( $idexpr);}
			CLOSE();
		}
		elsif ($curchan eq "structure")
		{
			my $name = lexerParseToken();
			if (lexerParseToken() ne "=") { die "syntax error at line $g_linecnt, expected assignment operator '='"; }
			my $headername = lexerParseToken();
			my $contentname = lexerParseToken();
			my $classname = lexerParseToken();

			if (!lexerEof()) { die "missing ';' at end of structure definition at line $g_linecnt >> $g_src"; }
	
			INIT_DELIM(); OPEN();
			DELIM(); NAME( "name"); VALUE( $name); 
			DELIM(); NAME( "header"); VALUE( $headername);
			DELIM(); NAME( "content"); VALUE( $contentname);
			DELIM(); NAME( "class"); VALUE( $classname);
			CLOSE();
		}
		else
		{
			die "unhandled section class name '$curchan' at line $g_linecnt\n";
		}
		$g_src = "";
	}
	else
	{
		lexerAddSource( $ln);
	}
}

$curchan = "ALL";

sub print_section {
	my ($arg) = @_;
	if ($outputmap{ $arg}) {
		my $content = $outputmap{ $arg};
		$content =~ s/\n/\n\t$indentstr/g;
		DELIM(); NAME( $arg); OPENAR(); print_chan( $content); CLOSEAR();
	}
}

# --- Output result:
STARTDOC();
NAME( "docanalyzer"); OPEN();
DELIM(); NAME( "class"); OPEN(); 
	NAME( "mimetype"); VALUE("application/xml");
CLOSE();
print_section( "content");
print_section( "document");
DELIM(); NAME("feature"); OPEN();
	print_section( "forward");
	print_section( "search");
	print_section( "attribute");
	print_section( "aggregator");
	print_section( "metadata");
	print_section( "lexem");
CLOSE();
print_section( "field");
print_section( "structure");
CLOSE();
ENDDOC();

print( $outputmap{ "ALL"} . "\n");


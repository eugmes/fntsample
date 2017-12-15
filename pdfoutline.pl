#! /usr/bin/perl -w
# This file is in public domain
# Author: Eugeniy Meshcheryakov <eugen@debian.org>
#
# This program adds outlines to pdf files.
# Usage: pdfoutline input.pdf outline.txt out.pdf
#
# File given as second argument should contain outline information in
# form:
#
# <level> <page> Some text
#
# where <level> and <page> are integers. Values for <level> should be greater
# or equal than that of first line. Page numeration starts with 1.
#
# Outlines file can contain comments that start with # in first column. Comments
# and empty lines are ignored.
#
# Example file:
# 0 1 Document title
# 1 1 Chapter 1
# 2 1 Chapter 1.1
# 2 2 Chapter 1.2
# 1 3 Chapter 2
#
# This file will result in outline like the following:
#
# Document title
# +-Chapter 1
# | +-Chapter 1.1
# | +-Chapter 1.2
# +-Chapter 2

use strict;
use PDF::API2;
use Locale::TextDomain('@CMAKE_PROJECT_NAME@', '@CMAKE_INSTALL_FULL_LOCALEDIR@');
use POSIX qw(:locale_h);
use Encode qw(encode);
use subs qw(add_outlines);

sub usage() {
	printf(__"Usage: %s input.pdf outline.txt out.pdf\n", $0);
}

# get first non-empty non-comment line
sub get_line($) {
	my $F = shift;
	my $line;

	while ($line = <$F>) {
		chomp $line;
		# skip comments ...
		next if $line =~ /^#/;
		# ... and empty lines
		next if $line =~ /^$/;
		last;
	}
	return $line;
}

# Encode string to UTF-16BE with BOM if it contains non-ASCII characters
sub encode_pdf_text($) {
	my $str = shift;

	if ($str !~ /[^[:ascii:]]/) {
		return $str;
	} else {
		return encode("UTF-16", $str);
	}
}

sub add_outlines($$$$) {
	my $pdf = shift;
	my $parent = shift;
	my $line = shift;
	my $F = shift;
	my $cur_outline;

	my ($level) = split / /, $line;

	MAINLOOP: while ($line) {
		my ($new_level, $page, $text) = split / /, $line, 3;

		if ($new_level > $level) {
			$line = add_outlines($pdf, $cur_outline, $line, $F);
			next MAINLOOP;
		}
		elsif ($new_level < $level) {
			return $line;
		}
		else {
			$cur_outline = $parent->outline;
			$cur_outline->title(encode_pdf_text($text));
			# FIXME it should be posible to make it easier
			my $pdfpage = $pdf->{pagestack}->[$page - 1];
			$cur_outline->dest($pdfpage);
		}

		$line = get_line($F);
	}
}

setlocale(LC_ALL, '');

if ($#ARGV != 2) {
	usage;
	exit 1;
}

my $inputfile = $ARGV[0];
my $outlinefile = $ARGV[1];
my $outputfile = $ARGV[2];
my $pdf = PDF::API2->open($inputfile);
open(OUTLINE, "<:encoding(UTF-8)", $outlinefile) or die __x("Cannot open outline file '{outlinefile}'", outlinefile => $outlinefile);
my $line = get_line(*OUTLINE);
add_outlines($pdf, $pdf->outlines, $line, *OUTLINE) if $line;
$pdf->saveas($outputfile);
exit 0;

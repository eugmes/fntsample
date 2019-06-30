#! /usr/bin/env perl
# This file is in the public domain
# Author: Ievgenii Meshcheriakov <eugen@debian.org>
#
# This program extracts outlines from PDF files. The outlines
# are stored into a text file that could be used with pdfoutline
# program.
#
# Usage: pdf-extract-outline input.pdf outline.txt

use strict;
use warnings;
use feature qw(say);
use PDF::API2;
use Locale::TextDomain('@CMAKE_PROJECT_NAME@', '@CMAKE_INSTALL_FULL_LOCALEDIR@');
use POSIX qw(:locale_h);
use Encode qw(decode find_encoding);
use Encode::Guess;
use List::MoreUtils qw(first_index);

my $fallback_encoding = 'PDFDocumentEncoding';

eval {
	require Encode::PDFDocumentEncoding;
	Encode::Guess->set_suspects(qw/PDFDocumentEncoding/);
	1;
} or do {
	warn 'Encode::PDFDocumentEncoding is missing, falling back to ASCII';
	$fallback_encoding = 'ascii';
};

sub decode_pdf {
	my ($s) = @_;

	eval {
		decode('Guess', $s);
	} or do {
		decode $fallback_encoding, $s, sub {
			my $code = shift;
			my $repr = sprintf "\\x%02X", $code;
			warn "$fallback_encoding \"$repr\" does not map to Unicode";
			return q{?};
		};
	}
}

sub usage {
	printf __"Usage: %s input.pdf outline.txt\n", $0;
}

sub search_tree {
	my ($tree, $key) = @_;

	if ($tree->{'Limits'}) {
		my ($first, $last) = @{$tree->{'Limits'}->val};
		return if (($key lt $first->val) or ($key gt $last->val));
	}

	if ($tree->{'Names'}) {
		my @arr = @{$tree->{'Names'}->val};
		for (my $i = 0; $i < $#arr; $i += 2) {
			return $arr[$i + 1] if ($arr[$i]->val eq $key);
		}
	}

	if ($tree->{'Kids'}) {
		foreach my $kid (@{$tree->{'Kids'}->val}) {
			my $result = search_tree($kid->val, $key);
			return $result if $result;
		}
	}

	return;
}

sub extract_outlines {
	my ($pdf, $level, $outline, $F) = @_;

	OUTLINE: for (; $outline; $outline = $outline->{'Next'}) {
		$outline = $outline->val;

		my $raw_title = $outline->{'Title'}->val;
		my $title = decode_pdf($raw_title);
		my $dest;

		if ($outline->{'Dest'}) {
			$dest = $outline->{'Dest'};
		} elsif ($outline->{'A'}) {
			my $a = $outline->{'A'}->val;
			# TODO Search for GoTo entry
			if ($a->{'S'}->val eq 'GoTo') {
				$dest = $a->{'D'};
			} else {
				warn 'Action is not GoTo';
				next OUTLINE;
			}
		} else {
			warn "No Dest or A entry for '$title'";
			next OUTLINE;
		}

		if (ref($dest) eq 'PDF::API2::Basic::PDF::Name') {
			# Find the destination in Dest dictionary in Root object.
			my $named_ref = $dest->val;
			my $dests = $pdf->{'pdf'}->{'Root'}->{'Dests'}->val;
			$dest = $dests->{$named_ref};
		} elsif (ref($dest) eq 'PDF::API2::Basic::PDF::String') {
			# Find the destination in Dest tree in Names dictionary of Root object
			my $names = $pdf->{'pdf'}->{'Root'}->{'Names'}->val;
			my $tree = $names->{'Dests'}->val;
			my $name = $dest->val;
			$dest = search_tree($tree, $name);
			unless ($dest) {
				warn "No Dest found with name '$name'";
				next OUTLINE;
			}
		}

		if (ref($dest) eq 'PDF::API2::Basic::PDF::Objind') {
			$dest = $dest->val;
		}

		if (ref($dest) eq 'PDF::API2::Basic::PDF::Dict') {
			$dest = $dest->{'D'};
		}

		if (ref($dest) eq 'PDF::API2::Basic::PDF::Array') {
			$dest = $dest->val;
		}

		unless ($dest) {
			warn "Destination not found for '$title'";
			next OUTLINE;
		}

		my $page = $dest->[0];
		my $page_no;

		if (ref($page) eq 'PDF::API2::Basic::PDF::Number') {
			# Some documents use numbers even for pages in the current document
			$page_no = $page->val + 1;
		} else {
			my $page_idx = first_index { $_ == $page } @{$pdf->{'pagestack'}};

			if ($page_idx == -1) {
				warn "Page not found in the page stack for '$title'";
				next OUTLINE;
			}

			$page_no = $page_idx + 1;
		}

		print {$F} "$level $page_no $title\n";

		my $sub_outlines = $outline->{'First'};
		if ($sub_outlines) {
			extract_outlines($pdf, $level + 1, $sub_outlines, $F);
		}
	}
}

setlocale(LC_ALL, q{});

if ($#ARGV != 1) {
	usage;
	exit 1;
}

my ($pdffile, $outlinefile) = @ARGV;

my $pdf = PDF::API2->open($pdffile);

open my $outline_fh, '>:encoding(UTF-8)', $outlinefile
	or die __x("Cannot open outline file '{outlinefile}'",
	outlinefile => $outlinefile);

my $outlines = $pdf->{'pdf'}->{'Root'}->{'Outlines'};

if ($outlines) {
	if ($pdf->{'pdf'}->{'Encrypt'}) {
		die __('Extracting outlines from encrypted files is not supported');
	}

	my $first = $outlines->val->{'First'};
	extract_outlines($pdf, 0, $first, $outline_fh);
}

close $outline_fh;

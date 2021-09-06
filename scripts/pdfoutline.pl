#! /usr/bin/env perl
# Author: Ievgenii Meshcheriakov <eugen@debian.org>
# SPDX-License-Identifier: CC-PDDC
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
use warnings;
use PDF::API2;
use Locale::TextDomain('@CMAKE_PROJECT_NAME@', '@CMAKE_INSTALL_FULL_LOCALEDIR@');
use POSIX qw(:locale_h);
use Encode qw(encode);

sub usage {
    printf __"Usage: %s input.pdf outline.txt out.pdf\n", $0;
}

# get first non-empty non-comment line
sub get_line {
    my ($F) = @_;
    my $line;

    while ($line = <$F>) {
        chomp $line;
        # skip comments ...
        next if $line =~ /^#/;
        # ... and empty lines
        next if $line eq q{};
        last;
    }
    return $line;
}

# Encode string to UTF-16BE with BOM if it contains non-ASCII characters
sub encode_pdf_text {
    my ($str) = @_;

    if ($str !~ /[^[:ascii:]]/) {
        return $str;
    } else {
        if (PDF::API2->VERSION ge "2.034") {
            # Perl PDF::API2 >= 2.034 already handles non-ASCII characters
            # automatically. This also avoids a bug before v2.040.
            # See: https://rt.cpan.org/Public/Bug/Display.html?id=33497
            return $str;
        } else {
            # Buggy before PDF::API2 v2.040.
            # See: https://rt.cpan.org/Public/Bug/Display.html?id=134957
            return encode('UTF-16', $str);
        }
    }
}

sub add_outlines {
    my ($pdf, $parent, $line, $F) = @_;
    my $cur_outline;

    my ($level) = split / /, $line;

    MAINLOOP: while ($line) {
        my ($new_level, $page, $text) = split / /, $line, 3;

        if ($new_level > $level) {
            $line = add_outlines($pdf, $cur_outline, $line, $F);
            next MAINLOOP;
        } elsif ($new_level < $level) {
            return $line;
        } else {
            $cur_outline = $parent->outline;
            $cur_outline->title(encode_pdf_text($text));
            # FIXME it should be posible to make it easier
            my $pdfpage = $pdf->{pagestack}->[$page - 1];
            $cur_outline->dest($pdfpage);
        }

        $line = get_line($F);
    }
}

# Create new outlines object ignorig outlines that can be
# already present in the PDF file.
sub new_outlines {
    my ($pdf) = @_;

    require PDF::API2::Outlines;
    $pdf->{'pdf'}->{'Root'}->{'Outlines'} = PDF::API2::Outlines->new($pdf);
    my $obj = $pdf->{'pdf'}->{'Root'}->{'Outlines'};

    $pdf->{'pdf'}->new_obj($obj) unless $obj->is_obj($pdf->{'pdf'});
    $pdf->{'pdf'}->out_obj($obj);
    $pdf->{'pdf'}->out_obj($pdf->{'pdf'}->{'Root'});

    return $obj;
}

setlocale(LC_ALL, q{});

if ($#ARGV != 2) {
    usage;
    exit 1;
}

if (PDF::API2->VERSION le "2.033") {
    print STDERR "Warning: Perl PDF::API2 v2.033 or earlier detected.\n";
    print STDERR "It's known to have an outline corruption bug!\n";
    print STDERR "See pdfoutline man page for more information.\n";
}

my ($inputfile, $outlinefile, $outputfile) = @ARGV;

my $pdf = PDF::API2->open($inputfile);

open my $outline_fh, '<:encoding(UTF-8)', $outlinefile
    or die __x("Cannot open outline file '{outlinefile}'",
               outlinefile => $outlinefile);

my $line = get_line($outline_fh);

# create new outlines here, don't try to use old ones
my $outlines = new_outlines($pdf);

add_outlines($pdf, $outlines, $line, $outline_fh) if $line;
close $outline_fh;

$pdf->saveas($outputfile);

exit 0;

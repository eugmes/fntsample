FntSample Manual
================

.. highlight:: sh

Usage
-----

.. code-block:: sh

    fntsample [ OPTIONS ] -f FONT-FILE -o OUTPUT-FILE
    fntsample -h

:program:`fntsample` can be used to generate font samples that show Unicode coverage of the font and
are similar in appearance to Unicode charts.
Samples can be saved into PDF (default) or PostScript file.

Options
-------

:program:`fntsample` supports the following options.

.. program:: fntsample

.. cmdoption:: --font-file, -f <FONT-FILE>

    Make samples of ``FONT-FILE``

.. cmdoption:: --font-index, -n <IDX>

    Font index for ``FONT-FILE`` specified using :option:`--font-file` option.
    Useful for files that contain multiple fonts, like TrueType Collections (.ttc).
    By default font with index 0 is used.

.. cmdoption:: --output-file, -o <OUTPUT-FILE>

    Write output to ``OUTPUT-FILE``.

.. cmdoption:: --other-font-file, -d <OTHER-FONT>

    Compare ``FONT-FILE`` with ``OTHER-FONT``.
    Glyphs added to ``FONT-FILE`` will be highlighted.

.. cmdoption:: --other-index, -m <IDX>

    Font index for ``OTHER-FONT`` specified using :option:`--other-font-file` option.

.. cmdoption:: --postscript-output, -s

    Use PostScript format for output instead of PDF.

.. cmdoption:: --svg, -g

    Use SVG format for output.
    The generated document contains one page.
    Use range selection options to specify which.

.. cmdoption:: --print-outline, -l

    Print document outlines data to standard output.
    This data can be used to add outlines (aka bookmarks) to resulting PDF file with
    :program:`pdfoutline` program.

.. cmdoption:: --include-range, -i <RANGE>

    Show characters in ``RANGE``.

.. cmdoption:: --exclude-range, -x <RANGE>

    Do not show characters in ``RANGE``.


.. cmdoption:: --style, -t <"STYLE: VAL">

    Set ``STYLE`` to value ``VAL``.
    Run :program:`fntsample` with option :option:`--help` to see list of styles and default values.

.. cmdoption:: --help, -h

    Display help text and exit.

Parameter ``RANGE`` for :option:`--include-range` and :option:`--exclude-range` can be given as one integer
or a pair of integers delimited by minus sign (-).
Integers can be specified in decimal, hexadecimal (0x...) or octal (0...) format.
One integer of a pair can be missing (-N can be used to specify all characters with
codes less or equal to N, and N- for all characters with codes greater or equal to N).
Multiple :option:`--include-range` and :option:`--exclude-range` options can be used.

Colors
------

Glyph cells can have one of several background colors.
Meaning of those colors is following:

* white --- normal glyph present in the font, this includes space glyphs that are usually invisible;
* gray --- this glyph is defined in Unicode but not present in the font;
* blue --- this is a control character;
* black --- this glyph is not defined in Unicode;
* yellow --- this is a new glyph (only when used with :option:`--other-font`).

Examples
--------

Make PDF samples for :file:`font.ttf` and write them to file :file:`samples.pdf`::

    fntsample -f font.ttf -o samples.pdf

Make PDF samples for :file:`font.ttf`, compare it with :file:`oldfont.ttf` and highlight new glyphs.
Write output to file :file:`samples.pdf`::

    fntsample -f font.ttf -d oldfont.ttf -o samples.pdf

Make PostScript samples for :file:`font.ttf` and write output to file :file:`samples.ps`.
Show only glyphs for characters with codes less then or equal to U+04FF but exclude U+0370-U+03FF::

    fntsample -f font.ttf -s -o samples.ps -i -0x04FF -x 0x0370-0x03FF

Make PDF samples for :file:`font.ttf` and save output to file :file:`samples.pdf` adding outlines to it::

    fntsample -f font.ttf -o temp.pdf -l > outlines.txt
    pdfoutline temp.pdf outlines.txt samples.pdf

.. seealso:: :doc:`pdfoutline`

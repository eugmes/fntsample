fntsample
=========

|build| |license|

.. |build| image:: https://travis-ci.org/eugmes/fntsample.svg?branch=master
    :target: https://travis-ci.org/eugmes/fntsample
.. |license| image:: https://img.shields.io/badge/License-GPL%20v3-blue.svg
    :alt: License: GPL v3
    :target: https://www.gnu.org/licenses/gpl-3.0

``fntsample`` is a tool that can be used to make font samples that show coverage of the font
and are similar in appearance to `Unicode Charts <https://www.unicode.org/charts/>`_.
It was developed for use with `DejaVu Fonts <https://dejavu-fonts.github.io>`_ project.
``fntsample`` is licensed under `GPL <https://www.gnu.org/licenses/gpl.html>`_ version 3 or later.

.. image:: screenshot.png
   :alt: Output example

Features
--------

* Support for various font formats using `FreeType <https://www.freetype.org>`_ library,
  including TrueType, OpenType, and Type1.

* Creating samples in PDF, PostScript, and SVG formats.

* Adding outlines with Unicode block names for PDF samples.

* Selection of code ranges to show in charts.

* Comparing of two font files with highlighting of added glyphs.

* Runs on Linux and other Unix-like systems.

Download
--------

Releases are available from `releases page <https://github.com/eugmes/fntsample/releases>`_.
For source code releases for versions before 5.0 visit the `old project page <https://sourceforge.net/projects/fntsample/>`_.
The source code and issues tracker are accessible via the `project page <https://github.com/eugmes/fntsample>`_.

Building
--------

The following libraries are required to build ``fntsample``:
`cairo <https://www.cairographics.org>`_,
`fontconfig <https://www.fontconfig.org>`_,
`FreeType2 <https://www.freetype.org>`_,
`GLib <https://developer.gnome.org/glib/>`_,
`Pango <http://www.pango.org/>`_.
They should be available in most Linux distributions.
Additionally Unicode `blocks <https://unicode.org/Public/UNIDATA/Blocks.txt>`_ file is required.

`CMake <https://cmake.org>`_ is used to build the code. In the directory with source code execute::

    % mkdir build
    % cd build
    % cmake .. -DUNICODE_BLOCKS=/path/to/Blocks.txt
    % make
    % make install

The last step will install files under ``/usr/local`` by default. This can be overridden by adding
``-DCMAKE_INSTALL_PREFIX=/another/prefix`` to the ``cmake`` invocation.

Usage
-----

The basic usage for ``fntsample`` looks as follows::

    % fntsample -f /file/to/font/file.ttf -o output.pdf

For more advanced usage consult the man pages for ``fntsample`` and ``pdfoutline``.

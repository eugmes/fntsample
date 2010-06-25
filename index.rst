Welcome
=======

FntSample is a tool that can be used to make font samples that show coverage of the font
and are similar in appearance to `Unicode Charts <http://www.unicode.org/charts/>`_.
It was developed by Eugeniy Meshcheryakov for use with
`DejaVu Fonts <http://dejavu-fonts.org/wiki/Main_Page>`_ project.
FntSample is licensed under `GPL <http://www.gnu.org/licenses/gpl.html>`_ version 3 or later.

Features
--------

* Support for various font formats using `FreeType <http://www.freetype.org/>`_ library,
  including TrueType, OpenType, and Type1.

* Creating samples in PDF and PostScript format.

* Adding outlines with Unicode block names for PDF samples.

* Selection of code ranges to show in charts.

* Comparing of two font files with highlighting of added glyphs.

* Runs on Linux and other Unix-like systems.

Getting It
----------

FntSample can be downloaded from the `project page <http://sourceforge.net/projects/fntsample>`_.
Development version is also available in Git repository::

     git://fntsample.git.sourceforge.net/gitroot/fntsample/fntsample

It is also possible to `browse <http://fntsample.git.sourceforge.net/git/gitweb.cgi?p=fntsample/fntsample;a=summary>`_
the repository.

For building fntsample the following libraries are needed:
`cairo <http://cairographics.org/>`_,
`fontconfig <http://www.fontconfig.org/wiki/>`_,
`FreeType2 <http://www.freetype.org/>`_,
`GLib <http://library.gnome.org/devel/glib/>`_,
and `Pango <http://www.pango.org/>`_.
They should be available in most Linux distributions.
Additionally Unicode `blocks <http://unicode.org/Public/UNIDATA/Blocks.txt>`_ file is required.

Using It
--------

Please see :doc:`fntsample` and :doc:`pdfoutline` for usage instructions.
Those documents also contain some examples.
Examples of charts produced with fntsample can be found on DejaVu Fonts
`PDF Samples <http://dejavu-fonts.org/wiki/PDF_samples>`_ page.

.. toctree::
   :hidden:

   fntsample
   pdfoutline

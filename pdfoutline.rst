PDFOutline Manual
=================

Usage
-----

.. code-block:: sh

    pdfoutline input.pdf outlines.txt output.pdf

:program:`pdfoutline` reads input file given as first argument, adds outlines
from text file given as second argument, and saves result to file with name given as third argument.

.. highlight:: text

File with outlines information should consist of lines in the following format::

    <level> <page> Outline text

``<level>`` and ``<page>`` should be integers.
Each field should be separated by exactly one space symbol.
All values for ``<level>`` should be greater or equal than that of the first line.
Page numeration starts with 1.

Outlines file can contain comments that start with # in first column.
Comments and empty lines are ignored.

.. program:: fntsample

:program:`fntsample` produces outline files if invoked with option :option:`--print-outline`.

Example
-------

Here is example of outlines data file::

    0 1 Document title
    1 1 Chapter 1
    2 1 Chapter 1.1
    2 2 Chapter 1.2
    1 3 Chapter 2

Using this file will result in outlines like the following:

    * Document title

      * Chapter 1

        * Chapter 1.1

        * Chapter 1.2

      * Chapter 2

.. seealso:: :doc:`fntsample`

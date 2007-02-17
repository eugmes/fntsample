AM_CFLAGS = -Wall -W -Wwrite-strings -Wstrict-prototypes -pedantic
bin_PROGRAMS = fntsample
dist_bin_SCRIPTS = pdfoutline

fntsample_SOURCES = fntsample.c unicode_blocks.h
nodist_fntsample_SOURCES = unicode_blocks.c
fntsample_CFLAGS = $(AM_CFLAGS) $(cairo_CFLAGS) $(freetype2_CFLAGS) $(glib_CFLAGS)
fntsample_LDFLAGS = -Wl,--as-needed
fntsample_LDADD = $(cairo_LIBS) $(freetype2_LIBS) $(glib_LIBS)

nodist_man_MANS = fntsample.1

EXTRA_DIST = genblocks.awk
CLEANFILES = unicode_blocks.c

unicode_blocks.c: genblocks.awk
	$(AWK) -f $(srcdir)/genblocks.awk $(UNICODE_BLOCKS) > unicode_blocks.c

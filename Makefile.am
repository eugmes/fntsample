AM_CFLAGS = -Wall -W -Wwrite-strings -Wstrict-prototypes -pedantic
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = fntsample
bin_SCRIPTS = pdfoutline

fntsample_SOURCES = fntsample.c unicode_blocks.h
nodist_fntsample_SOURCES = unicode_blocks.c
fntsample_CPPFLAGS = $(AM_CPPFLAGS) $(cairo_CFLAGS) $(fontconfig_CFLAGS) $(freetype2_CFLAGS) $(glib_CFLAGS) $(pangocairo_CFLAGS)
fntsample_LDFLAGS = @LD_AS_NEEDED@
fntsample_LDADD = @LIBICONV@ @LIBINTL@ -lm $(cairo_LIBS) $(fontconfig_LIBS) $(freetype2_LIBS) $(glib_LIBS) $(pangocairo_LIBS)

nodist_man_MANS = fntsample.1 pdfoutline.1

EXTRA_DIST = config.rpath genblocks.awk pdfoutline.pl po/Changes
CLEANFILES = unicode_blocks.c $(bin_SCRIPTS)

AWK_V = $(AWK_V_$(V))
AWK_V_ = $(AWK_V_$(AM_DEFAULT_VERBOSITY))
AWK_V_0 = @echo "  AWK   " $@;

SED_V = $(SED_V_$(V))
SED_V_ = $(SED_V_$(AM_DEFAULT_VERBOSITY))
SED_V_0 = @echo "  SED   " $@;


unicode_blocks.c: genblocks.awk
	$(AWK_V)$(AWK) -f $(srcdir)/genblocks.awk $(UNICODE_BLOCKS) > unicode_blocks.c

pdfoutline: pdfoutline.pl Makefile
	$(SED_V)$(SED) -e 's|##PACKAGE##|$(PACKAGE)|' -e 's|##LOCALEDIR##|$(localedir)|' $< > $@

SUBDIRS = po

/* Copyright (C) 2007 Eugeniy Meshcheryakov <eugen@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TYPE1_TABLES_H
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include <cairo-ft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <pango/pangocairo.h>

#include "unicode_blocks.h"

#define A4_WIDTH	(8.3*72)
#define A4_HEIGHT	(11.7*72)

#define xmin_border	(72.0/1.5)
#define ymin_border	(72.0)
#define cell_width	((A4_WIDTH - 2*xmin_border) / 16)
#define cell_height	((A4_HEIGHT - 2*ymin_border) / 16)

static struct option longopts[] = {
  {"font-file", 1, 0, 'f'},
  {"output-file", 1, 0, 'o'},
  {"help", 0, 0, 'h'},
  {"other-font-file", 1, 0, 'd'},
  {"postscript-output", 0, 0, 's'},
  {"print-outline", 0, 0, 'l'},
  {"include-range", 1, 0, 'i'},
  {"exclude-range", 1, 0, 'x'},
  {"style", 1, 0, 't'},
  {0, 0, 0, 0}
};

struct range {
	uint32_t first;
	uint32_t last;
	bool include;
	struct range *next;
};

static const char *font_file_name;
static const char *other_font_file_name;
static const char *output_file_name;
static bool postscript_output;
static bool print_outline;
static struct range *ranges;
static struct range *last_range;

struct fntsample_style {
	const char *const name;
	const char *const default_val;
	char *val;
};

static struct fntsample_style styles[] = {
	{ "header-font", "Sans Bold 12", NULL },
	{ "font-name-font", "Serif Bold 12", NULL },
	{ "table-numbers-font", "Sans 10", NULL },
	{ "cell-numbers-font", "Mono 8", NULL },
	{ NULL, NULL, NULL }
};

static PangoFontDescription *header_font;
static PangoFontDescription *font_name_font;
static PangoFontDescription *table_numbers_font;
static PangoFontDescription *cell_numbers_font;

static void usage(const char *);

static struct fntsample_style *find_style(const char *name)
{
	struct fntsample_style *style = styles;

	for (; style->name; style++) {
		if (!strcmp(name, style->name))
			return style;
	}

	return NULL;
}

static int set_style(const char *name, const char *val)
{
	struct fntsample_style *style;
	char *new_val;

	style = find_style(name);

	if (!style)
		return -1;

	new_val = strdup(val);
	if (!new_val)
		return -1;

	if (style->val)
		free(style->val);
	style->val = new_val;
	
	return 0;
}

static const char *get_style(const char *name)
{
	struct fntsample_style *style = find_style(name);
	
	if (!style)
		return NULL;

	return style->val ? style->val : style->default_val;
}

static int parse_style_string(char *s)
{
	char *n;

	n = strchr(s, ':');
	if (!n)
		return -1;
	*n++ = '\0';
	return set_style(s, n);
}

static int add_range(char *range, bool include)
{
	struct range *r;
	uint32_t first = 0, last = 0xffffffff;
	char *minus;
	char *endptr;

	minus = strchr(range, '-');

	if (minus) {
		if (minus != range) {
			*minus = '\0';
			first = strtoul(range, &endptr, 0);
			if (*endptr)
				return -1;
		}

		if (*(minus + 1)) {
			last = strtoul(minus + 1, &endptr, 0);
			if (*endptr)
				return -1;
		}
		else if (minus == range)
			return -1;
	}
	else {
		first = strtoul(range, &endptr, 0);
		if (*endptr)
			return -1;
		last = first;
	}

	if (first > last)
		return -1;

	r = malloc(sizeof(*r));
	if (!r)
		return -1;

	r->first = first;
	r->last = last;
	r->include = include;
	r->next = NULL;

	if (ranges)
		last_range->next = r;
	else
		ranges = r;

	last_range = r;

	return 0;
}

static bool in_range(uint32_t c)
{
	bool in = ranges ? (!ranges->include) : 1;
	struct range *r;

	for (r = ranges; r; r = r->next) {
		if ((c >= r->first) && (c <= r->last))
			in = r->include;
	}
	return in;
}

static FT_ULong get_next_char(FT_Face face, FT_ULong charcode, FT_UInt *idx)
{
	FT_ULong rval = charcode;

	do {
		rval = FT_Get_Next_Char(face, rval, idx);
	} while (*idx && !in_range(rval));

	return rval;
}

static FT_ULong get_first_char(FT_Face face, FT_UInt *idx)
{
	FT_ULong rval;

	rval = FT_Get_First_Char(face, idx);

	if (*idx && !in_range(rval))
		rval = get_next_char(face, rval, idx);

	return rval;
}

static PangoLayout *draw_text(cairo_t *cr, PangoFontDescription *ftdesc, const char *text, PangoRectangle *r)
{
	PangoLayout *layout;

	layout = pango_cairo_create_layout (cr);
	pango_layout_set_font_description(layout, ftdesc);
	pango_layout_set_text(layout, text, -1);
	pango_layout_get_extents(layout, r, NULL);

	return layout;
}

static void parse_options(int argc, char * const argv[])
{
	for (;;) {
		int c;

		c = getopt_long(argc, argv, "f:o:hd:sli:x:t:", longopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'f':
			if (font_file_name) {
				fprintf(stderr, "Font file name should be given only once!\n");
				exit(1);
			}
			font_file_name = optarg;
			break;
		case 'o':
			if (output_file_name) {
				fprintf(stderr, "Output file name should be given only once!\n");
				exit(1);
			}
			output_file_name = optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
			break;
		case 'd':
			if (other_font_file_name) {
				fprintf(stderr, "Font file name should be given only once!\n");
				exit(1);
			}
			other_font_file_name = optarg;
			break;
		case 's':
			postscript_output = true;
			break;
		case 'l':
			print_outline = true;
			break;
		case 'i':
		case 'x':
			if (add_range(optarg, c == 'i')) {
				usage(argv[0]);
				exit(1);
			}
			break;
		case 't':
			if (parse_style_string(optarg) == -1) {
				usage(argv[0]);
				exit(1);
			}
			break;
		case '?':
		default:
			usage(argv[0]);
			exit(1);
			break;
		}
	}
	if (!font_file_name || !output_file_name) {
		usage(argv[0]);
		exit(1);
	}
}

static const struct unicode_block *get_unicode_block(unsigned long charcode)
{
	const struct unicode_block *block;

	for (block = unicode_blocks; block->name; block++) {
		if ((charcode >= block->start) && (charcode <= block->end))
			return block;
	}
	return NULL;
}

static int is_in_block(unsigned long charcode, const struct unicode_block *block)
{
	return ((charcode >= block->start) && (charcode <= block->end));
}

static void outline(int level, int page, const char *text)
{
	if (print_outline)
		printf("%d %d %s\n", level, page, text);
}

static void draw_header(cairo_t *cr, const char *face_name, const char *range_name)
{
	PangoLayout *layout;
	PangoRectangle r;

	layout = draw_text(cr, font_name_font, face_name, &r);
	cairo_move_to(cr, (A4_WIDTH - (double)r.width/PANGO_SCALE)/2.0, 30.0);
	pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
	g_object_unref(layout);

	layout = draw_text(cr, header_font, range_name, &r);
	cairo_move_to(cr, (A4_WIDTH - (double)r.width/PANGO_SCALE)/2.0, 50.0);
	pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
	g_object_unref(layout);
}

static void draw_cell(cairo_t *cr, double x, double y,
		unsigned long idx, bool highlight, cairo_glyph_t *glyph)
{
	cairo_text_extents_t extents;

	if (highlight) {
		cairo_set_source_rgb(cr, 1.0, 1.0, 0.6);
		cairo_rectangle(cr, x, y, cell_width, cell_height);
		cairo_fill(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	}

	*glyph = (cairo_glyph_t){idx, 0, 0};

	cairo_glyph_extents(cr, glyph, 1, &extents);

	glyph->x += x + (cell_width - extents.width)/2.0 - extents.x_bearing;
	glyph->y += y + cell_height / 2.0;
}

static void draw_grid(cairo_t *cr, unsigned int x_cells,
		unsigned long block_start)
{
	unsigned int i;
	double x_min = (A4_WIDTH - x_cells * cell_width) / 2;
	double x_max = (A4_WIDTH + x_cells * cell_width) / 2;
	char buf[9];
	PangoLayout *layout;
	PangoRectangle r;

#define TABLE_H (A4_HEIGHT - ymin_border * 2)
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, x_min, ymin_border, x_max - x_min, TABLE_H);
	cairo_move_to(cr, x_min, ymin_border);
	cairo_line_to(cr, x_min, ymin_border - 15.0);
	cairo_move_to(cr, x_max, ymin_border);
	cairo_line_to(cr, x_max, ymin_border - 15.0);
	cairo_stroke(cr);

	cairo_set_line_width(cr, 0.5);
	/* draw horizontal lines */
	for (i = 1; i < 16; i++) {
		cairo_move_to(cr, x_min, 72.0 + i * TABLE_H/16);
		cairo_line_to(cr, x_max, 72.0 + i * TABLE_H/16);
	}

	/* draw vertical lines */
	for (i = 1; i < x_cells; i++) {
		cairo_move_to(cr, x_min + i * cell_width, ymin_border);
		cairo_line_to(cr, x_min + i * cell_width, A4_HEIGHT - ymin_border);
	}
	cairo_stroke(cr);

	/* draw glyph numbers */
	buf[1] = '\0';
#define hexdigs	"0123456789ABCDEF"

	for (i = 0; i < 16; i++) {
		buf[0] = hexdigs[i];
		layout = draw_text(cr, table_numbers_font, buf, &r);
		cairo_move_to(cr, x_min - (double)PANGO_RBEARING(r)/PANGO_SCALE - 5.0,
				72.0 + (i+0.5) * TABLE_H/16 + (double)PANGO_DESCENT(r)/PANGO_SCALE/2);
		pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
		cairo_move_to(cr, x_min + x_cells * cell_width + 5.0,
				72.0 + (i+0.5) * TABLE_H/16 + (double)PANGO_DESCENT(r)/PANGO_SCALE/2);
		pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
		g_object_unref(layout);
	}

	for (i = 0; i < x_cells; i++) {
		snprintf(buf, sizeof(buf), "%03lX", block_start / 16 + i);
		layout = draw_text(cr, table_numbers_font, buf, &r);
		cairo_move_to(cr, x_min + i*cell_width + (cell_width - (double)r.width/PANGO_SCALE)/2,
				ymin_border - 5.0);
		pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
		g_object_unref(layout);
	}
}

static void draw_empty_cell(cairo_t *cr, double x, double y, unsigned long charcode)
{
	if (g_unichar_isdefined(charcode)) {
		if (g_unichar_iscntrl(charcode))
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.5);
		else
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	}
	cairo_rectangle(cr, x, y, cell_width, cell_height);
	cairo_fill(cr);
	if (g_unichar_isdefined(charcode))
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
}

static void draw_charcode(cairo_t *cr, double x, double y, FT_ULong charcode)
{
	char buf[9];
	PangoLayout *layout;
	PangoRectangle r;

	snprintf(buf, sizeof(buf), "%04lX", charcode);
	layout = draw_text(cr, cell_numbers_font, buf, &r);
	cairo_move_to(cr, x + (cell_width - (double)r.width/PANGO_SCALE)/2.0, y + cell_height - 4.0);
	pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
	g_object_unref(layout);
}

static int draw_unicode_block(cairo_t *cr, cairo_font_face_t *face,
		FT_Face ft_face, const char *fontname, unsigned long *charcode,
		const struct unicode_block *block, FT_Face ft_other_face)
{
	FT_UInt idx;
	unsigned long prev_charcode;
	unsigned long prev_cell;
	int npages = 0;

	idx = FT_Get_Char_Index(ft_face, *charcode);

	do {
		unsigned long offset = ((*charcode - block->start) / 0x100) * 0x100;
		unsigned long tbl_start = block->start + offset;
		unsigned long tbl_end = tbl_start + 0xFF > block->end ?
			block->end + 1 : tbl_start + 0x100;
		unsigned int rows = (tbl_end - tbl_start) / 16;
		double x_min = (A4_WIDTH - rows * cell_width) / 2;
		unsigned long i;
		bool filled_cells[256]; /* 16x16 glyphs max */
		bool highlight = false;

		/* XXX WARNING: not reentrant! */
		static cairo_glyph_t glyphs[256];
		unsigned int nglyphs = 0;

		cairo_save(cr);
		draw_header(cr, fontname, block->name);
		prev_cell = tbl_start - 1;

		memset(filled_cells, '\0', sizeof(filled_cells));

		cairo_set_font_face(cr, face);
		cairo_set_font_size(cr, 20.0);
		do {
			for (i = prev_cell + 1; i < *charcode; i++) {
				draw_empty_cell(cr, x_min + cell_width*((i - tbl_start) / 16),
							ymin_border + cell_height*((i - tbl_start) % 16), i);
			}

			if (ft_other_face)
				highlight = !FT_Get_Char_Index(ft_other_face, *charcode);

			draw_cell(cr, x_min + cell_width*((*charcode - tbl_start) / 16),
					ymin_border + cell_height*((*charcode - tbl_start) % 16),
					idx, highlight, &glyphs[nglyphs++]);

			filled_cells[*charcode - tbl_start] = true;

			prev_charcode = *charcode;
			prev_cell = *charcode;
			*charcode = get_next_char(ft_face, *charcode, &idx);
		} while (idx && (*charcode < tbl_end) && is_in_block(*charcode, block));

		cairo_show_glyphs(cr, glyphs, nglyphs);

		for (i = 0; i < tbl_end - tbl_start; i++)
			if (filled_cells[i])
				draw_charcode(cr, x_min + cell_width*(i / 16), ymin_border + cell_height*(i % 16),
						i + tbl_start);
		for (i = prev_cell + 1; i < tbl_end; i++) {
			draw_empty_cell(cr, x_min + cell_width*((i - tbl_start) / 16),
					ymin_border + cell_height*((i - tbl_start) % 16),
					i);
		}

		draw_grid(cr, rows, tbl_start);
		npages++;
		cairo_show_page(cr);
		cairo_restore(cr);
	} while (idx && is_in_block(*charcode, block));

	*charcode = prev_charcode;
	return npages;
}

static void draw_glyphs(cairo_t *cr, cairo_font_face_t *face, FT_Face ft_face,
		const char *fontname, FT_Face ft_other_face)
{
	FT_ULong charcode;
	FT_UInt idx;
	const struct unicode_block *block;
	int pageno = 1;

	outline(0, pageno, fontname);

	charcode = get_first_char(ft_face, &idx);

	while (idx) {
		block = get_unicode_block(charcode);
		if (block) {
			int npages;
			outline(1, pageno, block->name);
			npages = draw_unicode_block(cr, face, ft_face, fontname,
					&charcode, block, ft_other_face);
			pageno += npages;
		}
		charcode = get_next_char(ft_face, charcode, &idx);
	}
}

static void usage(const char *cmd)
{
	const struct fntsample_style *style;

	fprintf(stderr, "Usage: %s [ OPTIONS ] -f FONT-FILE -o OUTPUT-FILE\n"
			"       %s -h\n\n" , cmd, cmd);
	fprintf(stderr, "Options:\n"
			"  --font-file,         -f FONT-FILE    Create samples of FONT-FILE\n"
			"  --output-file,       -o OUTPUT-FILE  Save samples to OUTPUT-FILE\n"
			"  --help,              -h              Show this information message and exit\n"
			"  --other-font-file,   -d OTHER-FONT   Compare FONT-FILE with OTHER-FONT and highlight added glyphs\n"
			"  --postscript-output, -s              Use PostScript format for output instead of PDF\n"
			"  --print-outline,     -l              Print document outlines data to standard output\n"
			"  --include-range,     -i RANGE        Show characters in RANGE\n"
			"  --exclude-range,     -x RANGE        Do not show characters in RANGE\n"
			"  --style,             -t \"STYLE: VAL\" Set STYLE to value VAL\n");
	fprintf(stderr, "\nSupported styles (and default values):\n");
	for (style = styles; style->name; style++)
		fprintf(stderr, "\t%s (%s)\n", style->name, style->default_val);
}

static const char *get_font_name(FT_Face face)
{
	FT_Error error;
	FT_SfntName face_name;
	char *fontname;

	/* try SFNT format */
	error = FT_Get_Sfnt_Name(face, 4 /* full font name */, &face_name);
	if (!error) {
		fontname = malloc(face_name.string_len + 1);
		if (!fontname) {
			perror("malloc");
			exit(1);
		}
		memcpy(fontname, face_name.string, face_name.string_len);
		fontname[face_name.string_len] = '\0';
		return fontname;
	}

	/* try Type1 format */
	PS_FontInfoRec fontinfo;

	error = FT_Get_PS_Font_Info(face, &fontinfo);
	if (!error) {
		if (fontinfo.full_name) {
			fontname = strdup(fontinfo.full_name);
			if (!fontname) {
				perror("strdup");
				exit(1);
			}
			return fontname;
		}
	}

	/* fallback */
	const char *family_name = face->family_name ? face->family_name : "Unknown";
	const char *style_name = face->style_name ? face->style_name : "";
	size_t len = strlen(family_name) + strlen(style_name) + 1/* for space */;

	fontname = malloc(len + 1);
	if (!fontname) {
		perror("malloc");
		exit(1);
	}

	sprintf(fontname, "%s %s", family_name, style_name);
	return fontname;
}

static void init_pango_fonts(void)
{
	/* FIXME is this correct? */
	PangoCairoFontMap *map = (PangoCairoFontMap *) pango_cairo_font_map_get_default();

	pango_cairo_font_map_set_resolution(map, 72.0);

	header_font = pango_font_description_from_string(get_style("header-font"));
	font_name_font = pango_font_description_from_string(get_style("font-name-font"));
	table_numbers_font = pango_font_description_from_string(get_style("table-numbers-font"));
	cell_numbers_font = pango_font_description_from_string(get_style("cell-numbers-font"));
}

int main(int argc, char **argv)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	FT_Error error;
	FT_Library library;
	FT_Face face, other_face = NULL;
	const char *fontname; /* full name of the font */
	cairo_font_face_t *cr_face;
	cairo_status_t cr_status;

	parse_options(argc, argv);

	error = FT_Init_FreeType(&library);
	if (error) {
		fprintf(stderr, "%s: freetype error\n", argv[0]);
		exit(3);
	}

	error = FT_New_Face(library, font_file_name, 0, &face);
	if (error) {
		fprintf(stderr, "%s: failed to create new face\n", argv[0]);
		exit(4);
	}

	fontname = get_font_name(face);

	cr_face = cairo_ft_font_face_create_for_ft_face(face, 0);

	if (other_font_file_name) {
		error = FT_New_Face(library, other_font_file_name, 0, &other_face);
		if (error) {
			fprintf(stderr, "%s: failed to create new face\n", argv[0]);
			exit(4);
		}
	}

	if (postscript_output)
		surface = cairo_ps_surface_create(output_file_name, A4_WIDTH, A4_HEIGHT);
	else
		surface = cairo_pdf_surface_create(output_file_name, A4_WIDTH, A4_HEIGHT); /* A4 paper */

	cr_status = cairo_surface_status(surface);
	if (cr_status != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "%s: failed to create cairo surface: %s\n",
				argv[0], cairo_status_to_string(cr_status));
		exit(1);
	}

	cr = cairo_create(surface);
	cr_status = cairo_status(cr);
	if (cr_status != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "%s: cairo_create failed: %s\n",
				argv[0], cairo_status_to_string(cr_status));
		exit(1);
	}

	cairo_surface_destroy(surface);

	init_pango_fonts();

	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	draw_glyphs(cr, cr_face, face, fontname, other_face);
	cairo_destroy(cr);
	return 0;
}

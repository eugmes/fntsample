/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <assert.h>
// TODO: freetype 2.10.3, do not include ft2build.h anymore
#include <ft2build.h>
#include <freetype/freetype.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include <cairo-svg.h>
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
#include <pango/pangofc-fontmap.h>
#include <math.h>
#include <libintl.h>
#include <locale.h>

#include "unicode_blocks.h"
#include "static_unicode_blocks.h"
#include "config.h"

#define _(str) gettext(str)

#define POINTS_PER_INCH 72

#define A4_WIDTH (8.3 * POINTS_PER_INCH)
#define A4_HEIGHT (11.7 * POINTS_PER_INCH)

#define xmin_border (POINTS_PER_INCH / 1.5)
#define ymin_border POINTS_PER_INCH
#define cell_width ((A4_WIDTH - 2 * xmin_border) / 16)
#define cell_height ((A4_HEIGHT - 2 * ymin_border) / 16)

static double cell_x(double x_min, int pos) { return x_min + cell_width * (pos / 16); }

static double cell_y(int pos) { return ymin_border + cell_height * (pos % 16); }

static struct option longopts[] = {
    {"blocks-file", 1, 0, 'b'},
    {"font-file", 1, 0, 'f'},
    {"output-file", 1, 0, 'o'},
    {"help", 0, 0, 'h'},
    {"other-font-file", 1, 0, 'd'},
    {"postscript-output", 0, 0, 's'},
    {"svg", 0, 0, 'g'},
    {"print-outline", 0, 0, 'l'},
    {"write-outline", 0, 0, 'w'},
    {"include-range", 1, 0, 'i'},
    {"exclude-range", 1, 0, 'x'},
    {"style", 1, 0, 't'},
    {"font-index", 1, 0, 'n'},
    {"other-index", 1, 0, 'm'},
    {"no-embed", 0, 0, 'e'},
    {"use-pango", 0, 0, 'p'}, /* For compatibility with version <= 5.3 */
    {0, 0, 0, 0},
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
static bool svg_output;
static bool print_outline;
static bool write_outline;
static bool no_embed;
static struct range *ranges;
static struct range *last_range;
static int font_index;
static int other_index;

struct fntsample_style {
    const char *const name;
    const char *const default_val;
    char *val;
};

static struct fntsample_style styles[] = {
    {"header-font", "Sans Bold 12", NULL},
    {"font-name-font", "Serif Bold 12", NULL},
    {"table-numbers-font", "Sans 10", NULL},
    {"cell-numbers-font", "Mono 8", NULL},
    {NULL, NULL, NULL},
};

struct table_fonts {
    PangoFontDescription *header;
    PangoFontDescription *font_name;
    PangoFontDescription *table_numbers;
    PangoFontDescription *cell_numbers;
};

static struct table_fonts table_fonts;

static double cell_label_offset;
static double cell_glyph_bot_offset;
static double glyph_baseline_offset;
static double font_scale;

static const struct unicode_block *unicode_blocks;

static void usage(const char *);

static struct fntsample_style *find_style(const char *name)
{
    for (struct fntsample_style *style = styles; style->name; style++) {
        if (!strcmp(name, style->name)) {
            return style;
        }
    }

    return NULL;
}

static int set_style(const char *name, const char *val)
{
    struct fntsample_style *style = find_style(name);

    if (!style) {
        return -1;
    }

    char *new_val = strdup(val);
    if (!new_val) {
        return -1;
    }

    if (style->val) {
        free(style->val);
    }

    style->val = new_val;

    return 0;
}

static const char *get_style(const char *name)
{
    struct fntsample_style *style = find_style(name);

    if (!style) {
        return NULL;
    }

    return style->val ? style->val : style->default_val;
}

static int parse_style_string(char *s)
{
    char *n = strchr(s, ':');
    if (!n) {
        return -1;
    }

    *n++ = '\0';
    return set_style(s, n);
}

/*
 * Update output range.
 *
 * Returns -1 on error.
 */
static int add_range(char *range, bool include)
{
    uint32_t first = 0, last = 0xffffffff;
    char *endptr;

    char *minus = strchr(range, '-');

    if (minus) {
        if (minus != range) {
            *minus = '\0';
            first = strtoul(range, &endptr, 0);
            if (*endptr) {
                return -1;
            }
        }

        if (*(minus + 1)) {
            last = strtoul(minus + 1, &endptr, 0);
            if (*endptr) {
                return -1;
            }
        } else if (minus == range) {
            return -1;
        }
    } else {
        first = strtoul(range, &endptr, 0);
        if (*endptr)
            return -1;
        last = first;
    }

    if (first > last) {
        return -1;
    }

    struct range *r = malloc(sizeof(*r));
    if (!r) {
        return -1;
    }

    r->first = first;
    r->last = last;
    r->include = include;
    r->next = NULL;

    if (ranges) {
        last_range->next = r;
    } else {
        ranges = r;
    }

    last_range = r;

    return 0;
}

/*
 * Check if character with the given code belongs
 * to output range specified by the user.
 */
static bool in_range(uint32_t c)
{
    bool in = ranges ? (!ranges->include) : 1;

    for (struct range *r = ranges; r; r = r->next) {
        if ((c >= r->first) && (c <= r->last)) {
            in = r->include;
        }
    }
    return in;
}

/*
 * Get glyph index for the next glyph from the given font face, that
 * represents character from output range specified by the user.
 *
 * Returns character code, updates 'idx'.
 * 'idx' can became 0 if there are no more glyphs.
 */
static FT_ULong get_next_char(FT_Face face, FT_ULong charcode, FT_UInt *idx)
{
    FT_ULong rval = charcode;

    do {
        rval = FT_Get_Next_Char(face, rval, idx);
    } while (*idx && !in_range(rval));

    return rval;
}

/*
 * Locate first character from the given font face that belongs
 * to the user-specified output range.
 *
 * Returns character code, updates 'idx' with glyph index.
 * Glyph index can became 0 if there are no matching glyphs in the font.
 */
static FT_ULong get_first_char(FT_Face face, FT_UInt *idx)
{
    FT_ULong rval = FT_Get_First_Char(face, idx);

    if (*idx && !in_range(rval)) {
        rval = get_next_char(face, rval, idx);
    }

    return rval;
}

/*
 * Create Pango layout for the given text.
 * Updates 'r' with text extents.
 * Returned layout should be freed using g_object_unref().
 */
static PangoLayout *layout_text(cairo_t *cr, PangoFontDescription *ftdesc, const char *text,
                                PangoRectangle *r)
{
    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, ftdesc);
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_extents(layout, r, NULL);

    return layout;
}

static void parse_options(int argc, char *const argv[])
{
    for (;;) {
        int n;
        int c = getopt_long(argc, argv, "b:f:o:hd:sglwi:x:t:n:m:ep", longopts, NULL);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 'b':
            if (unicode_blocks) {
                fprintf(stderr, _("Unicode blocks file should be given at most once!\n"));
                exit(1);
            }

            unicode_blocks = read_blocks(optarg, &n);
            if (n == 0) {
                fprintf(stderr, _("Failed to load any blocks from the blocks file!\n"));
                exit(6);
            }
            break;
        case 'f':
            if (font_file_name) {
                fprintf(stderr, _("Font file name should be given only once!\n"));
                exit(1);
            }
            font_file_name = optarg;
            break;
        case 'o':
            if (output_file_name) {
                fprintf(stderr, _("Output file name should be given only once!\n"));
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
                fprintf(stderr, _("Font file name should be given only once!\n"));
                exit(1);
            }
            other_font_file_name = optarg;
            break;
        case 's':
            postscript_output = true;
            break;
        case 'g':
            svg_output = true;
            break;
        case 'l':
            print_outline = true;
            break;
        case 'w':
            write_outline = true;
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
        case 'n':
            font_index = atoi(optarg);
            break;
        case 'm':
            other_index = atoi(optarg);
            break;
        case 'e':
            no_embed = true;
            break;
        case 'p':
            /* Ignored for compatibility */
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

    if (font_index < 0 || other_index < 0) {
        fprintf(stderr, _("Font index should be non-negative!\n"));
        exit(1);
    }

    if (postscript_output && svg_output) {
        fprintf(stderr, _("-s and -g cannot be used together!\n"));
        exit(1);
    }

    if (!unicode_blocks) {
        unicode_blocks = static_unicode_blocks;
    }
}

/*
 * Locate unicode block that contains given character code.
 * Returns this block or NULL if not found.
 */
static const struct unicode_block *get_unicode_block(unsigned long charcode)
{
    for (const struct unicode_block *block = unicode_blocks; block->name; block++) {
        if ((charcode >= block->start) && (charcode <= block->end)) {
            return block;
        }
    }

    return NULL;
}

/*
 * Check if the given character code belongs to the given Unicode block.
 */
static bool is_in_block(unsigned long charcode, const struct unicode_block *block)
{
    return ((charcode >= block->start) && (charcode <= block->end));
}

/*
 * Format and print/write outline information, if requested by the user.
 */
static void outline(cairo_surface_t *surface, int level, int page, const char *text)
{
    if (print_outline) {
        printf("%d %d %s\n", level, page, text);
    }

    if (write_outline && cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_PDF) {
        int len = snprintf(0, 0, "page=%d", page);
        char *dest = malloc(len + 1);
        sprintf(dest, "page=%d", page);

        /* FIXME passing level here is not correct. */
        cairo_pdf_surface_add_outline(surface, level, text, dest, CAIRO_PDF_OUTLINE_FLAG_OPEN);
        free(dest);
    }
}

/*
 * Draw header of a page.
 * Header shows font name and current Unicode block.
 */
static void draw_header(cairo_t *cr, const char *face_name, const char *block_name)
{
    PangoRectangle r;

    PangoLayout *layout = layout_text(cr, table_fonts.font_name, face_name, &r);
    cairo_move_to(cr, (A4_WIDTH - pango_units_to_double(r.width)) / 2.0, 30.0);
    pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
    g_object_unref(layout);

    layout = layout_text(cr, table_fonts.header, block_name, &r);
    cairo_move_to(cr, (A4_WIDTH - pango_units_to_double(r.width)) / 2.0, 50.0);
    pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
    g_object_unref(layout);
}

/*
 * Highlight the cell with given coordinates.
 * Used to highlight new glyphs.
 */
static void highlight_cell(cairo_t *cr, double x, double y)
{
    cairo_save(cr);
    cairo_set_source_rgb(cr, 1.0, 1.0, 0.6);
    cairo_rectangle(cr, x, y, cell_width, cell_height);
    cairo_fill(cr);
    cairo_restore(cr);
}

/*
 * Draw table grid with row and column numbers.
 */
static void draw_grid(cairo_t *cr, unsigned int x_cells, unsigned long block_start)
{
    const double x_min = (A4_WIDTH - x_cells * cell_width) / 2;
    const double x_max = (A4_WIDTH + x_cells * cell_width) / 2;
    const double table_height = A4_HEIGHT - ymin_border * 2;

    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, x_min, ymin_border, x_max - x_min, table_height);
    cairo_move_to(cr, x_min, ymin_border);
    cairo_line_to(cr, x_min, ymin_border - 15.0);
    cairo_move_to(cr, x_max, ymin_border);
    cairo_line_to(cr, x_max, ymin_border - 15.0);
    cairo_stroke(cr);

    cairo_set_line_width(cr, 0.5);
    /* draw horizontal lines */
    for (int i = 1; i < 16; i++) {
        // TODO: use better name instead of just POINTS_PER_INCH
        cairo_move_to(cr, x_min, POINTS_PER_INCH + i * table_height / 16);
        cairo_line_to(cr, x_max, POINTS_PER_INCH + i * table_height / 16);
    }

    /* draw vertical lines */
    for (unsigned int i = 1; i < x_cells; i++) {
        cairo_move_to(cr, x_min + i * cell_width, ymin_border);
        cairo_line_to(cr, x_min + i * cell_width, A4_HEIGHT - ymin_border);
    }
    cairo_stroke(cr);

    /* draw glyph numbers */
    char buf[17];
    buf[1] = '\0';
#define hexdigs "0123456789ABCDEF"

    for (int i = 0; i < 16; i++) {
        buf[0] = hexdigs[i];

        PangoRectangle r;
        PangoLayout *layout = layout_text(cr, table_fonts.table_numbers, buf, &r);
        cairo_move_to(cr, x_min - pango_units_to_double(PANGO_RBEARING(r)) - 5.0,
                      POINTS_PER_INCH + (i + 0.5) * table_height / 16
                          + pango_units_to_double(PANGO_DESCENT(r)) / 2);
        pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
        cairo_move_to(cr, x_min + x_cells * cell_width + 5.0,
                      POINTS_PER_INCH + (i + 0.5) * table_height / 16
                          + pango_units_to_double(PANGO_DESCENT(r)) / 2);
        pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
        g_object_unref(layout);
    }

    for (unsigned int i = 0; i < x_cells; i++) {
        snprintf(buf, sizeof(buf), "%03lX", block_start / 16 + i);

        PangoRectangle r;
        PangoLayout *layout = layout_text(cr, table_fonts.table_numbers, buf, &r);
        cairo_move_to(cr,
                      x_min + i * cell_width + (cell_width - pango_units_to_double(r.width)) / 2,
                      ymin_border - 5.0);
        pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
        g_object_unref(layout);
    }
}

/*
 * Fill empty cell. Color of the fill depends on the character properties.
 */
static void fill_empty_cell(cairo_t *cr, double x, double y, unsigned long charcode)
{
    cairo_save(cr);
    if (g_unichar_isdefined(charcode)) {
        if (g_unichar_iscntrl(charcode))
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.5);
        else
            cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_rectangle(cr, x, y, cell_width, cell_height);
    cairo_fill(cr);
    cairo_restore(cr);
}

/*
 * Draw label with character code.
 */
static void draw_charcode(cairo_t *cr, double x, double y, FT_ULong charcode)
{
    char buf[9];
    snprintf(buf, sizeof(buf), "%04lX", charcode);

    PangoRectangle r;
    PangoLayout *layout = layout_text(cr, table_fonts.cell_numbers, buf, &r);
    cairo_move_to(cr, x + (cell_width - pango_units_to_double(r.width)) / 2.0,
                  y + cell_height - cell_label_offset);
    pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
    g_object_unref(layout);
}

/*
 * Draws tables for all characters in the given Unicode block.
 * Use font described by face and ft_face. Start from character
 * with given charcode (it should belong to the given Unicode
 * block). After return 'charcode' equals the last character code
 * of the block.
 *
 * Returns number of pages drawn.
 */
static int draw_unicode_block(cairo_t *cr, PangoLayout *layout, FT_Face ft_face,
                              const char *font_name, unsigned long charcode,
                              const struct unicode_block *block, FT_Face ft_other_face)
{
    int npages = 0;
    FT_UInt idx = FT_Get_Char_Index(ft_face, charcode);

    do {
        unsigned long offset = ((charcode - block->start) / 0x100) * 0x100;
        unsigned long tbl_start = block->start + offset;
        unsigned long tbl_end = tbl_start + 0xFF > block->end ? block->end + 1 : tbl_start + 0x100;
        unsigned int rows = (tbl_end - tbl_start) / 16;
        double x_min = (A4_WIDTH - rows * cell_width) / 2;

        bool filled_cells[256]; /* 16x16 glyphs max */
        unsigned long curr_charcode = tbl_start;
        int pos = 0;

        cairo_save(cr);
        draw_header(cr, font_name, block->name);

        memset(filled_cells, '\0', sizeof(filled_cells));

        /*
         * Fill empty cells and calculate coordinates of the glyphs.
         * Also highlight cells if needed.
         */
        do {
            /* fill empty cells before the current glyph */
            for (; curr_charcode < charcode; curr_charcode++, pos++) {
                fill_empty_cell(cr, cell_x(x_min, pos), cell_y(pos), curr_charcode);
            }

            /* if it is new glyph - highlight the cell */
            if (ft_other_face && !FT_Get_Char_Index(ft_other_face, charcode)) {
                highlight_cell(cr, cell_x(x_min, pos), cell_y(pos));
            }

            /* draw the character */
            char buf[9];
            gint len = g_unichar_to_utf8((gunichar)charcode, buf);
            pango_layout_set_text(layout, buf, len);

            double baseline = pango_units_to_double(pango_layout_get_baseline(layout));
            cairo_move_to(cr, cell_x(x_min, pos), cell_y(pos) + glyph_baseline_offset - baseline);

            if (no_embed) {
                pango_cairo_layout_path(cr, layout);
            } else {
                pango_cairo_show_layout(cr, layout);
            }

            filled_cells[pos] = true;
            curr_charcode++;
            pos++;

            charcode = get_next_char(ft_face, charcode, &idx);
        } while (idx && (charcode < tbl_end) && is_in_block(charcode, block));

        /* Fill remaining empty cells */
        for (; curr_charcode < tbl_end; curr_charcode++, pos++) {
            fill_empty_cell(cr, cell_x(x_min, pos), cell_y(pos), curr_charcode);
        }

        /*
         * Charcodes are drawn here to avoid switching between the charcode
         * font and the cell font for each filled cell.
         */
        for (unsigned long i = 0; i < tbl_end - tbl_start; i++) {
            if (filled_cells[i]) {
                draw_charcode(cr, cell_x(x_min, i), cell_y(i), i + tbl_start);
            }
        }

        draw_grid(cr, rows, tbl_start);
        npages++;
        cairo_show_page(cr);
        cairo_restore(cr);
    } while (idx && is_in_block(charcode, block));

    return npages;
}

static PangoLayout *create_glyph_layout(cairo_t *cr, FcConfig *fc_config, FcPattern *fc_font)
{
    PangoFontMap *fontmap = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    pango_fc_font_map_set_config(PANGO_FC_FONT_MAP(fontmap), fc_config);
    PangoContext *context = pango_font_map_create_context(fontmap);
    pango_cairo_update_context(cr, context);

    PangoFontDescription *font_desc = pango_fc_font_description_from_pattern(fc_font, FALSE);
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_width(layout, pango_units_from_double(cell_width));
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    g_object_unref(context);
    g_object_unref(fontmap);
    pango_font_description_free(font_desc);

    return layout;
}

/*
 * The main drawing function.
 */
static void draw_glyphs(cairo_t *cr, FT_Face ft_face, FT_Face ft_other_face)
{
    FcConfig *fc_config = FcConfigCreate();
    FcConfigAppFontAddFile(fc_config, (const FcChar8 *)font_file_name);

    FcPattern *fc_pat = FcPatternCreate();
    FcPatternAddInteger(fc_pat, FC_INDEX, font_index);

    FcFontSet *fc_fontset = FcFontList(fc_config, fc_pat, NULL);
    assert(fc_fontset->nfont > 0);
    FcPattern *fc_font = fc_fontset->fonts[0];

    const char *font_name;
    if (FcPatternGetString(fc_font, FC_FULLNAME, 0, (FcChar8 **)&font_name) != FcResultMatch) {
        font_name = "Unknown";
    }

    cairo_surface_t *surface = cairo_get_target(cr);

    int pageno = 1;
    outline(surface, 0, pageno, font_name);

    PangoLayout *layout = create_glyph_layout(cr, fc_config, fc_font);

    FT_UInt idx;
    FT_ULong charcode = get_first_char(ft_face, &idx);

    while (idx) {
        const struct unicode_block *block = get_unicode_block(charcode);
        if (block) {
            outline(surface, 1, pageno, block->name);
            int npages = draw_unicode_block(cr, layout, ft_face, font_name, charcode, block,
                                            ft_other_face);
            pageno += npages;
            charcode = block->end;
        }

        charcode = get_next_char(ft_face, charcode, &idx);
    }

    g_object_unref(layout);

    FcPatternDestroy(fc_pat);
    FcFontSetDestroy(fc_fontset);
    FcConfigDestroy(fc_config);
}

/*
 * Print usage instructions and default values for styles
 */
static void usage(const char *cmd)
{
    fprintf(stderr,
            _("Usage: %s [ OPTIONS ] -f FONT-FILE -o OUTPUT-FILE\n"
              "       %s -h\n\n"),
            cmd, cmd);
    fprintf(
        stderr,
        _("Options:\n"
          "  --blocks-file,       -b BLOCKS-FILE  Read Unicode blocks information from "
          "BLOCKS-FILE\n"
          "  --font-file,         -f FONT-FILE    Create samples of FONT-FILE\n"
          "  --font-index,        -n IDX          Font index in FONT-FILE\n"
          "  --output-file,       -o OUTPUT-FILE  Save samples to OUTPUT-FILE\n"
          "  --help,              -h              Show this information message and exit\n"
          "  --other-font-file,   -d OTHER-FONT   Compare FONT-FILE with OTHER-FONT and highlight "
          "added glyphs\n"
          "  --other-index,       -m IDX          Font index in OTHER-FONT\n"
          "  --postscript-output, -s              Use PostScript format for output instead of PDF\n"
          "  --svg,               -g              Use SVG format for output\n"
          "  --print-outline,     -l              Print document outlines data to standard output\n"
          "  --write-outline,     -w              Write document outlines (only in PDF output)\n"
          "  --no-embed,          -e              Don't embed the font in the output file, draw "
          "the glyphs instead\n"
          "  --include-range,     -i RANGE        Show characters in RANGE\n"
          "  --exclude-range,     -x RANGE        Do not show characters in RANGE\n"
          "  --style,             -t \"STYLE: VAL\" Set STYLE to value VAL\n"));

    fprintf(stderr, _("\nSupported styles (and default values):\n"));

    for (const struct fntsample_style *style = styles; style->name; style++) {
        fprintf(stderr, "\t%s (%s)\n", style->name, style->default_val);
    }
}

/*
 * Initialize fonts used to print table heders and character codes.
 */
static void init_table_fonts(void)
{
    /* FIXME is this correct? */
    PangoCairoFontMap *map = (PangoCairoFontMap *)pango_cairo_font_map_get_default();

    pango_cairo_font_map_set_resolution(map, POINTS_PER_INCH);

    table_fonts.header = pango_font_description_from_string(get_style("header-font"));
    table_fonts.font_name = pango_font_description_from_string(get_style("font-name-font"));
    table_fonts.table_numbers = pango_font_description_from_string(get_style("table-numbers-font"));
    table_fonts.cell_numbers = pango_font_description_from_string(get_style("cell-numbers-font"));
}

/*
 * Calculate various offsets.
 */
static void calculate_offsets(cairo_t *cr)
{
    PangoRectangle extents;
    /* Assume that vertical extents does not depend on actual text */
    PangoLayout *l = layout_text(cr, table_fonts.cell_numbers, "0123456789ABCDEF", &extents);
    g_object_unref(l);
    /* Unsolved mistery of pango's font metrics.... */
    double digits_ascent = pango_units_to_double(PANGO_DESCENT(extents));
    double digits_descent = -pango_units_to_double(PANGO_ASCENT(extents));

    cell_label_offset = digits_descent + 2;
    cell_glyph_bot_offset = cell_label_offset + digits_ascent + 2;
}

/*
 * Calculate font scaling
 */
void calc_font_scaling(FT_Face ft_face)
{
    cairo_font_face_t *cr_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_font_options_t *options = cairo_font_options_create();

    /* First create font with size 1 and measure it */
    cairo_matrix_t font_matrix;
    cairo_matrix_init_identity(&font_matrix);
    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    /* Turn off rounding, so we can get real metrics */
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
    cairo_scaled_font_t *cr_font = cairo_scaled_font_create(cr_face, &font_matrix, &ctm, options);
    cairo_font_extents_t extents;
    cairo_scaled_font_extents(cr_font, &extents);

    /* Use some magic to find the best font size... */
    double tgt_size = cell_height - cell_glyph_bot_offset - 2;
    if (tgt_size <= 0) {
        fprintf(stderr, _("Not enough space for rendering glyphs. Make cell font smaller.\n"));
        exit(5);
    }

    double act_size = extents.ascent + extents.descent;
    if (act_size <= 0) {
        fprintf(stderr, _("The font has strange metrics: ascent + descent = %g\n"), act_size);
        exit(5);
    }

    font_scale = tgt_size / act_size;
    if (font_scale > 1)
        font_scale = trunc(font_scale); // just to make numbers nicer
    if (font_scale > 20)
        font_scale = 20; // Do not make font larger than in previous versions

    cairo_scaled_font_destroy(cr_font);

    /* Create the font once again, but this time scaled */
    cairo_matrix_init_scale(&font_matrix, font_scale, font_scale);
    cr_font = cairo_scaled_font_create(cr_face, &font_matrix, &ctm, options);
    cairo_scaled_font_extents(cr_font, &extents);
    glyph_baseline_offset
        = (tgt_size - (extents.ascent + extents.descent)) / 2 + 2 + extents.ascent;
    cairo_scaled_font_destroy(cr_font);
}

/*
 * Configure DPF surface metadata so fntsample can be used with
 * repeatable builds.
 */
static void set_repeatable_pdf_metadata(cairo_surface_t *surface)
{
    char *source_date_epoch = getenv("SOURCE_DATE_EPOCH");

    if (source_date_epoch) {
        char *endptr;
        time_t now = strtoul(source_date_epoch, &endptr, 10);

        if (*endptr != 0) {
            fprintf(stderr, _("Failed to parse environment variable SOURCE_DATE_EPOCH.\n"));
            exit(1);
        }
        struct tm *build_time = gmtime(&now);
        char buffer[25];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", build_time);

        cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATE_DATE, buffer);
    }
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    bindtextdomain(CMAKE_PROJECT_NAME, CMAKE_INSTALL_FULL_LOCALEDIR);
    textdomain(CMAKE_PROJECT_NAME);

    parse_options(argc, argv);

    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);

    if (error) {
        /* TRANSLATORS: 'freetype' is a name of a library, and should be left untranslated */
        fprintf(stderr, _("%s: freetype error\n"), argv[0]);
        exit(3);
    }

    FT_Face face;
    error = FT_New_Face(library, font_file_name, font_index, &face);

    if (error) {
        fprintf(stderr, _("%s: failed to open font file %s\n"), argv[0], font_file_name);
        exit(4);
    }

    FT_Face other_face = NULL;

    if (other_font_file_name) {
        error = FT_New_Face(library, other_font_file_name, other_index, &other_face);

        if (error) {
            fprintf(stderr, _("%s: failed to create new font face\n"), argv[0]);
            exit(4);
        }
    }

    cairo_surface_t *surface;

    if (postscript_output) {
        surface = cairo_ps_surface_create(output_file_name, A4_WIDTH, A4_HEIGHT);
    } else if (svg_output) {
        surface = cairo_svg_surface_create(output_file_name, A4_WIDTH, A4_HEIGHT);
    } else {
        surface = cairo_pdf_surface_create(output_file_name, A4_WIDTH, A4_HEIGHT); /* A4 paper */
        set_repeatable_pdf_metadata(surface);
    }

    cairo_status_t cr_status = cairo_surface_status(surface);
    if (cr_status != CAIRO_STATUS_SUCCESS) {
        /* TRANSLATORS: 'cairo' is a name of a library, and should be left untranslated */
        fprintf(stderr, _("%s: failed to create cairo surface: %s\n"), argv[0],
                cairo_status_to_string(cr_status));
        exit(1);
    }

    cairo_t *cr = cairo_create(surface);
    cr_status = cairo_status(cr);
    if (cr_status != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, _("%s: cairo_create failed: %s\n"), argv[0],
                cairo_status_to_string(cr_status));
        exit(1);
    }

    cairo_surface_destroy(surface);

    init_table_fonts();
    calculate_offsets(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    calc_font_scaling(face);
    draw_glyphs(cr, face, other_face);
    cairo_destroy(cr);

    return 0;
}

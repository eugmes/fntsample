/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <cassert>
// TODO: freetype 2.10.3, do not include ft2build.h anymore
#include <ft2build.h>
#include <freetype/freetype.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include <cairo-svg.h>
#include <cairo-ft.h>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <unistd.h>
#include <getopt.h>
#include <cstdint>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <cmath>
#include <libintl.h>
#include <clocale>
#include <iostream>
#include <fmt/ostream.h>
#include <memory>
#include <fstream>
#include <string_view>
#include <charconv>

#include "loadable_unicode_blocks.h"
#include "static_unicode_blocks.h"
#include "unicode_range_set.h"
#include "unicode_utils.h"

#include "config.h"

using namespace std;

#define _(str) gettext(str)

constexpr double POINTS_PER_INCH = 72;

class page_metrics {
public:
    explicit page_metrics(unsigned num_columns) noexcept
        : num_columns(num_columns)
        , table_width(num_columns * cell_width)
        , x_min((page_width - table_width) / 2)
        , x_max(page_width - x_min)
    {
    }

    static constexpr unsigned num_rows = 16;
    static constexpr unsigned max_num_columns = 16;
    const unsigned num_columns;

    // NOTE: A4 paper size
    static constexpr double page_width = 8.3 * POINTS_PER_INCH;
    static constexpr double page_height = 11.7 * POINTS_PER_INCH;

    static constexpr double min_horiz_border = POINTS_PER_INCH / 1.5;
    static constexpr double vert_border = POINTS_PER_INCH;
    static constexpr double cell_width = (page_width - 2 * min_horiz_border) / max_num_columns;
    static constexpr double cell_height = (page_height - 2 * vert_border) / num_rows;

    static constexpr double table_height = page_height - vert_border * 2;
    const double table_width;
    const double x_min;
    const double x_max;

    double cell_x(unsigned pos) const noexcept { return x_min + cell_width * (pos / num_rows); }

    double cell_y(unsigned pos) const noexcept
    {
        return vert_border + cell_height * (pos % num_rows);
    }
};

static option longopts[] = {
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

static const char *blocks_file;
static const char *font_file_name;
static const char *other_font_file_name;
static const char *output_file_name;
static bool postscript_output;
static bool svg_output;
static bool print_outline;
static bool write_outline;
static bool no_embed;
static unicode_range_set ranges;
static int font_index;
static int other_index;

struct fntsample_style {
    const char *const name;
    const char *const default_val;
    char *val;
};

static fntsample_style styles[] = {
    {"header-font", "Sans Bold 12", nullptr},
    {"font-name-font", "Serif Bold 12", nullptr},
    {"table-numbers-font", "Sans 10", nullptr},
    {"cell-numbers-font", "Mono 8", nullptr},
    {nullptr, nullptr, nullptr},
};

struct table_fonts {
    PangoFontDescription *header;
    PangoFontDescription *font_name;
    PangoFontDescription *table_numbers;
    PangoFontDescription *cell_numbers;
};

static table_fonts table_fonts;

static double cell_label_offset;
static double cell_glyph_bot_offset;
static double glyph_baseline_offset;
static double font_scale;

static void usage(const char *);

static fntsample_style *find_style(const char *name)
{
    for (fntsample_style *style = styles; style->name; style++) {
        if (!strcmp(name, style->name)) {
            return style;
        }
    }

    return nullptr;
}

static int set_style(const char *name, const char *val)
{
    fntsample_style *style = find_style(name);

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
    fntsample_style *style = find_style(name);

    if (!style) {
        return nullptr;
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
 * Parse output range.
 */
static optional<unicode_range> parse_range(string_view s)
{
    auto parse_code = [](const char *first, const char *last) -> optional<char32_t> {
        int base = 10;
        assert(first < last);

        if (*first == '0') {
            base = 8;
            first++;
            if (first < last && (*first == 'x' || *first == 'X')) {
                base = 16;
                first++;
            }
        }

        unsigned long n;
        auto [ptr, ec] = from_chars(first, last, n, base);
        if (ec == errc()) {
            return unicode_utils::to_char32_t(n);
        }
        return {};
    };

    unicode_range r {0, unicode_utils::last_codepoint};

    auto minus = s.find('-');

    if (minus >= 0) {
        // minus found
        if (s.size() == 1) {
            return {};
        }

        if (minus > 0) {
            // not at the beginning
            auto res = parse_code(s.data(), s.data() + minus);
            if (!res) {
                return {};
            }
            r.start = *res;
        }

        if (minus < s.size() - 1) {
            // not at the end
            auto res = parse_code(s.data() + minus + 1, s.data() + s.size());
            if (!res) {
                return {};
            }
            r.end = *res;
        }
    } else {
        // no minus found - single codepoint
        auto res = parse_code(s.data(), s.data() + s.size());
        if (!res) {
            return {};
        }
        r.start = r.end = *res;
    }

    if (!r.is_valid()) {
        return {};
    }

    return r;
}

bool add_range(string_view s, bool include)
{
    auto res = parse_range(s);
    if (!res) {
        return false;
    }
    ranges.add(*res, include);
    return true;
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
    } while (*idx && !ranges.contains(rval));

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

    if (*idx && !ranges.contains(rval)) {
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
    pango_layout_get_extents(layout, r, nullptr);

    return layout;
}

static void parse_options(int argc, char *const argv[])
{
    for (;;) {
        int c = getopt_long(argc, argv, "b:f:o:hd:sglwi:x:t:n:m:ep", longopts, nullptr);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 'b':
            if (blocks_file) {
                cerr << _("Unicode blocks file should be given at most once!\n");
                exit(1);
            }
            blocks_file = optarg;
            break;
        case 'f':
            if (font_file_name) {
                cerr << _("Font file name should be given only once!\n");
                exit(1);
            }
            font_file_name = optarg;
            break;
        case 'o':
            if (output_file_name) {
                cerr << _("Output file name should be given only once!\n");
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
                cerr << _("Font file name should be given only once!\n");
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
            if (!add_range(optarg, c == 'i')) {
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
        cerr << _("Font index should be non-negative!\n");
        exit(1);
    }

    if (postscript_output && svg_output) {
        cerr << _("-s and -g cannot be used together!\n");
        exit(1);
    }
}

/*
 * Format and print/write outline information, if requested by the user.
 */
static void outline(cairo_surface_t *surface, int level, int page, const char *text)
{
    if (print_outline) {
        fmt::print("{} {} {}\n", level, page, text);
    }

    if (write_outline && cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_PDF) {
        auto s = fmt::format("page={}", page);
        /* FIXME passing level here is not correct. */
        cairo_pdf_surface_add_outline(surface, level, text, s.c_str(), CAIRO_PDF_OUTLINE_FLAG_OPEN);
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
    cairo_move_to(cr, (page_metrics::page_width - pango_units_to_double(r.width)) / 2.0, 30.0);
    pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
    g_object_unref(layout);

    layout = layout_text(cr, table_fonts.header, block_name, &r);
    cairo_move_to(cr, (page_metrics::page_width - pango_units_to_double(r.width)) / 2.0, 50.0);
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
    cairo_rectangle(cr, x, y, page_metrics::cell_width, page_metrics::cell_height);
    cairo_fill(cr);
    cairo_restore(cr);
}

/*
 * Draw table grid with row and column numbers.
 */
static void draw_grid(cairo_t *cr, const page_metrics &page, unsigned long block_start)
{
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, page.x_min, page.vert_border, page.table_width, page.table_height);
    cairo_move_to(cr, page.x_min, page.vert_border);
    cairo_line_to(cr, page.x_min, page.vert_border - 15.0);
    cairo_move_to(cr, page.x_max, page.vert_border);
    cairo_line_to(cr, page.x_max, page.vert_border - 15.0);
    cairo_stroke(cr);

    cairo_set_line_width(cr, 0.5);
    /* draw horizontal lines */
    for (unsigned row = 1; row < page.num_rows; row++) {
        const auto y = page.vert_border + row * page.cell_height;
        cairo_move_to(cr, page.x_min, y);
        cairo_line_to(cr, page.x_max, y);
    }

    /* draw vertical lines */
    for (unsigned col = 1; col < page.num_columns; col++) {
        const auto x = page.x_min + col * page.cell_width;
        cairo_move_to(cr, x, page.vert_border);
        cairo_line_to(cr, x, page.page_height - page.vert_border);
    }
    cairo_stroke(cr);

    string buf;

    /* draw glyph numbers */
    for (unsigned row = 0; row < page.num_rows; row++) {
        buf = fmt::format("{:X}", row);

        PangoRectangle r;
        PangoLayout *layout = layout_text(cr, table_fonts.table_numbers, buf.c_str(), &r);
        const auto y = page.vert_border + (row + 0.5) * page.cell_height
            + pango_units_to_double(PANGO_DESCENT(r)) / 2;

        cairo_move_to(cr, page.x_min - pango_units_to_double(PANGO_RBEARING(r)) - 5.0, y);
        pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
        cairo_move_to(cr, page.x_max + 5.0, y);
        pango_cairo_show_layout_line(cr, pango_layout_get_line_readonly(layout, 0));
        g_object_unref(layout);
    }

    for (unsigned col = 0; col < page.num_columns; col++) {
        buf = fmt::format("{:03X}", block_start / page.num_rows + col);

        PangoRectangle r;
        PangoLayout *layout = layout_text(cr, table_fonts.table_numbers, buf.c_str(), &r);
        cairo_move_to(cr,
                      page.x_min + col * page.cell_width
                          + (page.cell_width - pango_units_to_double(r.width)) / 2,
                      page.vert_border - 5.0);
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
    cairo_rectangle(cr, x, y, page_metrics::cell_width, page_metrics::cell_height);
    cairo_fill(cr);
    cairo_restore(cr);
}

/*
 * Draw label with character code.
 */
static void draw_charcode(cairo_t *cr, double x, double y, FT_ULong charcode)
{
    auto s = fmt::format("{:04X}", charcode);

    PangoRectangle r;
    PangoLayout *layout = layout_text(cr, table_fonts.cell_numbers, s.c_str(), &r);
    cairo_move_to(cr, x + (page_metrics::cell_width - pango_units_to_double(r.width)) / 2.0,
                  y + page_metrics::cell_height - cell_label_offset);
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
                              const unicode_blocks::block &block, FT_Face ft_other_face)
{
    int npages = 0;
    FT_UInt idx = FT_Get_Char_Index(ft_face, charcode);

    do {
        unsigned long offset = ((charcode - block.r.start) / 0x100) * 0x100;
        unsigned long tbl_start = block.r.start + offset;
        unsigned long tbl_end
            = tbl_start + 0xFF > block.r.end ? block.r.end + 1 : tbl_start + 0x100;
        const page_metrics page((tbl_end - tbl_start) / page.num_rows);

        bool filled_cells[256]; /* 16x16 glyphs max */
        unsigned long curr_charcode = tbl_start;
        int pos = 0;

        cairo_save(cr);
        draw_header(cr, font_name, block.name);

        memset(filled_cells, '\0', sizeof(filled_cells));

        /*
         * Fill empty cells and calculate coordinates of the glyphs.
         * Also highlight cells if needed.
         */
        do {
            /* fill empty cells before the current glyph */
            for (; curr_charcode < charcode; curr_charcode++, pos++) {
                fill_empty_cell(cr, page.cell_x(pos), page.cell_y(pos), curr_charcode);
            }

            /* if it is new glyph - highlight the cell */
            if (ft_other_face && !FT_Get_Char_Index(ft_other_face, charcode)) {
                highlight_cell(cr, page.cell_x(pos), page.cell_y(pos));
            }

            /* draw the character */
            char buf[9];
            gint len = g_unichar_to_utf8((gunichar)charcode, buf);
            pango_layout_set_text(layout, buf, len);

            double baseline = pango_units_to_double(pango_layout_get_baseline(layout));
            cairo_move_to(cr, page.cell_x(pos),
                          page.cell_y(pos) + glyph_baseline_offset - baseline);

            if (no_embed) {
                pango_cairo_layout_path(cr, layout);
            } else {
                pango_cairo_show_layout(cr, layout);
            }

            filled_cells[pos] = true;
            curr_charcode++;
            pos++;

            charcode = get_next_char(ft_face, charcode, &idx);
        } while (idx && (charcode < tbl_end) && block.contains(charcode));

        /* Fill remaining empty cells */
        for (; curr_charcode < tbl_end; curr_charcode++, pos++) {
            fill_empty_cell(cr, page.cell_x(pos), page.cell_y(pos), curr_charcode);
        }

        /*
         * Charcodes are drawn here to avoid switching between the charcode
         * font and the cell font for each filled cell.
         */
        for (unsigned long i = 0; i < tbl_end - tbl_start; i++) {
            if (filled_cells[i]) {
                draw_charcode(cr, page.cell_x(i), page.cell_y(i), i + tbl_start);
            }
        }

        draw_grid(cr, page, tbl_start);
        npages++;
        cairo_show_page(cr);
        cairo_restore(cr);
    } while (idx && block.contains(charcode));

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
    pango_layout_set_width(layout, pango_units_from_double(page_metrics::cell_width));
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    g_object_unref(context);
    g_object_unref(fontmap);
    pango_font_description_free(font_desc);

    return layout;
}

/*
 * The main drawing function.
 */
static void draw_glyphs(const unicode_blocks &blocks, cairo_t *cr, FT_Face ft_face,
                        FT_Face ft_other_face)
{
    FcConfig *fc_config = FcConfigCreate();
    FcConfigAppFontAddFile(fc_config, (const FcChar8 *)font_file_name);

    FcPattern *fc_pat = FcPatternCreate();
    FcPatternAddInteger(fc_pat, FC_INDEX, font_index);

    FcFontSet *fc_fontset = FcFontList(fc_config, fc_pat, nullptr);
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
        if (auto b = blocks.find_block(charcode); b.has_value()) {
            const auto block = *b;

            outline(surface, 1, pageno, block.name);
            int npages = draw_unicode_block(cr, layout, ft_face, font_name, charcode, block,
                                            ft_other_face);
            pageno += npages;
            charcode = block.r.end;
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
    fmt::print(cerr,
               _("Usage: {0} [ OPTIONS ] -f FONT-FILE -o OUTPUT-FILE\n"
                 "       {0} -h\n\n"),
               cmd);
    cerr << _(
        "Options:\n"
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
        "  --style,             -t \"STYLE: VAL\" Set STYLE to value VAL\n");

    cerr << _("\nSupported styles (and default values):\n");

    for (const fntsample_style *style = styles; style->name; style++) {
        fmt::print(cerr, "\t{} ({})\n", style->name, style->default_val);
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
    double tgt_size = page_metrics::cell_height - cell_glyph_bot_offset - 2;
    if (tgt_size <= 0) {
        cerr << _("Not enough space for rendering glyphs. Make cell font smaller.\n");
        exit(5);
    }

    double act_size = extents.ascent + extents.descent;
    if (act_size <= 0) {
        fmt::print(cerr, _("The font has strange metrics: ascent + descent = {}\n"), act_size);
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
            cerr << _("Failed to parse environment variable SOURCE_DATE_EPOCH.\n");
            exit(1);
        }
        tm *build_time = gmtime(&now);
        char buffer[25];
        // TODO
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

    unique_ptr<loadable_unicode_blocks> loadable_blocks;

    if (blocks_file) {
        ifstream f(blocks_file, ios::binary);
        if (!f) {
            fmt::print(cerr, _("{}: cannot open Unicode blocks file.\n"), argv[0]);
            exit(6);
        }

        // FIXME
        loadable_blocks = make_unique<loadable_unicode_blocks>(f);
        if (loadable_blocks->error_line()) {
            fmt::print(cerr, _("{}: parse error in Unicode blocks file at line {}\n"), argv[0],
                       loadable_blocks->error_line());
            exit(6);
        }
    }

    const unicode_blocks &blocks = loadable_blocks ? *loadable_blocks : get_static_blocks();

    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);

    if (error) {
        /* TRANSLATORS: 'freetype' is a name of a library, and should be left untranslated */
        fmt::print(cerr, _("{}: freetype error\n"), argv[0]);
        exit(3);
    }

    FT_Face face;
    error = FT_New_Face(library, font_file_name, font_index, &face);

    if (error) {
        fmt::print(cerr, _("%{}: failed to open font file {}\n"), argv[0], font_file_name);
        exit(4);
    }

    FT_Face other_face = nullptr;

    if (other_font_file_name) {
        error = FT_New_Face(library, other_font_file_name, other_index, &other_face);

        if (error) {
            fmt::print(cerr, _("%{}: failed to create new font face\n"), argv[0]);
            exit(4);
        }
    }

    cairo_surface_t *surface;

    if (postscript_output) {
        surface = cairo_ps_surface_create(output_file_name, page_metrics::page_width,
                                          page_metrics::page_height);
    } else if (svg_output) {
        surface = cairo_svg_surface_create(output_file_name, page_metrics::page_width,
                                           page_metrics::page_height);
    } else {
        surface = cairo_pdf_surface_create(output_file_name, page_metrics::page_width,
                                           page_metrics::page_height);
        set_repeatable_pdf_metadata(surface);
    }

    cairo_status_t cr_status = cairo_surface_status(surface);
    if (cr_status != CAIRO_STATUS_SUCCESS) {
        /* TRANSLATORS: 'cairo' is a name of a library, and should be left untranslated */
        fmt::print(cerr, _("{}: failed to create cairo surface: {}\n"), argv[0],
                   cairo_status_to_string(cr_status));
        exit(1);
    }

    cairo_t *cr = cairo_create(surface);
    cr_status = cairo_status(cr);
    if (cr_status != CAIRO_STATUS_SUCCESS) {
        fmt::print(cerr, _("{}: cairo_create failed: {}\n"), argv[0],
                   cairo_status_to_string(cr_status));
        exit(1);
    }

    cairo_surface_destroy(surface);

    init_table_fonts();
    calculate_offsets(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    calc_font_scaling(face);
    draw_glyphs(blocks, cr, face, other_face);
    cairo_destroy(cr);

    return 0;
}

//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

void vm::render(lol::u8vec4 *screen) const
{
    /* FIXME performance: a 256-value LUT would be better. */
    auto &ds = m_ram.draw_state;

    /* Precompute the current palette */
    u8vec4 lut[16];
    for (int n = 0; n < 16; ++n)
        lut[n] = palette::get8(ds.pal[1][n]);

    /* Render actual screen */
    for (int y = 0; y < 128; ++y)
    for (int x = 0; x < 128; ++x)
        screen[y * 128 + x] = lut[m_ram.pixel(x, y)];
}

void vm::print_ansi(lol::ivec2 term_size,
                    uint8_t const *prev_screen) const
{
    static int const ansi_palette[] =
    {
         16, // 000000 → 000000
         17, // 1d2b53 → 00005f
         89, // 7e2553 → 87005f
         29, // 008751 → 00875f
        131, // ab5236 → ab5236
        240, // 5f574f → 5f5f5f
        251, // c2c3c7 → c6c6c6
        230, // fff1e8 → ffffdf
        197, // ff004d → ff005f
        214, // ffa300 → ffaf00
        220, // ffec27 → ffdf00
         47, // 00e436 → 00ff5f
         39, // 29adff → 00afff
        103, // 83769c → 8787af
        211, // ff77a8 → f787af
        223, // ffccaa → ffdfaf
    };

    printf("\x1b[?25l"); // hide cursor

    auto &ds = m_ram.draw_state;

    for (int y = 0; y < 2 * lol::min(64, term_size.y); y += 2)
    {
        if (prev_screen && !memcmp(&m_ram.screen[y * 64],
                                   &prev_screen[y * 64], 128))
            continue;

        printf("\x1b[%d;1H", y / 2 + 1);

        int oldfg = -1, oldbg = -1;

        for (int x = 0; x < lol::min(128, term_size.x); ++x)
        {
            int offset = y * 64 + x / 2;
            int shift = 4 * (x & 1);
            uint8_t fg = (m_ram.screen[offset] >> shift) & 0xf;
            uint8_t bg = (m_ram.screen[offset + 64] >> shift) & 0xf;
            char const *glyph = "▀";

            if (fg < bg)
            {
                std::swap(fg, bg);
                glyph = "▄";
            }

            if (fg == oldfg)
            {
                if (bg != oldbg)
                    printf("\x1b[48;5;%dm", ansi_palette[ds.pal[1][bg]]);
            }
            else
            {
                if (bg == oldbg)
                    printf("\x1b[38;5;%dm", ansi_palette[ds.pal[1][fg]]);
                else
                    printf("\x1b[38;5;%d;48;5;%dm", ansi_palette[ds.pal[1][fg]], ansi_palette[ds.pal[1][bg]]);
            }

            printf(glyph);

            oldfg = fg;
            oldbg = bg;
        }

        printf("\x1b[0m\x1b[K"); // reset properties and clear to end of line
    }

    printf("\x1b[?25h"); // show cursor
    fflush(stdout);
}

} // namespace z8


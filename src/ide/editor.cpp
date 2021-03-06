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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

#include "zepto8.h"
#include "editor.h"

namespace z8
{

static TextEditor::LanguageDefinition const& get_lang_def();
static TextEditor::Palette const &get_palette();

editor::editor()
{
    m_widget.SetLanguageDefinition(get_lang_def());
    m_widget.SetPalette(get_palette());

    // Debug text
    m_widget.SetText("-- pico-8 syntax test\n-- by sam\n\n"
        "function _init()\n cls()\n step = 1\n tmp = rnd(17)\n lst = {\"lol\"}\nend\n\n"
        "function _update()\n if (btnp(\x97) or btnp(\x8e)) step = 0\n\n"
        " if step < #lst then\n  step += 1\n end\nend\n\n"
        "function _draw()\n local x = 28\n local y = 120\n\n map(0, 0, 0, 0, 16, 16)\nend\n\n"
        "--  !\"#$%&'()*+,-./0123456789:;<=>?\n"
        "-- @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n"
        "-- `abcdefghijklmnopqrstuvwxyz{|}~\x7f\n"
        "-- \x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\n"
        "-- \x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\n");
}

editor::~editor()
{
}

void editor::render()
{
    ImGui::Begin("cODE", nullptr);
    m_widget.Render("Text Editor");
    ImGui::End();
}

static TextEditor::LanguageDefinition const& get_lang_def()
{
    static bool inited = false;
    static TextEditor::LanguageDefinition ret;

    if (!inited)
    {
        // This is the list from Lua’s luaX_tokens
        static char const* const keywords[] =
        {
            "and", "break", "do", "else", "elseif",
            "end", "false", "for", "function", "goto", "if",
            "in", "local", "nil", "not", "or", "repeat",
            "return", "then", "true", "until", "while",
        };

        for (auto& k : keywords)
            ret.mKeywords.insert(k);

        static const char* const identifiers[] =
        {
            // Implemented in pico8lib (from z8lua)
            "max", "min", "mid", "ceil", "flr", "cos", "sin", "atan2", "sqrt",
            "abs", "sgn", "band", "bor", "bxor", "bnot", "shl", "shr", "lshr",
            "rotl", "rotr", "tostr", "tonum", "srand", "rnd",
            // Implemented in the ZEPTO-8 VM
            "run", "menuitem", "reload", "peek", "peek4", "poke", "poke4",
            "memcpy", "memset", "stat", "printh", "extcmd", "_update_buttons",
            "btn", "btnp", "cursor", "print", "camera", "circ", "circfill",
            "clip", "cls", "color", "fillp", "fget", "fset", "line", "map",
            "mget", "mset", "pal", "palt", "pget", "pset", "rect", "rectfill",
            "sget", "sset", "spr", "sspr", "music", "sfx", "time",
            // Implemented in the ZEPTO-8 BIOS
            "cocreate", "coresume", "costatus", "yield", "trace", "stop",
            "count", "add", "sub", "foreach", "all", "del", "t", "dget",
            "dset", "cartdata", "load", "save", "info", "abort", "folder",
            "resume", "reboot", "dir", "ls", "flip", "mapdraw",
            // Not implemented but we should!
            "assert", "getmetatable", "setmetatable",
        };

        for (auto& k : identifiers)
        {
            TextEditor::Identifier id;
            id.mDeclaration = "Built-in function";
            ret.mIdentifiers.insert(std::make_pair(std::string(k), id));
        }

        #define ADD_COLOR(regex, index) \
            ret.mTokenRegexStrings.push_back(std::make_pair(std::string(regex), TextEditor::PaletteIndex::index))
        ADD_COLOR("(--|//).*", Comment);
        ADD_COLOR("L?\\\"(\\\\.|[^\\\"])*\\\"", String);
        ADD_COLOR("'[^']*'", String);
        ADD_COLOR("[+-]?0[xX]([0-9a-fA-F]+([.][0-9a-fA-F]*)?|[.][0-9a-fA-F]+)", Number);
        ADD_COLOR("[+-]?0[bB]([01]+([.][01]*)?|[.][0-1]+)", Number);
        ADD_COLOR("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)", Number);
        ADD_COLOR("[a-zA-Z_][a-zA-Z0-9_]*", Identifier);
        ADD_COLOR("[-\\[\\]{}!%^&*()+=~|<>?/;,.]", Punctuation);
        #undef ADD_COLOR

        ret.mCommentStart = "--\\[\\[";
        ret.mCommentEnd = "\\]\\]";
        ret.mSingleLineComment = "--";

        ret.mCaseSensitive = true;
        ret.mAutoIndentation = false;

        ret.mName = "PICO-8";

        inited = true;
    }

    return ret;
}

static uint32_t z8tou32(int n)
{
    return lol::dot(lol::uvec4(1, 1 << 8, 1 << 16, 1 << 24),
                    lol::uvec4(z8::palette::get8(n)));
}

static TextEditor::Palette const &get_palette()
{
    static TextEditor::Palette p =
    { {
        0xffffffff,                   // None
        z8tou32(z8::palette::pink),   // Keyword
        z8tou32(z8::palette::blue),   // Number
        z8tou32(z8::palette::blue),   // String
        z8tou32(z8::palette::blue),   // Char literal
        z8tou32(z8::palette::white),  // Punctuation
        0xff409090,                   // Preprocessor
        z8tou32(z8::palette::light_gray), // Identifier
        z8tou32(z8::palette::green),  // Known identifier
        0xffc040a0,                   // Preproc identifier
        z8tou32(z8::palette::indigo), // Comment (single line)
        z8tou32(z8::palette::indigo), // Comment (multi line)
        //z8tou32(z8::palette::black),  // Background
        z8tou32(z8::palette::dark_gray), // Background
        z8tou32(z8::palette::red),    // Cursor
        z8tou32(z8::palette::yellow), // Selection
        0x800020ff,                   // ErrorMarker
        0x40f08000,                   // Breakpoint
        z8tou32(z8::palette::orange), // Line number
        0x40000000,                   // Current line fill
        0x40808080,                   // Current line fill (inactive)
        0x40a0a0a0,                   // Current line edge
    } };
    return p;
}

} // namespace z8


//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include <pegtl.hh>
#include <pegtl/trace.hh>

#include "code-fixer.h"
#define WITH_PICO8 1
#include "lua53-parse.h"

using lol::String;
using lol::ivec2;
using lol::ivec3;
using lol::msg;

namespace z8
{

//
// Actions executed during analysis
//

template<typename R>
struct analyze_action : pegtl::nothing<R> {};

template<>
struct analyze_action<lua53::short_if_statement>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        UNUSED(f);
        msg::info("unsupported short_if_statement at line %ld:%ld: %s\n", in.line(), in.byte_in_line(), in.string().c_str());
    }
};

template<>
struct analyze_action<lua53::reassignment>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        msg::info("reassignment operator %ld:%ld byte %ld: %s\n", in.line(), in.byte_in_line(), in.byte(), in.string().c_str());
        f.m_reassignments.push(ivec3(in.line(), in.byte_in_line(), in.size()));
    }
};

template<>
struct analyze_action<lua53::operator_notequal>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        /* XXX: Try to remove elements that are now invalid because
         * of backtracking. See https://github.com/ColinH/PEGTL/issues/32 */
        while (f.m_notequals.count() &&
               f.m_notequals.last() >= (int)in.byte())
            f.m_notequals.pop();
        f.m_notequals.push(in.byte());
    }
};

//
// Actions executed during translation
//

#if 0
template<typename R>
struct translate_action : pegtl::nothing<R> {};

struct translate_action_verbatim
{
    static void apply(pegtl::action_input const &in, code_fixer &that)
    {
        printf("[%s]", in.string().c_str());
    }
};

template<> struct translate_action<lua53::str_keyword> : translate_action_verbatim {};
#endif

code_fixer::code_fixer(String const &code)
  : m_code(code)
{
    // Some versions of PICO-8 add this to the end of the cartridge; we have
    // to fix it to use the old syntax, but we also have to add a carriage
    // return before it, otherwise we may end up with invalid tokens such as
    // “endif” if there is no carriage return after the last “end”.
    static char const *invalid = "if(_update60)_update=function()";
    static char const *valid = "\nif(_update60)then _update=function()";

    int tofix = m_code.index_of(invalid);
    if (tofix != lol::INDEX_NONE)
    {
        m_code = m_code.sub(0, tofix) + valid + m_code.sub(tofix + strlen(invalid)) + " end";
    }
}

String code_fixer::fix()
{
    m_notequals.empty();
    m_reassignments.empty();

    msg::info("Checking grammar\n");
    pegtl::analyze< lua53::grammar >();
    msg::info("Checking code\n");
    pegtl::parse_string<lua53::grammar, analyze_action>(m_code.C(), "code", *this);
    msg::info("Code seems valid\n");

    String new_code = m_code;

    /* Fix != → ~= */
    for (auto offset : m_notequals)
    {
        ASSERT(new_code[offset] == '!');
        new_code[offset] = '~';
    }

    /* Fix a+=b → a=a+(b) etc. */
    auto lines = new_code.split('\n');
    new_code = "";
    for (int l = 0; l < lines.count(); ++l)
    {
        String line = lines[l];

        for (auto item : m_reassignments)
            if (item[0] == l + 1)
            {
                for (int byte = item[1]; byte < line.count(); ++byte)
                {
                    if (line[byte] == '=' && strchr("+-*/%", line[byte - 1]))
                    {
                        line = String::format("%s=%s%c(%s)%s",
                                              line.sub(0, byte - 1).C(),
                                              line.sub(item[1], byte - 1 - item[1]).C(),
                                              line[byte - 1],
                                              line.sub(byte + 1, item[1] + item[2] - byte - 1).C(),
                                              line.sub(item[1] + item[2]).C());
                        break;
                    }
                }
            }

        new_code += line + '\n';
    }

    return new_code;
}

#if 0
size_t code_fixer::pos_to_offset(size_t line, size_t byte_in_line)
{
    char const *parser = m_code.C();
    for (; line > 1; --line)
    {
        char const *eol = strchr(parser, '\n');
        if (eol == nullptr)
            return parser - m_code.C();
        parser = eol + 1;
    }

    return (parser - m_code.C()) + byte_in_line - 1;
}
#endif

}

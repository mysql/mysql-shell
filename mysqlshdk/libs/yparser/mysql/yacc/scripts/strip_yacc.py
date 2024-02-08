#!/usr/bin/env python3
# Copyright (c) 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import argparse
import io
import re
import sys

def strip_actions(data, outf, api_prefix, strip_yyundef):
    def peek(data):
        c = data.read(1)
        data.seek(data.tell() - 1)
        return c

    header = f"""
#include "mysqlshdk/libs/yparser/mysql/yacc/parser_interface.h"
"""

    out = []
    nesting = 0
    mlcomment = 0
    instring = None
    lnum = 1
    params_added = False
    prefix_added = False

    def add_prefix():
        nonlocal out
        nonlocal prefix_added
        nonlocal lnum
        out.append(f"%define api.prefix {{{api_prefix}}}\n")
        prefix_added = True
        lnum += 1

    while True:
        c = data.read(1)

        if c == "\n":
            lnum += 1

        if not c:
            break

        assert nesting >= 0

        if c == "/" and peek(data) == "*" and instring is None:
            assert mlcomment == 0, f"{lnum}: {instring} {mlcomment} {nesting}"
            mlcomment += 1
        elif c == "/" and peek(data) == "/" and instring is None:
            if nesting == 0:
                out.append(c)
                out.append(data.readline())

            lnum += 1
            continue
        elif c == "*" and peek(data) == "/" and instring is None:
            assert mlcomment == 1, f"{lnum}: ins={instring} mlc={mlcomment} nest={nesting}"
            mlcomment -= 1
        elif c == "}" and (mlcomment == 0 and instring is None):
            if out[-2] == "%" and out[-3] == "\n": # %}
                out.append("\n%")

            nesting -= 1
        elif c in ["'", '"'] and mlcomment == 0 and nesting == 0:
            if instring == c:
                instring = None
            elif instring is None:
                instring = c
        elif c == "%" and out[-1][-1] == "\n":
            p = data.tell()
            line = data.readline()

            if "api.prefix" in line:
                add_prefix()
                continue

            if line.startswith("parse-param") or line.startswith("lex-param"):
                if not params_added:
                    out.append("%code requires {\n")
                    out.append(header)
                    out.append("}\n")
                    out.append("%lex-param { struct internal::Parser_context *context }\n")
                    out.append("%parse-param { struct internal::Parser_context *context }\n")
                    params_added = True

                lnum += 1

                if "Parse_tree_root" in line or "THD" in line or "Hint_scanner" in line or "PT_hint_list" in line:
                    continue

                out.append("%"+line)
                continue
            elif strip_yyundef and line.startswith("token") and "YYUNDEF" in line:
                continue
            elif "%" == line.strip() and not prefix_added:
                add_prefix()
                out.append("%"+line)
                continue
            else:
                data.seek(p)

        if nesting == 0:
            out.append(c)

        if c == "{" and (mlcomment == 0 and instring is None):
            nesting += 1

    outf.write("".join(out))

def main():
    parser = argparse.ArgumentParser(description="Strip YACC file")
    parser.add_argument("src", help="Source file")
    parser.add_argument("dst", help="Destination file")
    parser.add_argument("prefix", help="API prefix")
    parser.add_argument("--strip-yyundef", help="Do not emit YYUNDEF token", action="store_true")
    args = parser.parse_args()

    strip_actions(open(args.src, encoding="utf8"), open(args.dst, "w+", encoding="utf8"), args.prefix, args.strip_yyundef)

if __name__ == "__main__":
    main()

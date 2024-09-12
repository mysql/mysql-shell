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
import re
import subprocess

# external symbol defined in a section
external_symbol = re.compile("^.+SECT.+External\s+\|\s+(\S+).*$")

def mangled_symbols(lib):
    p = subprocess.Popen(['dumpbin', '/symbols', lib], encoding="ascii", stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    for line in p.stdout:
        m = external_symbol.match(line)

        if m is not None:
            yield m.group(1)

    p.wait()

selected_symbols = set([
    "get_charset",
    "get_charset_by_csname",
    "get_collation_number",
    "my_charset_latin1",
    "my_charset_utf8mb4_0900_ai_ci",
    "my_charset_utf8mb4_bin",
    "my_charset_utf8mb4_general_ci",
    "my_free",
    "my_init",
    "my_thread_end",
    "my_thread_init",
    "strmake_root",
    "AllocSlow@MEM_ROOT",
    "Clear@MEM_ROOT",
    "s_dummy_target@MEM_ROOT",
])

symbol_name = re.compile("\?(.+?)@@.*")

def use_symbol(mangled):
    m = symbol_name.search(mangled)

    if m is not None and m.group(1) in selected_symbols:
        return m.group(0)

    return None

def main():
    parser = argparse.ArgumentParser(description="Extract symbols from static libraries")
    parser.add_argument("--out", help="Output def file", required=True)
    parser.add_argument('libs', metavar='lib', nargs='+', help='Static library')
    args = parser.parse_args()

    with open(args.out, "w", encoding="ascii") as f:
        f.write("EXPORTS\n")

        for lib in args.libs:
            for mangled in mangled_symbols(lib):
                symbol = use_symbol(mangled)

                if symbol is not None:
                    f.write(f"{symbol}\n")

if __name__ == "__main__":
    main()

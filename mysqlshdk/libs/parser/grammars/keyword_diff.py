 #!/usr/bin/env python
 #
 # Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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
 #

import argparse
import os
import os.path
import re

def load(path):
    keywords = {}
    pattern = re.compile(r'{\s*"([^"]+)",\s*(\d)\s*},')

    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            m = re.search(pattern, line)

            if m is not None:
                keywords[m.group(1)] = m.group(2)

    return keywords

def diff(a, b):
    return dict(set(a.items()) - set(b.items()))

def output(added, removed):
    def items(l):
        for k, v in sorted(l.items()):
            print(f'    {{"{k}", {v}}},')

    print("#include <array>")
    print()
    print("namespace keyword_diff {")
    print()
    print(f"inline constexpr std::array<keyword_t, {len(added)}> added = {{{{")
    items(added)
    print("}};")
    print()
    print(f"inline constexpr std::array<keyword_t, {len(removed)}> removed = {{{{")
    items(removed)
    print("}};")
    print()
    print("}  // namespace keyword_diff")

def main():
    parser = argparse.ArgumentParser(description="Create diff between two keyword lists")
    parser.add_argument("old", help="File with the old keyword list")
    parser.add_argument("new", help="File with the new keyword list")
    args = parser.parse_args()

    old = load(args.old)
    new = load(args.new)

    added = diff(new, old)
    removed = diff(old, new)

    output(added, removed)

if __name__ == "__main__":
    main()

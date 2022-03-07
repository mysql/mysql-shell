/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/utils/document_parser.h"

#include <chrono>
#include <iostream>

int main() {
  shcore::Buffered_input input;
  shcore::Document_reader_options opts;
  opts.convert_bson_id = true;
  opts.convert_bson_types = false;
  shcore::Json_reader reader(&input, opts);

  size_t docs = 0;
  size_t docs_length = 0;

  const auto t_start = std::chrono::steady_clock::now();

  while (!reader.eof()) {
    std::string jd = reader.next();

    if (!jd.empty()) {
      ++docs;
      docs_length += jd.size();
      // std::cout << jd << '\n';
    }
  }

  const auto t_end = std::chrono::steady_clock::now();
  const auto t_int_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
  const auto bytes_per_ms = [&]() -> long long unsigned int {
    if (t_int_ms.count() == 0) {
      return 0ULL;
    }
    return docs_length / t_int_ms.count();
  }();

  std::cout << "# " << docs << " docs, " << docs_length << " bytes @ "
            << t_int_ms.count() << "ms\n";
  std::cout << "# " << bytes_per_ms / 1000.0 << " Mbytes/s\n";
}

/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/mysql/mycnf.h"
#include <cstdio>
#include <fstream>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {
namespace mycnf {

void update_options(const std::string &path, const std::string &section,
                    const std::vector<Option> &mycnf_options) {
  std::vector<Option> options(mycnf_options);
  std::ifstream ifs;
  std::ofstream ofs;
  ifs.open(path);
  if (ifs.fail()) {
    throw std::runtime_error(path + ": " + strerror(errno));
  }
  ofs.open(path + ".tmp");
  if (ofs.fail()) {
    throw std::runtime_error(path + ".tmp: " + strerror(errno));
  }

  try {
    bool found_section = false;
    bool inside_section = false;
    std::string line;
    while (std::getline(ifs, line)) {
      bool skip = false;
      if (!line.empty() && line[0] == '[') {
        inside_section = false;
        if (shcore::str_strip(line) == section) {
          found_section = true;
          inside_section = true;
        }
      } else if (inside_section) {
        std::string key;
        utils::nullable<std::string> value;
        std::tie(key, value) = shcore::str_partition(line, "=");
        for (auto opt = options.begin(); opt != options.end(); ++opt) {
          if (opt->first == key) {
            if (!opt->second) {
              // option set to NULL means erase the option
              skip = true;
            } else {
              line = key + "=" + *opt->second;
            }
            opt->first.clear();
            break;
          }
        }
      }
      if (!skip) {
        ofs << line << "\n";
      }
    }

    if (!found_section) {
      ofs << section << "\n";
    }
    for (auto opt = options.begin(); opt != options.end(); ++opt) {
      if (!opt->first.empty() && opt->second) {
        ofs << opt->first << "=" << *opt->second << "\n";
      }
    }
  } catch (...) {
    ifs.close();
    ofs.close();
    try {
      shcore::delete_file(path + ".tmp");
    } catch (...) {
    }
    throw;
  }

  ifs.close();
  ofs.close();

  shcore::copy_file(path+".tmp", path);
  try {
    shcore::delete_file(path + ".tmp");
  } catch (...) {
  }
}

}  // namespace mycnf
}  // namespace mysql
}  // namespace mysqlshdk

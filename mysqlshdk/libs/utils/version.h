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

#ifndef MYSQLSHDK_LIBS_UTILS_VERSION_H_
#define MYSQLSHDK_LIBS_UTILS_VERSION_H_

#include <string>
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace utils {
/**
 * Class to handle versions following the MySQL Standard which is
 * \<major\>.\<minor\>.\<patch\>[-more]
 */
class Version {
 public:
  Version();
  explicit Version(const std::string& version);

  int get_major() const {
    return _major;
  }
  int get_minor() const {
    return _minor ? *_minor : 0;
  }
  int get_patch() const {
    return _patch ? *_patch : 0;
  }
  std::string get_extra() const {
    return _extra ? *_extra : "";
  }

  std::string get_base() const;
  std::string get_full() const;

  bool operator<(const Version& other);
  bool operator<=(const Version& other);
  bool operator>(const Version& other);
  bool operator>=(const Version& other);
  bool operator==(const Version& other);
  bool operator!=(const Version& other);

 private:
  int _major;
  nullable<int> _minor;
  nullable<int> _patch;
  nullable<std::string> _extra;

  int parse_token(const std::string& data);
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_VERSION_H_

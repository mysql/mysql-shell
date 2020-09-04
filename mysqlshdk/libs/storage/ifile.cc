/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/ifile.h"

#include <cstdarg>
#include <vector>

#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/storage/backend/http.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/storage/utils.h"

namespace mysqlshdk {
namespace storage {

std::unique_ptr<IFile> make_file(const std::string &filepath,
                                 const File_options &options) {
  if (!options.empty()) {
    mysqlshdk::oci::Oci_options oci_options;
    bool is_oci =
        mysqlshdk::oci::parse_oci_options(filepath, options, &oci_options);

    if (is_oci) {
      return make_file(filepath, oci_options);
    }
  }

  const auto scheme = utils::get_scheme(filepath);
  if (scheme.empty() || utils::scheme_matches(scheme, "file")) {
    backend::File::Options file_options;

    auto it = options.find("file.mmap");
    if (it != options.end()) {
      try {
        file_options.mmap = backend::to_mmap_preference(it->second);
      } catch (...) {
        throw std::invalid_argument("Invalid value '" + it->second +
                                    "' for option file.mmap");
      }
    }
    return std::make_unique<backend::File>(filepath, file_options);
  } else if (utils::scheme_matches(scheme, "http") ||
             utils::scheme_matches(scheme, "https")) {
    return std::make_unique<backend::Http_get>(filepath);
  }

  throw std::invalid_argument("File handling for " + scheme +
                              " protocol is not supported.");
}

std::unique_ptr<IFile> make_file(const std::string &filepath,
                                 const mysqlshdk::oci::Oci_options &options) {
  if (options) {
    return std::make_unique<backend::oci::Object>(options, filepath);
  } else {
    return make_file(filepath);
  }
}

int fprintf(IFile *file, const char *format, ...) {
  constexpr int BUFSIZE = 2048;
  char buf[BUFSIZE];
  va_list args, copy;
  va_start(args, format);
  va_copy(copy, args);
  int ret = vsnprintf(buf, BUFSIZE, format, args);
  if (ret >= BUFSIZE) {
    std::vector<char> vec(ret + 1u);
    ret = vsnprintf(&vec[0], vec.size(), format, copy);
    assert(ret < static_cast<int>(vec.size()));
    if (ret > 0) ret = file->write(&vec[0], ret);
  } else if (ret > 0) {
    ret = file->write(buf, ret);
  }
  va_end(args);
  va_end(copy);
  return ret;
}

int fputs(const char *s, IFile *file) {
  int ret = file->write(s, strlen(s));
  return ret >= 0 ? ret : EOF;
}

int fputs(const std::string &s, IFile *file) {
  int ret = file->write(s.c_str(), s.length());
  return ret >= 0 ? ret : EOF;
}

std::string read_file(IFile *file) {
  std::string buffer;
  size_t size = file->file_size();

  buffer.resize(size);

  auto data_read = file->read(&buffer[0], size);
  if (data_read < 0) {
    throw std::runtime_error("Error reading " + file->full_path() + ": " +
                             shcore::errno_to_string(file->error()));
  }

  buffer.resize(data_read);

  return buffer;
}

}  // namespace storage
}  // namespace mysqlshdk

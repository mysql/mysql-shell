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

#include "mysqlshdk/libs/storage/backend/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cassert>

#if defined(_WIN32)
#include <io.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <utility>

#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

// initial size of an mmapped file opened for writing
constexpr const size_t k_initial_mmapped_file_size = 1024 * 1024;

Mmap_preference to_mmap_preference(const std::string &s) {
  auto ls = shcore::str_lower(s);
  if (ls.empty() || ls == "off") return Mmap_preference::OFF;
  if (ls == "on") return Mmap_preference::ON;
  if (ls == "required") return Mmap_preference::REQUIRED;
  throw std::logic_error("Invalid value '" + s +
                         "', must be one of off, on or required");
}

File::File(const std::string &filename, const File::Options &options)
    : m_use_mmap(options.mmap) {
  // no mmap in < 64bits archs
  if (sizeof(void *) < 8) m_use_mmap = Mmap_preference::OFF;

  const auto expanded =
      shcore::path::expand_user(utils::strip_scheme(filename, "file"));
  m_filepath = shcore::get_absolute_path(expanded);
}

File::~File() {
  try {
    do_close();
  } catch (const std::exception &e) {
    log_error("Unhandled error closing file object for '%s': %s",
              m_filepath.c_str(), e.what());
  }
}

void File::open(Mode m) {
  assert(!is_open());
#ifdef _WIN32
  const auto wpath = shcore::utf8_to_wide(m_filepath);
#endif

  m_writing = (m != Mode::READ);
  bool do_chmod = true;

#ifdef _WIN32
  const wchar_t *mode =
      m == Mode::READ ? L"rb" : m == Mode::WRITE ? L"wb" : L"ab";
  m_file = _wfsopen(wpath.c_str(), mode, _SH_DENYWR);

  if (m_use_mmap == Mmap_preference::REQUIRED)
    throw std::invalid_argument("mmap() not supported");
#else   // !_WIN32
  const char *mode = m == Mode::READ ? "rb" : m == Mode::WRITE ? "wb" : "ab";

  if (m_use_mmap != Mmap_preference::OFF && m == Mode::WRITE) {
    // We need to open the file as O_RDWR in case we need to mmap it later,
    // otherwise that won't work.
    int fd = ::open(m_filepath.c_str(), O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd >= 0) {
      m_file = fdopen(fd, mode);
      do_chmod = true;
    }
  } else {
    m_file = fopen(m_filepath.c_str(), mode);
  }
#endif  // !_WIN32

  if (!is_open()) {
    throw std::runtime_error("Cannot open file '" + full_path() +
                             "': " + shcore::errno_to_string(errno));
  }

  if (do_chmod) {
    int ret = 0;

#ifdef _WIN32
    if (m != Mode::READ) ret = _wchmod(wpath.c_str(), _S_IREAD | _S_IWRITE);
#else   // !_WIN32
    if (m != Mode::READ)
      ret = chmod(m_filepath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP);
#endif  // !_WIN32

    if (ret) {
      throw std::runtime_error("Unable to set permissions on file '" +
                               full_path() +
                               "': " + shcore::errno_to_string(errno));
    }
  }
}

bool File::is_open() const { return m_file != nullptr; }

int File::error() const { return ferror(m_file); }

void File::close() { do_close(); }

void File::do_close() {
#ifndef _WIN32
  if (m_mmap_ptr) {
    if (m_writing) {
      if (m_mmap_used > 0 && msync(m_mmap_ptr, m_mmap_used, MS_SYNC) < 0) {
        log_warning("%s: Error syncing mmapped file: %s", m_filepath.c_str(),
                    shcore::errno_to_string(errno).c_str());
      }
      if (munmap(m_mmap_ptr, m_mmap_available) < 0) {
        log_warning("%s: Error unmapping file: %s", m_filepath.c_str(),
                    shcore::errno_to_string(errno).c_str());
      }
      if (m_file) {
        if (ftruncate(fileno(m_file), m_mmap_used) < 0) {
          int e = errno;
          log_warning("%s: Error truncating mmapped file: %s",
                      m_filepath.c_str(),
                      shcore::errno_to_string(errno).c_str());

          fclose(m_file);
          m_file = nullptr;
          throw std::runtime_error("Error truncating file '" + full_path() +
                                   "': " + shcore::errno_to_string(e));
        }
      }
    } else {
      if (munmap(m_mmap_ptr, m_mmap_available) < 0) {
        log_warning("%s: Error unmapping file: %s", m_filepath.c_str(),
                    shcore::errno_to_string(errno).c_str());
      }
    }
    m_mmap_ptr = nullptr;
  }
#endif

  if (m_file != nullptr) {
    fclose(m_file);
    m_file = nullptr;
  }
}

size_t File::file_size() const {
  if (m_mmap_ptr && m_writing) return m_mmap_used;
  return shcore::file_size(m_filepath);
}

std::string File::full_path() const { return m_filepath; }

off64_t File::seek(off64_t offset) {
  assert(is_open());

#if defined(_WIN32)
  return _fseeki64(m_file, offset, SEEK_SET);
#else
  if (m_mmap_ptr) {
    assert(offset < static_cast<off64_t>(m_mmap_used));
    m_mmap_offset = offset;
    return offset;
  }
  return fseeko(m_file, offset, SEEK_SET);
#endif
}

off64_t File::tell() const {
  assert(is_open());

#if defined(_WIN32)
  return _ftelli64(m_file);
#else
  if (m_mmap_ptr) return m_mmap_offset;
  return ftello(m_file);
#endif
}

ssize_t File::read(void *buffer, size_t length) {
  assert(is_open());
  if (m_mmap_ptr)
    throw std::logic_error("operation not allowed on a mmapped file");

  return fread(buffer, 1, length, m_file);
}

ssize_t File::write(const void *buffer, size_t length) {
  assert(is_open());
  if (m_mmap_ptr)
    throw std::logic_error("operation not allowed on a mmapped file");

  return fwrite(buffer, 1, length, m_file);
}

bool File::flush() {
  assert(is_open());

  if (m_mmap_ptr && m_mmap_used > 0) {
#ifndef _WIN32
    msync(m_mmap_ptr, m_mmap_used, MS_ASYNC);
    if (m_file) {
      if (ftruncate(fileno(m_file), m_mmap_used) < 0) {
        throw std::runtime_error("Error truncating file '" + full_path() +
                                 "': " + shcore::errno_to_string(errno));
      }
    }
#endif
    return true;
  }
  return fflush(m_file);
}

bool File::exists() const { return shcore::is_file(full_path()); }

std::unique_ptr<IDirectory> File::parent() const {
  return make_directory(shcore::path::dirname(full_path()));
}

void File::rename(const std::string &new_name) {
  assert(!is_open());

  if (std::any_of(std::begin(new_name), std::end(new_name),
                  shcore::path::is_path_separator)) {
    throw std::logic_error("Move operation is not supported.");
  }

  auto new_path =
      shcore::path::join_path(shcore::path::dirname(full_path()), new_name);

  if (exists()) {
    shcore::rename_file(full_path(), new_path);
  }

  m_filepath = std::move(new_path);
}

void File::remove() { shcore::delete_file(m_filepath); }

std::string File::filename() const {
  return shcore::path::basename(full_path());
}

char *File::mmap_will_write(size_t length, size_t *out_avail) {
  assert(is_open());

#ifdef _WIN32
  return nullptr;
#else
  if (!m_writing) throw std::logic_error("file not open for writing");
  if (m_use_mmap == Mmap_preference::OFF || sizeof(void *) < 8) return nullptr;

  if (!m_mmap_ptr || m_mmap_offset + length > m_mmap_available) {
    if (m_mmap_ptr) {
      if (m_mmap_used > 0 && msync(m_mmap_ptr, m_mmap_used, MS_ASYNC) < 0) {
        throw std::runtime_error("Could not sync mmapped file '" + full_path() +
                                 "': " + shcore::errno_to_string(errno));
      }
      if (munmap(m_mmap_ptr, m_mmap_available) < 0) {
        throw std::runtime_error("Could not unmap mmapped file '" +
                                 full_path() +
                                 "': " + shcore::errno_to_string(errno));
      }
      m_mmap_ptr = nullptr;
    } else {
      m_mmap_used = shcore::file_size(m_filepath);
      m_mmap_available = std::max(k_initial_mmapped_file_size, m_mmap_used);
      m_mmap_offset = 0;
    }

    int fd = fileno(m_file);

    while (m_mmap_offset + length > m_mmap_available) {
      m_mmap_available *= 2;
    }
    if (::ftruncate(fd, m_mmap_available - 1) < 0) {
      if (m_use_mmap == Mmap_preference::REQUIRED) {
        throw std::runtime_error("Could not extend mmapped file '" +
                                 full_path() +
                                 "': " + shcore::errno_to_string(errno));
      } else {
        m_mmap_available = 0;
        return nullptr;
      }
    }

    int flags = MAP_SHARED;
#ifdef __APPLE__
    // files opened for writing are write-only, so we don't need the OS to keep
    // pages around once they're written
    flags |= MAP_NOCACHE;
#endif

    m_mmap_ptr = static_cast<char *>(
        ::mmap(0, m_mmap_available, PROT_WRITE | PROT_READ, flags, fd, 0));
    if (!m_mmap_ptr || m_mmap_ptr == MAP_FAILED) {
      m_mmap_ptr = nullptr;

      if (m_use_mmap == Mmap_preference::REQUIRED)
        throw std::runtime_error("Could not mmap file '" + full_path() +
                                 "': " + shcore::errno_to_string(errno));
      return nullptr;
    }
  }

  if (out_avail) {
    *out_avail = m_mmap_available - m_mmap_offset;
    assert(*out_avail >= length);
  }

  return m_mmap_ptr + m_mmap_offset;
#endif
}

char *File::mmap_did_write(size_t length, size_t *out_avail) {
  assert(is_open());

  if (!m_mmap_ptr || !m_writing)
    throw std::logic_error("file must be open for writing and mmapped");

  if (m_mmap_offset + length > m_mmap_available)
    throw std::logic_error("wrote too much???");

  m_mmap_offset += length;

  if (m_mmap_offset > m_mmap_used) m_mmap_used = m_mmap_offset;

  if (out_avail) {
    *out_avail = m_mmap_available - m_mmap_offset;
  }

  return m_mmap_ptr + m_mmap_offset;
}

const char *File::mmap_will_read(size_t *out_avail) {
  assert(is_open());
#ifdef _WIN32
  return nullptr;
#else
  if (m_writing) throw std::logic_error("file must be open for reading");
  if (m_use_mmap == Mmap_preference::OFF || sizeof(void *) < 8) {
    if (out_avail) *out_avail = 0;
    return nullptr;
  }

  if (!m_mmap_ptr) {
    m_mmap_available = file_size();
    m_mmap_used = m_mmap_available;
    m_mmap_offset = 0;

    m_mmap_ptr = static_cast<char *>(
        ::mmap(0, m_mmap_available, PROT_READ, MAP_SHARED, fileno(m_file), 0));

    if (!m_mmap_ptr || m_mmap_ptr == MAP_FAILED) {
      m_mmap_ptr = nullptr;
      m_mmap_available = 0;
      m_mmap_used = 0;
      if (m_use_mmap == Mmap_preference::REQUIRED) {
        int e = errno;
        fclose(m_file);
        m_file = nullptr;
        errno = e;
        throw std::runtime_error("Could not mmap file '" + m_filepath +
                                 "': " + shcore::errno_to_string(errno));
      }
      if (out_avail) *out_avail = 0;
      return nullptr;
    }
  }

  assert(m_mmap_offset <= m_mmap_available);

  if (out_avail) *out_avail = m_mmap_available - m_mmap_offset;

  return m_mmap_ptr + m_mmap_offset;
#endif
}

const char *File::mmap_did_read(size_t length, size_t *out_avail) {
  assert(is_open());

  if (!m_mmap_ptr || m_writing)
    throw std::logic_error("file must be open for reading and mmapped");

  assert(m_mmap_offset <= m_mmap_available);

  if (m_mmap_offset + length > m_mmap_available)
    throw std::logic_error("read too much???");

  m_mmap_offset += length;

  if (out_avail) *out_avail = m_mmap_available - m_mmap_offset;

  return m_mmap_ptr + m_mmap_offset;
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

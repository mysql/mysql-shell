/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include <memory>
#include <random>
#include <utility>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace storage {
namespace tests {

TEST(Storage, file_write_truncate) {
  auto file =
      make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"));
  file->open(mysqlshdk::storage::Mode::WRITE);
  file->write("somedata", 8);
  file->close();

  // opening an existing file for writing should truncate old contents
  file->open(mysqlshdk::storage::Mode::WRITE);
  file->write("some", 4);
  file->close();

  std::string buffer;
  buffer.resize(1024);
  size_t r;
  file->open(mysqlshdk::storage::Mode::READ);
  r = file->read(&buffer[0], buffer.size());
  buffer.resize(r);

  EXPECT_EQ("some", buffer);
}

TEST(Storage, fprintf_off_by_1) {
  auto path = shcore::path::join_path(getenv("TMPDIR"), "testfile.txt");

  for (int i = 2040; i < 2055; i++) {
    std::string data(i, '#');
    auto file = make_file(path);
    file->open(mysqlshdk::storage::Mode::WRITE);
    fprintf(file.get(), "[%s]", data.c_str());
    file->close();

    auto written = shcore::get_text_file(path);

    EXPECT_EQ("[" + data + "]", written) << i;
  }
}

#ifndef _WIN32
TEST(Storage, file_mmap_option) {
  // mmap disabled by default even if requested
  {
    auto ifile =
        make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"));
    auto file = dynamic_cast<backend::File *>(ifile.get());

    file->open(Mode::WRITE);
    EXPECT_EQ(nullptr, file->mmap_will_write(1024));
    EXPECT_THROW(file->mmap_did_write(0), std::logic_error);
    file->close();
    file->open(Mode::READ);
    EXPECT_EQ(nullptr, file->mmap_will_read(nullptr));
    EXPECT_THROW(file->mmap_did_read(0), std::logic_error);
    file->close();
    file->remove();
  }
  {
    auto ifile =
        make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"),
                  {{"file.mmap", "off"}});
    auto file = dynamic_cast<backend::File *>(ifile.get());
    file->open(Mode::WRITE);
    EXPECT_EQ(nullptr, file->mmap_will_write(1024));
    EXPECT_THROW(file->mmap_did_write(0), std::logic_error);
    file->close();
    file->open(Mode::READ);
    EXPECT_EQ(nullptr, file->mmap_will_read(nullptr));
    EXPECT_THROW(file->mmap_did_read(0), std::logic_error);
    file->close();
    file->remove();
  }

  // find out the next fd that will be opened
  int next_fd = dup(0);
  ASSERT_GE(next_fd, 0);
  ::close(next_fd);

  // mmap on should work but fallback to plain reading if mmap() fails
  {
    auto ifile =
        make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"),
                  {{"file.mmap", "on"}});
    auto file = dynamic_cast<backend::File *>(ifile.get());
    file->open(Mode::WRITE);

    if (sizeof(void *) < 8) {
      // no mmap in 32bits
      EXPECT_EQ(nullptr, file->mmap_will_write(1024));
    } else {
      char *ptr;
      EXPECT_NE(nullptr, ptr = file->mmap_will_write(1024));
      ptr[0] = '!';
      EXPECT_EQ(ptr + 1, file->mmap_did_write(1));
    }
    file->close();

    file->open(Mode::WRITE);
    // force mmap error by closing the fd of the opened file
    ::close(next_fd);

    // mmap should fail but fallback
    EXPECT_EQ(nullptr, file->mmap_will_write(1024));
    // this write won't work because we closed the fd, but it shouldn't throw
    // either
    file->write("xxx", 3);
    file->close();

    shcore::create_file(file->full_path().real(), "123");

    file->open(Mode::READ);
    if (sizeof(void *) < 8)
      EXPECT_EQ(nullptr, file->mmap_will_read(nullptr));
    else
      EXPECT_NE(nullptr, file->mmap_will_read(nullptr));
    file->close();

    file->open(Mode::READ);
    // force mmap error by closing the fd of the opened file
    ::close(next_fd);

    std::string buf;
    buf.resize(3);
    // mmap should fail but fallback
    EXPECT_EQ(nullptr, file->mmap_will_read(nullptr));
    // this read won't work because we closed the fd, but it shouldn't throw
    // either
    file->read(&buf, buf.size());
    file->close();
    file->remove();
  }
  // mmap should work and throw error if mmap() fails
  {
    auto ifile =
        make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"),
                  {{"file.mmap", "required"}});
    auto file = dynamic_cast<backend::File *>(ifile.get());
    file->open(Mode::WRITE);

    if (sizeof(void *) < 8) {
      EXPECT_EQ(nullptr, file->mmap_will_write(1024));
    } else {
      char *ptr;
      EXPECT_NE(nullptr, ptr = file->mmap_will_write(1024));
      ptr[0] = '!';
      EXPECT_EQ(ptr + 1, file->mmap_did_write(1));
    }
    file->close();

    if (sizeof(void *) >= 8) {
      file->open(Mode::WRITE);
      // force mmap error by closing the fd of the opened file
      ::close(next_fd);
      EXPECT_THROW(file->mmap_will_write(1), std::runtime_error);
      file->close();
    }

    shcore::create_file(file->full_path().real(), "xxx");

    file->open(Mode::READ);
    if (sizeof(void *) < 8)
      EXPECT_EQ(nullptr, file->mmap_will_read(nullptr));
    else
      EXPECT_NE(nullptr, file->mmap_will_read(nullptr));
    file->close();

    if (sizeof(void *) >= 8) {
      file->open(Mode::READ);
      // force mmap error by closing the fd of the opened file
      ::close(next_fd);
      EXPECT_THROW(file->mmap_will_read(nullptr), std::runtime_error);
      file->close();
      file->remove();
    }
  }
}

TEST(Storage, file_mmap_write) {
  if (sizeof(void *) < 8) SKIP_TEST("no mmap in 32bits");

  auto ifile =
      make_file(shcore::path::join_path(getenv("TMPDIR"), "testfile.txt"),
                {{"file.mmap", "required"}});
  auto file = dynamic_cast<backend::File *>(ifile.get());
  file->open(Mode::WRITE);

  // This reproduces a bug where mmap() would try to sync with m_mmap_used == 0
  size_t avail = 0;
  file->mmap_will_write(10, &avail);
  file->mmap_will_write(67240458, &avail);
  file->close();

  file->open(Mode::WRITE);

  EXPECT_EQ(0, file->file_size());

  char *ptr = file->mmap_will_write(32, &avail);
  EXPECT_GE(avail, 32);

  EXPECT_EQ(0, file->file_size());

  std::string line = "hello world\n";
  memcpy(ptr, line.c_str(), line.size());

  // we wrote but didn't advance the file
  EXPECT_EQ(0, file->file_size());
  EXPECT_EQ(0, file->tell());

  size_t navail = 0;
  // ensure did_write advances the ptr
  EXPECT_EQ(ptr + line.size(), file->mmap_did_write(line.size(), &navail));
  EXPECT_EQ(avail - line.size(), navail);

  EXPECT_EQ(line.size(), file->tell());
  EXPECT_EQ(line.size(), file->file_size());

  ptr = file->mmap_will_write(32, &avail);

  std::string line2 = "second line\n";
  memcpy(ptr, line2.c_str(), line2.size());

  EXPECT_EQ(ptr + line2.size(), file->mmap_did_write(line.size(), &navail));

  EXPECT_EQ(line.size() + line2.size(), file->file_size());

  file->seek(line.size());
  EXPECT_EQ(line.size(), file->tell());

  ptr = file->mmap_will_write(32, &avail);
  memcpy(ptr, "SECOND", 6);
  EXPECT_EQ(ptr + 6, file->mmap_did_write(6, &navail));
  EXPECT_EQ(line.size() + 6, file->tell());

  // file size shouldn't have changed
  EXPECT_EQ(line.size() + line2.size(), file->file_size());

  std::string buf("xxx");
  EXPECT_THROW(file->read(&buf[0], buf.size()), std::logic_error);
  EXPECT_THROW(file->write(&buf[0], buf.size()), std::logic_error);
  EXPECT_THROW(file->mmap_will_read(0), std::logic_error);
  EXPECT_THROW(file->mmap_did_read(0), std::logic_error);

  file->close();

  EXPECT_EQ(line.size() + line2.size(), file->file_size());

  EXPECT_EQ("hello world\nSECOND line\n",
            shcore::get_text_file(ifile->full_path().real()));

  file->remove();
}

TEST(Storage, file_mmap_read) {
  if (sizeof(void *) < 8) SKIP_TEST("no mmap in 32bits");

  auto path = shcore::path::join_path(getenv("TMPDIR"), "testfile.txt");
  std::string line1 = "first line\n";
  std::string line2 = "second line\n";
  shcore::create_file(path, line1 + line2);

  auto ifile = make_file(path, {{"file.mmap", "required"}});
  auto file = dynamic_cast<backend::File *>(ifile.get());
  file->open(Mode::READ);

  EXPECT_EQ(line1.size() + line2.size(), file->file_size());
  EXPECT_EQ(0, file->tell());

  const char *ptr;
  size_t avail;

  ptr = file->mmap_will_read(&avail);
  EXPECT_EQ(line1.size() + line2.size(), avail);
  EXPECT_EQ(0, strncmp(ptr, line1.c_str(), line1.size()));

  ptr = file->mmap_did_read(line1.size(), &avail);
  EXPECT_EQ(line1.size(), file->tell());
  EXPECT_EQ(line2.size(), avail);

  ptr = file->mmap_will_read(&avail);
  EXPECT_EQ(line2.size(), avail);
  EXPECT_EQ(0, strncmp(ptr, line2.c_str(), line2.size()));

  ptr = file->mmap_did_read(line2.size(), &avail);
  EXPECT_EQ(line1.size() + line2.size(), file->tell());
  EXPECT_EQ(0, avail);

  file->seek(1);
  ptr = file->mmap_will_read(&avail);
  EXPECT_EQ(line1.size() + line2.size() - 1, avail);
  EXPECT_EQ(0, strncmp(ptr, line1.c_str() + 1, line1.size() - 1));

  std::string buf("xxx");
  EXPECT_THROW(file->read(&buf[0], buf.size()), std::logic_error);
  EXPECT_THROW(file->write(&buf[0], buf.size()), std::logic_error);
  EXPECT_THROW(file->mmap_will_write(0), std::logic_error);
  EXPECT_THROW(file->mmap_did_write(0), std::logic_error);

  file->close();
  file->remove();
}
#endif

}  // namespace tests
}  // namespace storage
}  // namespace mysqlshdk

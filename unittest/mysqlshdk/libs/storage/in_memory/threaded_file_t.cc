/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/in_memory/threaded_file.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/storage/backend/in_memory/allocated_file.h"
#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file_adapter.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {
namespace {

TEST(Threaded_file, constructor) {
  {
    // no allocator
    EXPECT_THROW(threaded_file({}), std::invalid_argument);
  }

  {
    // max memory too small
    Allocator allocator{2, 2};
    Threaded_file_config config;
    config.allocator = &allocator;
    config.max_memory = 1;

    EXPECT_THROW(threaded_file(std::move(config)), std::invalid_argument);
  }

  {
    // max memory OK
    Allocator allocator{2, 2};
    Threaded_file_config config;
    config.allocator = &allocator;
    config.max_memory = 2;

    EXPECT_NO_THROW(threaded_file(std::move(config)));
  }

  {
    // no threads
    Allocator allocator{2, 2};
    Threaded_file_config config;
    config.allocator = &allocator;
    config.max_memory = 2;
    config.threads = 0;

    EXPECT_THROW(threaded_file(std::move(config)), std::invalid_argument);
  }

  {
    // number of threads OK
    Allocator allocator{2, 2};
    Threaded_file_config config;
    config.allocator = &allocator;
    config.max_memory = 2;
    config.threads = 1;

    EXPECT_NO_THROW(threaded_file(std::move(config)));
  }
}

TEST(Threaded_file, read) {
  const std::string filename{"threaded-file-read.txt"};
  constexpr std::size_t length = 23;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  constexpr std::size_t threads = 4;
  const auto test_file = make_file(filename);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  std::string buffer;

  for (std::size_t block = 1; block <= length + 1; ++block) {
    for (std::size_t max = block; max <= (threads + 1) * block; max += block) {
      for (std::size_t read = 1; read <= length + 1; ++read) {
        SCOPED_TRACE("block size: " + std::to_string(block) +
                     ", max memory: " + std::to_string(max) +
                     ", read size: " + std::to_string(read));

        Allocator allocator{block, block};
        Threaded_file_config config;
        config.allocator = &allocator;
        config.max_memory = max;
        config.threads = threads;
        config.file_path = filename;

        // read the whole file
        buffer.clear();
        buffer.resize(length);

        auto b = buffer.data();
        std::size_t l = 0;

        const auto file = threaded_file(std::move(config));
        file->open(Mode::READ);

        while (l < length) {
          EXPECT_EQ(std::min(read, length - l), file->read(b, read));
          b += read;
          l += read;
        }

        file->close();

        // verify contents
        EXPECT_EQ(contents, buffer);
      }
    }
  }

  test_file->remove();
}

TEST(Threaded_file, read_long_threaded) {
  const std::string filename{"threaded-file-read-long-t.txt"};
  constexpr std::size_t length = 1024 * 1024;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  constexpr std::size_t threads = 4;
  const auto test_file = make_file(filename);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  Allocator allocator{1000, 100};
  Threaded_file_config config;
  config.allocator = &allocator;
  config.max_memory = 500;
  config.threads = threads;
  config.file_path = filename;

  const auto file = threaded_file(std::move(config));

  std::string buffer;
  buffer.resize(length);

  file->open(Mode::READ);
  file->read(buffer.data(), length);
  file->close();

  EXPECT_EQ(contents, buffer);

  test_file->remove();
}

TEST(Threaded_file, read_long) {
  const std::string filename{"threaded-file-read-long.txt"};
  constexpr std::size_t length = 1024 * 1024;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  const auto test_file = make_file(filename);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  std::string buffer;
  buffer.resize(length);

  test_file->open(Mode::READ);
  test_file->read(buffer.data(), length);
  test_file->close();

  EXPECT_EQ(contents, buffer);

  test_file->remove();
}

TEST(Threaded_file, read_compressed) {
  const std::string filename{"threaded-file-read.zst"};
  constexpr std::size_t length = 23;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  constexpr std::size_t threads = 4;
  const auto test_file = make_file(make_file(filename), Compression::ZSTD);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  std::string buffer;

  for (std::size_t block = 1; block <= length + 1; ++block) {
    for (std::size_t max = block; max <= (threads + 1) * block; max += block) {
      for (std::size_t read = 1; read <= length + 1; ++read) {
        SCOPED_TRACE("block size: " + std::to_string(block) +
                     ", max memory: " + std::to_string(max) +
                     ", read size: " + std::to_string(read));

        Allocator allocator{block, block};
        Threaded_file_config config;
        config.allocator = &allocator;
        config.max_memory = max;
        config.threads = threads;
        config.file_path = filename;

        // read the whole file
        buffer.clear();
        buffer.resize(length);

        auto b = buffer.data();
        std::size_t l = 0;

        const auto file = threaded_file(std::move(config));
        file->open(Mode::READ);

        while (l < length) {
          EXPECT_EQ(std::min(read, length - l), file->read(b, read));
          b += read;
          l += read;
        }

        file->close();

        // verify contents
        EXPECT_EQ(contents, buffer);
      }
    }
  }

  test_file->remove();
}

TEST(Threaded_file, read_long_compressed_threaded) {
  const std::string filename{"threaded-file-read-long-t.zst"};
  constexpr std::size_t length = 1024 * 1024;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  constexpr std::size_t threads = 4;
  const auto test_file = make_file(make_file(filename), Compression::ZSTD);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  Allocator allocator{1000, 100};
  Threaded_file_config config;
  config.allocator = &allocator;
  config.max_memory = 500;
  config.threads = threads;
  config.file_path = filename;

  const auto file = threaded_file(std::move(config));

  std::string buffer;
  buffer.resize(length);

  file->open(Mode::READ);
  file->read(buffer.data(), length);
  file->close();

  EXPECT_EQ(contents, buffer);

  test_file->remove();
}

TEST(Threaded_file, read_long_compressed) {
  const std::string filename{"threaded-file-read-long.zst"};
  constexpr std::size_t length = 1024 * 1024;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  const auto test_file = make_file(make_file(filename), Compression::ZSTD);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  std::string buffer;
  buffer.resize(length);

  test_file->open(Mode::READ);
  test_file->read(buffer.data(), length);
  test_file->close();

  EXPECT_EQ(contents, buffer);

  test_file->remove();
}

TEST(Threaded_file, extract) {
  const std::string filename{"threaded-file-extract.zst"};
  constexpr std::size_t length = 1024 * 1024;
  const std::string contents =
      shcore::get_random_string(length, "abcdefghijklmnopqrstuvwxyz");
  constexpr std::size_t threads = 4;
  const auto test_file = make_file(make_file(filename), Compression::ZSTD);

  test_file->open(Mode::WRITE);
  test_file->write(contents.c_str(), length);
  test_file->close();

  Allocator allocator{1000, 100};
  Threaded_file_config config;
  config.allocator = &allocator;
  config.max_memory = 500;
  config.threads = threads;
  config.file_path = filename;

  for (std::size_t offset : {0, 99}) {
    const auto file = threaded_file(config);
    file->open(Mode::READ);
    auto allocated =
        std::make_unique<Allocated_file>(filename, &allocator, true);

    while (true) {
      auto block = file->extract();

      if (!block->size) {
        break;
      }

      allocated->append(std::move(block));
    }

    file->close();

    auto adapter = std::make_unique<Virtual_file_adapter>(std::move(allocated));

    std::string buffer;
    buffer.resize(length - offset);

    adapter->open(Mode::READ);
    adapter->seek(offset);
    adapter->read(buffer.data(), length - offset);
    adapter->close();

    EXPECT_EQ(contents.substr(offset), buffer);
  }

  test_file->remove();
}

}  // namespace
}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

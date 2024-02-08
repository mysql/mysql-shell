/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/storage/backend/in_memory/synchronized_file.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

using set = std::unordered_set<IDirectory::File_info>;

TEST(Virtual_fs, join_path) {
  EXPECT_EQ("a/b", Virtual_fs::join_path("a", "b"));
  EXPECT_EQ("a/b/c", Virtual_fs::join_path("a/b", "c"));
}

TEST(Virtual_fs, split_path) {
  EXPECT_EQ(std::vector<std::string>{}, Virtual_fs::split_path(""));
  EXPECT_EQ(std::vector<std::string>{"a"}, Virtual_fs::split_path("a"));
  EXPECT_EQ((std::vector<std::string>{"a", "b"}),
            Virtual_fs::split_path("a/b"));
  EXPECT_EQ((std::vector<std::string>{"a", "b", "c"}),
            Virtual_fs::split_path("a/b/c"));
}

TEST(Virtual_fs, directory) {
  Virtual_fs fs{1024, 1024};

  // directory does not exist
  EXPECT_EQ(nullptr, fs.directory("dir"));
  // directory is created, it's possible to fetch it
  EXPECT_NE(nullptr, fs.create_directory("dir"));
  EXPECT_NE(nullptr, fs.directory("dir"));
  // another directory is created, both are accessible
  EXPECT_NE(nullptr, fs.create_directory("another_dir"));
  EXPECT_NE(nullptr, fs.directory("dir"));
  EXPECT_NE(nullptr, fs.directory("another_dir"));
}

TEST(Virtual_fs, create_directory) {
  Virtual_fs fs{1024, 1024};

  // invalid names
  EXPECT_THROW(fs.create_directory(""), std::runtime_error);
  EXPECT_THROW(fs.create_directory("/"), std::runtime_error);
  EXPECT_THROW(fs.create_directory("a/"), std::runtime_error);
  EXPECT_THROW(fs.create_directory("a/b"), std::runtime_error);

  // directory is created successfully
  ASSERT_NE(nullptr, fs.create_directory("dir"));
  EXPECT_EQ("dir", fs.directory("dir")->name());

  // another directory with the same name cannot be created
  EXPECT_THROW(fs.create_directory("dir"), std::runtime_error);

  // new directory is created, both are accessible
  ASSERT_NE(nullptr, fs.create_directory("another_dir"));
  EXPECT_EQ("dir", fs.directory("dir")->name());
  EXPECT_EQ("another_dir", fs.directory("another_dir")->name());

  // directly accessing the created directory gives a valid result
  EXPECT_EQ("local_dir", fs.create_directory("local_dir")->name());
}

TEST(Virtual_fs, directory_create_file) {
  Virtual_fs fs{1024, 1024};

  // additional validation that both create_directory() and directory() give the
  // same result
  auto dir = fs.create_directory("dir");
  ASSERT_NE(nullptr, dir);
  EXPECT_EQ(dir, fs.directory("dir"));
  EXPECT_EQ("dir", dir->name());

  // invalid names
  EXPECT_THROW(dir->create_file(""), std::runtime_error);
  EXPECT_THROW(dir->create_file("/"), std::runtime_error);
  EXPECT_THROW(dir->create_file("a/"), std::runtime_error);
  EXPECT_THROW(dir->create_file("a/b"), std::runtime_error);

  // file does not exist, it's created, it's not visible until it's published
  EXPECT_EQ(nullptr, dir->file("file"));
  EXPECT_TRUE(dir->list_files().empty());
  EXPECT_NE(nullptr, dir->create_file("file"));
  EXPECT_EQ(nullptr, dir->file("file"));
  EXPECT_TRUE(dir->list_files().empty());

  // file was not published, but still it's not possible to create a file with
  // the same name
  EXPECT_THROW(dir->create_file("file"), std::runtime_error);
  EXPECT_TRUE(dir->list_files().empty());

  // create another file, it's not visible until it's published
  auto file = dir->create_file("another_file");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ("another_file", file->name());
  EXPECT_EQ(nullptr, dir->file("another_file"));
  EXPECT_TRUE(dir->list_files().empty());

  // publish the file
  EXPECT_NO_THROW(dir->publish_created_file("another_file"));
  // publish the same file once again, still fine
  EXPECT_NO_THROW(dir->publish_created_file("another_file"));

  // file was published, old pointer is still valid
  EXPECT_EQ("another_file", file->name());

  // file was published it's now visible
  ASSERT_EQ(file, dir->file("another_file"));
  EXPECT_EQ("another_file", dir->file("another_file")->name());
  EXPECT_EQ((set{{"another_file", 0}}), dir->list_files());

  // it's not possible to create a file with the same name as the published one
  EXPECT_THROW(dir->create_file("another_file"), std::runtime_error);

  // create file in another directory, it's not visible in the first one
  fs.create_directory("dir2")->create_file("local_file");
  EXPECT_EQ(nullptr, dir->file("local_file"));
}

TEST(Virtual_fs, directory_list_files) {
  Virtual_fs fs{1024, 10};
  const auto buffer = "abcdefghijklmnopqrstuvwxyz";
  const auto dir = fs.create_directory("dir");
  const auto create_file = [=](const char *name, std::size_t size) {
    auto file = dir->create_file(name);
    file->open(false);
    file->write(buffer, size);
    file->close();
    dir->publish_created_file(file->name());
  };

  // create some files
  create_file("a1", 0);
  create_file("a22", 3);
  create_file("a3", 10);
  create_file("a4", 11);
  create_file("a5", 20);
  create_file("a6", 21);

  // list all files
  EXPECT_EQ((set{{"a1", 0},
                 {"a22", 3},
                 {"a3", 10},
                 {"a4", 11},
                 {"a5", 20},
                 {"a6", 21}}),
            dir->list_files());

  // filter files, exact name
  EXPECT_EQ((set{{"a3", 10}}), dir->list_files("a3"));

  // filter files with ?
  EXPECT_EQ((set{{"a1", 0}, {"a3", 10}, {"a4", 11}, {"a5", 20}, {"a6", 21}}),
            dir->list_files("a?"));

  // filter files with *
  EXPECT_EQ((set{{"a1", 0},
                 {"a22", 3},
                 {"a3", 10},
                 {"a4", 11},
                 {"a5", 20},
                 {"a6", 21}}),
            dir->list_files("a*"));

  // filter returns no files
  EXPECT_EQ((set{}), dir->list_files("b*"));
}

TEST(Virtual_fs, directory_rename_file) {
  Virtual_fs fs{1024, 10};
  const auto dir = fs.create_directory("dir");
  dir->create_file("a");

  // invalid name
  EXPECT_THROW(dir->rename_file("file", ""), std::runtime_error);
  EXPECT_THROW(dir->rename_file("file", "/"), std::runtime_error);
  EXPECT_THROW(dir->rename_file("file", "a/"), std::runtime_error);
  EXPECT_THROW(dir->rename_file("file", "a/b"), std::runtime_error);

  // non-existent file
  EXPECT_THROW(dir->rename_file("file", "b"), std::runtime_error);

  // renaming a file which hasn't been published yet
  dir->rename_file("a", "b");
  dir->publish_created_file("a");
  EXPECT_EQ(nullptr, dir->file("a"));
  dir->publish_created_file("b");
  ASSERT_NE(nullptr, dir->file("b"));
  EXPECT_EQ("b", dir->file("b")->name());

  // renaming a published file
  dir->rename_file("b", "c");
  EXPECT_EQ(nullptr, dir->file("b"));
  ASSERT_NE(nullptr, dir->file("c"));
  EXPECT_EQ("c", dir->file("c")->name());

  // renaming an open file
  dir->file("c")->open(true);
  EXPECT_NO_THROW(dir->rename_file("c", "d"));
  EXPECT_EQ(nullptr, dir->file("c"));
  ASSERT_NE(nullptr, dir->file("d"));
  EXPECT_EQ("d", dir->file("d")->name());

  // renaming to overwrite a file is not allowed - both files not published
  auto file = dir->create_file("1");
  file->open(false);
  file->write(file, 1);
  EXPECT_EQ(1, file->size());
  EXPECT_EQ(0, dir->create_file("2")->size());
  EXPECT_THROW(dir->rename_file("1", "2"), std::runtime_error);
  EXPECT_EQ(1, file->size());

  // renaming to overwrite a file is not allowed - published -> not published
  EXPECT_THROW(dir->rename_file("d", "1"), std::runtime_error);
  EXPECT_EQ(1, file->size());
  EXPECT_EQ(0, dir->file("d")->size());

  // renaming to overwrite a file is not allowed - not published -> published
  EXPECT_THROW(dir->rename_file("1", "d"), std::runtime_error);
  EXPECT_EQ(1, file->size());
  EXPECT_EQ(0, dir->file("d")->size());

  // renaming to overwrite a file is not allowed - both files published
  dir->publish_created_file("1");
  EXPECT_THROW(dir->rename_file("d", "1"), std::runtime_error);
  EXPECT_EQ(1, file->size());
  EXPECT_EQ(0, dir->file("d")->size());
}

TEST(Virtual_fs, directory_remove_file) {
  Virtual_fs fs{1024, 10};
  const auto dir = fs.create_directory("dir");

  // non-existent file
  EXPECT_THROW(dir->remove_file("file"), std::runtime_error);

  // trying to remove an open file fails - unpublished
  auto file = dir->create_file("file");
  file->open(true);
  EXPECT_THROW(dir->remove_file("file"), std::runtime_error);
  file->close();

  // removing unpublished file
  EXPECT_NO_THROW(dir->remove_file("file"));
  // file was removed, trying to remove it again throws
  EXPECT_THROW(dir->remove_file("file"), std::runtime_error);

  // trying to remove an open file fails - published
  file = dir->create_file("file");
  dir->publish_created_file("file");
  file->open(false);
  EXPECT_THROW(dir->remove_file("file"), std::runtime_error);
  file->close();

  // removing published file
  EXPECT_NO_THROW(dir->remove_file("file"));
  // file was removed, trying to remove it again throws
  EXPECT_THROW(dir->remove_file("file"), std::runtime_error);
}

TEST(Virtual_fs, file_open_close) {
  Virtual_fs fs{1024, 10};
  const auto dir = fs.create_directory("dir");
  const auto file = dir->create_file("file");

  // open the file
  EXPECT_FALSE(file->is_open());
  EXPECT_NO_THROW(file->open(true));
  EXPECT_TRUE(file->is_open());

  // opening it once again throws
  EXPECT_THROW(file->open(true), std::runtime_error);
  EXPECT_TRUE(file->is_open());

  // close the file
  EXPECT_NO_THROW(file->close());
  EXPECT_FALSE(file->is_open());

  // closing it again throws
  EXPECT_THROW(file->close(), std::runtime_error);
  EXPECT_FALSE(file->is_open());
}

TEST(Virtual_fs, file_write) {
  Virtual_fs fs{1024, 10};
  constexpr auto buffer = "abcdefghijklmnopqrstuvwxyz";
  constexpr auto buffer_length = std::char_traits<char>::length(buffer);
  const auto dir = fs.create_directory("dir");
  auto file = dir->create_file("file");

  // writing to a closed file throws
  EXPECT_THROW(file->write(buffer, 1), std::runtime_error);

  // writing to a file opened for reading throws
  file->open(true);
  EXPECT_THROW(file->write(buffer, 1), std::runtime_error);
  file->close();

  // writing using a nullptr and zero size works
  file->open(false);
  EXPECT_NO_THROW(file->write(nullptr, 0));
  file->close();

  const auto read_contents = [](Virtual_fs::IFile *f) {
    std::string result;
    result.resize(f->size());

    f->open(true);
    f->read(result.data(), result.length());
    f->close();

    return result;
  };

  // writing equal chunks
  for (std::size_t i = 1; i <= buffer_length; ++i) {
    SCOPED_TRACE("chunk size: " + std::to_string(i));

    // recreate the file
    dir->remove_file("file");
    file = dir->create_file("file");
    file->open(false);

    // write the whole buffer
    auto b = buffer;
    std::size_t l = 0;

    while (l < buffer_length) {
      const auto to_write = std::min(i, buffer_length - l);
      file->write(b, to_write);
      b += to_write;
      l += to_write;
      EXPECT_EQ(l, file->size());
    }

    file->close();

    // read from buffer, verify contents
    EXPECT_EQ(buffer, read_contents(file));

    // it's not possible to open the file for writing, as it was read from
    EXPECT_THROW(file->open(false), std::runtime_error);
  }

  // overwriting the file contents
  file = dir->create_file("o");
  file->open(false);
  file->write(buffer, 15);
  EXPECT_EQ(15, file->size());
  file->seek(1);
  EXPECT_EQ(15, file->size());
  file->write(buffer + 5, 1);
  EXPECT_EQ(15, file->size());
  EXPECT_EQ(2, file->tell());
  file->write(buffer + 5, 10);
  EXPECT_EQ(15, file->size());
  EXPECT_EQ(12, file->tell());
  file->seek(14);
  file->write(buffer + 5, 10);
  EXPECT_EQ(24, file->size());
  EXPECT_EQ(24, file->tell());
  file->close();
  EXPECT_EQ("affghijklmnomnfghijklmno", read_contents(file));
}

TEST(Virtual_fs, file_read) {
  Virtual_fs fs{1024, 10};
  constexpr auto data = "abcdefghijklmnopqrstuvwxyz";
  constexpr auto data_length = std::char_traits<char>::length(data);
  const auto dir = fs.create_directory("dir");
  auto file = dir->create_file("file");
  std::string buffer;
  buffer.resize(data_length);

  // reading from a closed file throws
  EXPECT_THROW(file->read(buffer.data(), 1), std::runtime_error);

  // reading from a file opened for writing throws
  file->open(false);
  EXPECT_THROW(file->read(buffer.data(), 1), std::runtime_error);
  file->close();

  // reading using a nullptr and zero size works
  file->open(true);
  EXPECT_NO_THROW(file->read(nullptr, 0));
  file->close();

  // reading equal chunks
  for (std::size_t offset = 0; offset < data_length; ++offset) {
    for (std::size_t length = 1; length <= data_length && offset < length;
         ++length) {
      for (std::size_t i = 1; i <= length; ++i) {
        SCOPED_TRACE("chunk size: " + std::to_string(i) +
                     ", total length: " + std::to_string(length) +
                     ", starting offset: " + std::to_string(offset));

        // recreate the file
        dir->remove_file("file");
        file = dir->create_file("file");

        // write the whole buffer
        file->open(false);
        file->write(data, length);
        file->close();

        // read the whole file
        buffer.clear();
        buffer.resize(length);

        auto b = buffer.data();
        std::size_t l = 0;

        file->open(true);
        file->seek(offset);

        while (l < length) {
          const auto to_read = i;
          EXPECT_EQ(std::min(i, length - l), file->read(b, to_read));
          b += to_read;
          l += to_read;
        }

        file->close();

        // verify contents
        EXPECT_EQ(std::string(data + offset, length), buffer);
      }
    }
  }

  // seek + read - setup
  file = dir->create_file("o");
  file->open(false);
  file->write(data, data_length);
  file->close();
  file->open(true);

  buffer.clear();
  buffer.resize(data_length);

  // reading after a non-zero seek does not invalidate the starting blocks
  file->seek(1);
  file->read(buffer.data(), 10);
  EXPECT_EQ(std::string(data + 1, 10), buffer.substr(0, 10));
  EXPECT_NO_THROW(file->seek(0));

  // reading a fragment of a block does not invalidate the starting blocks
  file->read(buffer.data(), 9);
  EXPECT_EQ(std::string(data, 9), buffer.substr(0, 9));
  EXPECT_NO_THROW(file->seek(0));

  // reading the whole block means that it's no longer possible to seek there
  file->read(buffer.data(), 10);
  EXPECT_EQ(std::string(data, 10), buffer.substr(0, 10));
  EXPECT_THROW(file->seek(0), std::runtime_error);
  EXPECT_THROW(file->seek(9), std::runtime_error);

  // it's still possible to seek to the unread part
  EXPECT_NO_THROW(file->seek(10));
  EXPECT_NO_THROW(file->seek(11));

  // reading after a seek past the beginning of available block does not
  // invalidate the blocks
  file->read(buffer.data(), 10);
  EXPECT_EQ(std::string(data + 11, 10), buffer.substr(0, 10));
  EXPECT_NO_THROW(file->seek(10));

  // reading a fragment of a block does not invalidate the blocks
  file->read(buffer.data(), 9);
  EXPECT_EQ(std::string(data + 10, 9), buffer.substr(0, 9));
  EXPECT_NO_THROW(file->seek(10));

  // reading the whole block means that it's no longer possible to seek there
  file->read(buffer.data(), 10);
  EXPECT_EQ(std::string(data + 10, 10), buffer.substr(0, 10));
  EXPECT_THROW(file->seek(10), std::runtime_error);
  EXPECT_THROW(file->seek(19), std::runtime_error);

  // it's still possible to seek to the unread part
  EXPECT_NO_THROW(file->seek(20));
  EXPECT_NO_THROW(file->seek(21));
  EXPECT_NO_THROW(file->seek(data_length));
  EXPECT_NO_THROW(file->seek(data_length + 1));

  // read the rest of the file
  EXPECT_NO_THROW(file->seek(20));
  const auto remaining = data_length - 20;
  EXPECT_EQ(remaining, file->read(buffer.data(), 10));
  EXPECT_EQ(std::string(data + 20, remaining), buffer.substr(0, remaining));
  EXPECT_THROW(file->seek(20), std::runtime_error);
  EXPECT_THROW(file->seek(21), std::runtime_error);
  EXPECT_THROW(file->seek(data_length), std::runtime_error);
  EXPECT_THROW(file->seek(data_length + 1), std::runtime_error);
}

TEST(Virtual_fs, file_seek_tell) {
  Virtual_fs fs{1024, 10};
  constexpr auto data = "abcdefghijklmnopqrstuvwxyz";
  constexpr auto data_length = std::char_traits<char>::length(data);
  const auto dir = fs.create_directory("dir");
  const auto file = dir->create_file("file");

  // seek using negative value throws
  EXPECT_THROW(file->seek(-1), std::runtime_error);

  // seek when file is closed throws
  EXPECT_THROW(file->seek(0), std::runtime_error);

  // seek past the file size - empty file
  file->open(true);

  EXPECT_EQ(0, file->seek(0));
  EXPECT_EQ(0, file->tell());

  EXPECT_EQ(0, file->seek(1));
  EXPECT_EQ(0, file->tell());

  EXPECT_EQ(0, file->seek(1024));
  EXPECT_EQ(0, file->tell());

  // seek using negative value throws - open file
  EXPECT_THROW(file->seek(-1), std::runtime_error);
  file->close();

  // write some data
  file->open(false);
  file->write(data, data_length);
  file->close();
  file->open(true);

  // seek into data
  EXPECT_EQ(0, file->seek(0));
  EXPECT_EQ(0, file->tell());

  EXPECT_EQ(1, file->seek(1));
  EXPECT_EQ(1, file->tell());

  EXPECT_EQ(data_length, file->seek(data_length));
  EXPECT_EQ(data_length, file->tell());

  EXPECT_EQ(data_length, file->seek(data_length + 1));
  EXPECT_EQ(data_length, file->tell());
}

TEST(Virtual_fs, threads) {
  constexpr int thread_count = 2;
  using shcore::ssl::restricted::md5;

  struct File {
    std::string name;
    std::vector<unsigned char> hash;
  };

  Virtual_fs fs{10, 2};
  const auto dir = fs.create_directory("dir");

  std::vector<std::thread> writers;
  shcore::Synchronized_queue<File> queue;

  for (int t = 0; t < thread_count; ++t) {
    writers.emplace_back([&, t]() {
      std::random_device rd;
      std::mt19937 gen{rd()};
      std::uniform_int_distribution<> size(9, 21);

      for (int i = 0; i < 10'000; ++i) {
        auto name = std::to_string(t) + '-' + std::to_string(i);
        const auto file = dir->create_file(name);
        file->open(false);
        const auto buffer =
            shcore::get_random_string(size(gen), "0123456789ABCDEF");
        file->write(buffer.c_str(), buffer.length());
        file->close();
        dir->publish_created_file(file->name());

        queue.push({std::move(name), md5(buffer.c_str(), buffer.length())});
      }
    });
  }

  // give a head start to the writers
  shcore::sleep_ms(10);

  std::vector<std::thread> readers;

  for (int t = 0; t < thread_count; ++t) {
    readers.emplace_back([&]() {
      while (true) {
        const auto file = queue.pop();

        if (file.hash.empty()) {
          break;
        }

        const auto f = dir->file(file.name);

        std::string result;
        result.resize(f->size());

        f->open(true);
        f->read(result.data(), result.length());
        f->close();

        SCOPED_TRACE(file.name);
        EXPECT_EQ(file.hash, md5(result.c_str(), result.length()));
      }
    });
  }

  // wait for writers to finish
  for (auto &t : writers) {
    t.join();
  }

  // signalize that there's no more data to read
  queue.shutdown(thread_count);

  // wait for readers to finish
  for (auto &t : readers) {
    t.join();
  }
}

TEST(Virtual_fs, synchronized_file) {
  Virtual_fs fs{1024, 1024};

  fs.set_uses_synchronized_io([](std::string_view name) {
    return shcore::str_iendswith(name, ".blob");
  });

  const auto dir = fs.create_directory("dir");
  ASSERT_NE(nullptr, dir);

  // a synchronized file is visible right away
  EXPECT_EQ(nullptr, dir->file("file.blob"));
  EXPECT_TRUE(dir->list_files().empty());

  EXPECT_NE(nullptr, dir->create_file("file.blob"));
  EXPECT_NE(nullptr, dir->file("file.blob"));
  EXPECT_FALSE(dir->list_files().empty());

  const auto file = dir->file("file.blob");

  // file is not open, tell() throws
  EXPECT_THROW(file->tell(), std::runtime_error);

  // file is not open, so reading throws
  EXPECT_THROW(file->read(nullptr, 0), std::runtime_error);

  // open the file for reading
  EXPECT_FALSE(file->is_open());
  EXPECT_NO_THROW(file->open(true));
  EXPECT_TRUE(file->is_open());

  // file is open, tell() works
  EXPECT_EQ(0, file->tell());

  // opening it for reading for the second time throws
  EXPECT_THROW(file->open(true), std::runtime_error);
  EXPECT_TRUE(file->is_open());

  // it's opened for reading, so writing throws
  EXPECT_THROW(file->write(nullptr, 0), std::runtime_error);

  // open the file for writing
  EXPECT_NO_THROW(file->open(false));
  EXPECT_TRUE(file->is_open());

  // file is still open, tell() works
  EXPECT_EQ(0, file->tell());

  // opening it for writing for the second time throws
  EXPECT_THROW(file->open(false), std::runtime_error);
  EXPECT_TRUE(file->is_open());

  // close the file, it's closed for writing
  EXPECT_NO_THROW(file->close());
  EXPECT_TRUE(file->is_open());
  EXPECT_THROW(file->write(nullptr, 0), std::runtime_error);

  // close the file again, it's closed for reading and writing
  EXPECT_NO_THROW(file->close());
  EXPECT_FALSE(file->is_open());
  EXPECT_THROW(file->write(nullptr, 0), std::runtime_error);
  EXPECT_THROW(file->read(nullptr, 0), std::runtime_error);

  // closing it again throws
  EXPECT_THROW(file->close(), std::runtime_error);
  EXPECT_FALSE(file->is_open());

  // file is closed, tell() throws
  EXPECT_THROW(file->tell(), std::runtime_error);
}

TEST(Virtual_fs, synchronized_file_interrupts) {
  // interrupts and reading
  {
    Virtual_fs fs{1024, 1024};

    fs.set_uses_synchronized_io([](std::string_view name) {
      return shcore::str_iendswith(name, ".blob");
    });

    std::thread t{[&fs]() {
      const auto dir = fs.create_directory("dir");
      const auto file = dir->create_file("file.blob");
      file->open(true);
      EXPECT_EQ(0, file->read(nullptr, 0));
      file->close();
    }};

    shcore::sleep_ms(150);
    fs.interrupt();
    t.join();
  }

  // interrupts and writing
  {
    Virtual_fs fs{1024, 1024};

    fs.set_uses_synchronized_io([](std::string_view name) {
      return shcore::str_iendswith(name, ".blob");
    });

    std::thread t{[&fs]() {
      const auto dir = fs.create_directory("dir");
      const auto file = dir->create_file("file.blob");
      file->open(false);
      EXPECT_EQ(0, file->write(nullptr, 0));
      file->close();
    }};

    shcore::sleep_ms(150);
    fs.interrupt();
    t.join();
  }
}

TEST(Virtual_fs, synchronized_file_io) {
  const std::string buffer{"1234567890"};
  Virtual_fs fs{1024, 1024};
  const auto dir = fs.create_directory("dir");

  fs.set_uses_synchronized_io([](std::string_view name) {
    return shcore::str_iendswith(name, ".blob");
  });

  for (std::size_t i = 1; i <= buffer.length(); ++i) {
    dir->create_file(std::to_string(i) + ".blob");
  }

  dir->create_file("file.blob");

  std::thread writer{[&buffer, &dir]() {
    const auto buffer_length = buffer.length();

    // writing equal chunks
    for (std::size_t i = 1; i <= buffer_length; ++i) {
      const auto file = dir->file(std::to_string(i) + ".blob");
      SCOPED_TRACE("writer: " + file->name());

      file->open(false);

      // write the whole buffer
      auto b = buffer.c_str();
      std::size_t l = 0;

      while (l < buffer_length) {
        const auto to_write = std::min(i, buffer_length - l);
        file->write(b, to_write);
        b += to_write;
        l += to_write;
        EXPECT_EQ(l, file->size());
      }

      file->close();
    }

    // writing more than reader can read
    {
      const auto file = dir->file("file.blob");
      SCOPED_TRACE("writer: " + file->name());

      file->open(false);
      EXPECT_EQ(0, file->write(buffer.c_str(), buffer_length + 1));
      file->close();
    }
  }};

  std::thread reader{[&buffer, &dir, &fs]() {
    const auto buffer_length = buffer.length();

    // read the whole buffer
    for (std::size_t i = 1; i <= buffer_length; ++i) {
      const auto file = dir->file(std::to_string(i) + ".blob");
      SCOPED_TRACE("reader: " + file->name());

      std::string input(buffer_length, 'x');

      file->open(true);
      file->read(input.data(), buffer_length);
      file->close();

      EXPECT_EQ(buffer, input);
    }

    // reading less than writer is writing
    {
      const auto file = dir->file("file.blob");
      SCOPED_TRACE("reader: " + file->name());

      std::string input(buffer_length, 'x');

      file->open(true);
      // reader reports an error
      EXPECT_EQ(-1, file->read(input.data(), buffer_length));
      // the size of pending write is available
      const auto handle = dynamic_cast<Synchronized_file *>(file);
      ASSERT_NE(nullptr, handle);
      EXPECT_EQ(buffer_length + 1, handle->pending_write_size());

      file->close();

      // signalize that there was a problem reading
      fs.interrupt();
    }
  }};

  writer.join();
  reader.join();
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

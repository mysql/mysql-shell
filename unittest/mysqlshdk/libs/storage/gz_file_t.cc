/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"

#include <memory>
#include <random>
#include <utility>
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/storage/compressed_file.h"

namespace mysqlshdk {
namespace storage {
namespace tests {

namespace {
using byte = char;
constexpr const size_t BUFSIZE = 1 << 18;

class Generate {
 public:
  Generate() = delete;
  Generate(int a, int b) : m_distribution(a, b) {}
  Generate(const Generate &other) = default;
  Generate(Generate &&other) = default;

  Generate &operator=(const Generate &other) = default;
  Generate &operator=(Generate &&other) = default;

  virtual ~Generate() = default;

  virtual std::string bytes(size_t length) = 0;

 protected:
  std::mt19937_64 m_generator{42};
  std::uniform_int_distribution<> m_distribution;
};

class Generate_binary : public Generate {
 public:
  Generate_binary() : Generate(0, 255) {}
  Generate_binary(const Generate_binary &other) = default;
  Generate_binary(Generate_binary &&other) = default;

  Generate_binary &operator=(const Generate_binary &other) = default;
  Generate_binary &operator=(Generate_binary &&other) = default;

  ~Generate_binary() = default;

  std::string bytes(size_t length) override;
};

std::string Generate_binary::bytes(size_t length) {
  std::string s;
  s.reserve(length);
  for (size_t i = 0; i < length; i++) {
    s += m_distribution(m_generator);
  }
  return s;
}

class Generate_text : public Generate {
 public:
  Generate_text() : Generate(0, s_words.size() - 1) {}
  Generate_text(const Generate_text &other) = default;
  Generate_text(Generate_text &&other) = default;

  Generate_text &operator=(const Generate_text &other) = default;
  Generate_text &operator=(Generate_text &&other) = default;

  ~Generate_text() = default;

  std::string words(size_t number);
  std::string bytes(size_t length) override;

 private:
  static const std::vector<std::string> s_words;
};

const std::vector<std::string> Generate_text::s_words{
    "Lorem",       "ipsum",     "dolor",
    "sit",         "amet,",     "consectetur",
    "adipisicing", "elit,",     "sed",
    "do",          "eiusmod",   "tempor",
    "incididunt",  "ut",        "labore",
    "et",          "dolore",    "magna",
    "aliqua.",     "Ut",        "enim",
    "ad",          "minim",     "veniam,",
    "quis",        "nostrud",   "exercitation",
    "ullamco",     "laboris",   "nisi",
    "ut",          "aliquip",   "ex",
    "ea",          "commodo",   "consequat.",
    "Duis",        "aute",      "irure",
    "dolor",       "in",        "reprehenderit",
    "in",          "voluptate", "velit",
    "esse",        "cillum",    "dolore",
    "eu",          "fugiat",    "nulla",
    "pariatur.",   "Excepteur", "sint",
    "occaecat",    "cupidatat", "non",
    "proident,",   "sunt",      "in",
    "culpa",       "qui",       "officia",
    "deserunt",    "mollit",    "anim",
    "id",          "est",       "laborum."};

std::string Generate_text::words(size_t number) {
  if (number <= 0) return std::string{};
  std::string s(s_words[m_distribution(m_generator)]);
  for (size_t i = 1; i < number; i++) {
    s += " " + s_words[m_distribution(m_generator)];
  }
  return s;
}

std::string Generate_text::bytes(size_t length) {
  if (length <= 0) return std::string{};
  std::string s(s_words[m_distribution(m_generator)]);
  while (s.size() < length) {
    s += " " + s_words[m_distribution(m_generator)];
  }
  return s;
}

}  // namespace

void compress_decompress(const std::string &input_data) {
  using Memory_file = mysqlshdk::storage::backend::Memory_file;
  using Mode = mysqlshdk::storage::Mode;

  byte buffer[BUFSIZE];

  auto memfile = Memory_file("");
  memfile.open(Mode::WRITE);
  memfile.write(input_data.data(), input_data.size());
  memfile.close();

  // compress
  std::unique_ptr<mysqlshdk::storage::IFile> compress_storage =
      std::make_unique<Memory_file>("");
  auto compress_storage_ptr =
      dynamic_cast<Memory_file *>(compress_storage.get());
  auto compress = mysqlshdk::storage::make_file(
      std::move(compress_storage), mysqlshdk::storage::Compression::GZIP);

  memfile.open(Mode::READ);
  compress->open(Mode::WRITE);
  for (auto read_bytes = memfile.read(buffer, BUFSIZE); read_bytes > 0;
       read_bytes = memfile.read(buffer, BUFSIZE)) {
    compress->write(buffer, read_bytes);
  }
  compress->close();
  memfile.close();

  // uncompress some bytes to test if we can reopen compressed stream later
  {
    compress->open(Mode::READ);
    const ssize_t first_bytes = 100;
    auto bytes_read = compress->read(buffer, first_bytes);
    const ssize_t bytes_to_compare =
        bytes_read < first_bytes ? bytes_read : first_bytes;
    EXPECT_EQ(bytes_to_compare, bytes_read);
    {
      const auto expected = input_data.substr(0, bytes_to_compare);
      const auto actual = std::string(buffer, buffer + bytes_to_compare);
      EXPECT_EQ(expected, actual);
    }
    compress->close();
  }

  // is gzip? (gzip header startswith "\x1f\x8b")
  EXPECT_EQ(static_cast<std::string::value_type>(0x1f),
            compress_storage_ptr->content()[0]);
  EXPECT_EQ(static_cast<std::string::value_type>(0x8b),
            compress_storage_ptr->content()[1]);

  // uncompress
  auto uncompress = Memory_file("");

  compress->open(Mode::READ);
  uncompress.open(Mode::WRITE);
  for (auto read_bytes = compress->read(buffer, BUFSIZE); read_bytes > 0;
       read_bytes = compress->read(buffer, BUFSIZE)) {
    uncompress.write(buffer, read_bytes);
  }
  compress->close();
  uncompress.close();

  // check
  EXPECT_EQ(input_data.size(), uncompress.file_size());
  EXPECT_EQ(memfile.content(), uncompress.content());
  EXPECT_EQ(input_data, uncompress.content());
}

TEST(storage_gz, ascii_text) {
  {
    SCOPED_TRACE("ascii text by bytes");
    Generate_text g;
    auto input_text = g.bytes(4 * 1024 * 1024);
    compress_decompress(input_text);
  }
  {
    for (const ssize_t count : {0, 1, 2, 4, 8, 1024, 2000, 8313}) {
      SCOPED_TRACE("ascii text by word count=" + std::to_string(count));

      Generate_text g;
      auto input_text = g.words(count);
      compress_decompress(input_text);
    }
  }
}

TEST(storage_gz, ascii_text_with_null_bytes) {
  Generate_text g;
  auto input_text = g.bytes(2 * 1024 * 1024);
  for (const int pos : {21, 50, 100, 112, 4000, 43000}) {
    input_text.insert(input_text.begin() + pos, '\0');
  }
  compress_decompress(input_text);
}

TEST(storage_gz, empty_input) {
  const std::string empty;
  compress_decompress(empty);
}

TEST(storage_gz, binary_data) {
  for (const ssize_t length :
       {0, 1, 2, 4, 8, 1024, 2000, 8313, 1024 * 1024, 4 * 1024 * 1024}) {
    SCOPED_TRACE(length);
    Generate_binary g;
    auto input_data = g.bytes(length);
    compress_decompress(input_data);
  }
}

}  // namespace tests
}  // namespace storage
}  // namespace mysqlshdk

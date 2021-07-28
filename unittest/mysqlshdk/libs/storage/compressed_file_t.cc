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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"

#include <memory>
#include <random>
#include <utility>
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

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

class Compression
    : public testing::TestWithParam<
          std::tuple<mysqlshdk::storage::Compression, std::string>> {
 public:
  std::unique_ptr<mysqlshdk::storage::IFile> make_output_file() {
    auto mmap_mode = std::get<1>(GetParam());
    if (mmap_mode.empty()) {
      return std::make_unique<backend::Memory_file>("");
    } else {
      std::string fn;
      if (std::get<0>(GetParam()) == storage::Compression::GZIP)
        fn = "compressed.gz";
      else
        fn = "compressed.zst";
      auto f = make_file(shcore::path::join_path(getenv("TMPDIR"), fn),
                         {{"file.mmap", mmap_mode}});
      f->remove();  // make sure file doesn't already exist
      return f;
    }
  }

  std::string read_header(mysqlshdk::storage::IFile *f, size_t nbytes) {
    if (auto mf = dynamic_cast<backend::Memory_file *>(f)) {
      return mf->content().substr(0, nbytes);
    } else {
      std::string buffer;
      buffer.resize(nbytes);
      auto ff = make_file(f->full_path().real());
      ff->open(mysqlshdk::storage::Mode::READ);
      ff->read(&buffer[0], nbytes);
      ff->close();
      return buffer;
    }
  }

  void compress_decompress(const std::string &input_data,
                           mysqlshdk::storage::Compression ctype) {
    using Memory_file = mysqlshdk::storage::backend::Memory_file;
    using Mode = mysqlshdk::storage::Mode;

    byte buffer[BUFSIZE];

    auto memfile = Memory_file("");
    memfile.open(Mode::WRITE);
    memfile.write(input_data.data(), input_data.size());
    memfile.close();

    // compress
    std::unique_ptr<mysqlshdk::storage::IFile> compress_storage;

    compress_storage = make_output_file();

    auto compress_storage_ptr = compress_storage.get();
    auto compress =
        mysqlshdk::storage::make_file(std::move(compress_storage), ctype);

#ifdef _WIN32
    if (std::get<1>(GetParam()) == "required") {
      EXPECT_THROW(compress->open(Mode::WRITE), std::invalid_argument);
      return;
    }
#endif
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

    std::string header = read_header(compress_storage_ptr, 4);
    switch (ctype) {
      case mysqlshdk::storage::Compression::GZIP:
        // is gzip? (gzip header startswith "\x1f\x8b")
        EXPECT_EQ(static_cast<std::string::value_type>(0x1f), header[0]);
        EXPECT_EQ(static_cast<std::string::value_type>(0x8b), header[1]);
        break;

      case mysqlshdk::storage::Compression::ZSTD:
        // is zstd? (zstd header startswith "\x28\xb5\x2f\xfd")
        EXPECT_EQ(static_cast<std::string::value_type>(0x28), header[0]);
        EXPECT_EQ(static_cast<std::string::value_type>(0xb5), header[1]);
        EXPECT_EQ(static_cast<std::string::value_type>(0x2f), header[2]);
        EXPECT_EQ(static_cast<std::string::value_type>(0xfd), header[3]);
        break;

      case mysqlshdk::storage::Compression::NONE:
        break;
    }

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
};

TEST_P(Compression, ascii_text) {
  {
    SCOPED_TRACE("ascii text by bytes");
    Generate_text g;
    auto input_text = g.bytes(4 * 1024 * 1024);
    compress_decompress(input_text, std::get<0>(GetParam()));
  }
  {
    for (const ssize_t count : {0, 1, 2, 4, 8, 1024, 2000, 8313}) {
      SCOPED_TRACE("ascii text by word count=" + std::to_string(count));

      Generate_text g;
      auto input_text = g.words(count);
      compress_decompress(input_text, std::get<0>(GetParam()));
    }
  }
}

TEST_P(Compression, ascii_text_with_null_bytes) {
  Generate_text g;
  auto input_text = g.bytes(2 * 1024 * 1024);
  for (const int pos : {21, 50, 100, 112, 4000, 43000}) {
    input_text.insert(input_text.begin() + pos, '\0');
  }
  compress_decompress(input_text, std::get<0>(GetParam()));
}

TEST_P(Compression, empty_input) {
  const std::string empty;
  compress_decompress(empty, std::get<0>(GetParam()));
}

TEST_P(Compression, binary_data) {
  for (const ssize_t length :
       {0, 1, 2, 4, 8, 1024, 2000, 8313, 1024 * 1024, 4 * 1024 * 1024}) {
    SCOPED_TRACE(length);
    Generate_binary g;
    auto input_data = g.bytes(length);
    compress_decompress(input_data, std::get<0>(GetParam()));
  }
}

extern "C" const char *g_test_home;
TEST_P(Compression, compress_decompress_bigdata) {
  SKIP_TEST("Slow test");

  std::string test_file =
      shcore::path::join_path(g_test_home, "data/sql/sakila-data.sql");

  auto in_file = make_file(test_file);

  auto out_file = make_file(make_output_file(), std::get<0>(GetParam()));

  for (size_t bufsize = 1024; bufsize < 100000; bufsize += 1) {
    printf("testing with buffer size %zi\n", bufsize);

    in_file->open(Mode::READ);
    out_file->open(Mode::WRITE);

    std::string buffer;
    buffer.resize(bufsize);

    for (;;) {
      auto c = in_file->read(&buffer[0], buffer.size());
      if (c <= 0) break;
      out_file->write(&buffer[0], c);
    }
    in_file->close();
    out_file->close();

    // puts("compression done");

    auto file = make_file(test_file + ".out");
    file->open(Mode::WRITE);
    out_file->open(Mode::READ);
    for (;;) {
      auto c = out_file->read(&buffer[0], buffer.size());
      if (c <= 0) break;
      file->write(&buffer[0], c);
    }
    file->close();
    out_file->close();

    // puts("decompression done");

    file->open(Mode::READ);
    in_file->open(Mode::READ);

    std::string buffer2;
    buffer2.resize(bufsize);

    for (;;) {
      auto c = file->read(&buffer[0], buffer.size());
      auto c2 = in_file->read(&buffer2[0], buffer2.size());

      ASSERT_EQ(c, c2);
      if (c == 0) break;
      ASSERT_EQ(buffer2, buffer);
    }
    file->close();
    in_file->close();
  }
}

inline std::string fmt_compr(
    const testing::TestParamInfo<
        std::tuple<mysqlshdk::storage::Compression, std::string>> &info) {
  // get the script filename alone, without the prefix directory
  return mysqlshdk::storage::to_string(std::get<0>(info.param)) +
         (std::get<1>(info.param).empty()
              ? "_mem"
              : ("_mmap_" + std::get<1>(info.param)));
}

INSTANTIATE_TEST_SUITE_P(
    StorageCompression, Compression,
    ::testing::Values(
        std::make_tuple(mysqlshdk::storage::Compression::GZIP, ""),
        std::make_tuple(mysqlshdk::storage::Compression::ZSTD, ""),
        std::make_tuple(mysqlshdk::storage::Compression::ZSTD, "off"),
        std::make_tuple(mysqlshdk::storage::Compression::ZSTD, "on"),
        std::make_tuple(mysqlshdk::storage::Compression::ZSTD, "required")),
    fmt_compr);

}  // namespace tests
}  // namespace storage
}  // namespace mysqlshdk

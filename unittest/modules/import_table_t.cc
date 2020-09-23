/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#include <iostream>
#include <queue>
#include <random>

#include "gtest_clean.h"

#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/import_table.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlsh {
namespace import_table {

constexpr const int kBufferSize = BUFFER_SIZE;

struct Range {
  size_t begin{0};
  size_t end{0};
};

TEST(import_table, double_buffer_iteration) {
  const std::string pattern{"abcdefghijklmnopqrstuvwxyz01234567890"};
  std::string test_string;
  int file_len = (kBufferSize * 50) / pattern.size();
  for (int i = 0; i < file_len; i++) {
    test_string += pattern;
  }
  const std::string path{"import_table_repeated_string.dump"};
  shcore::create_file(path, test_string, true);

  for (int reserved : {0, 1, 2, 3, 10, 500, 512, 1024, kBufferSize / 4,
                       kBufferSize / 2, (kBufferSize / 2 + 1)}) {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    auto first = fh.begin(reserved);
    auto last = fh.end(reserved);

    std::string from_file;
    for (; first != last; ++first) {
      from_file += *first;
    }
    EXPECT_EQ(test_string, from_file);
  }

  shcore::delete_file(path, true);
}

TEST(import_table, find_char) {
  const std::string s{"1234567890abcdef"};
  {
    Find_context<char> context{};
    auto p = find(s.begin(), s.end(), '4', &context);
    EXPECT_EQ('5', *p);
    EXPECT_EQ(true, context.needle_found);
    EXPECT_EQ('3', context.preceding_element);
  }
  {
    Find_context<char> context{};
    auto p = find(s.begin(), s.end(), 'x', &context);
    EXPECT_EQ(s.end(), p);
    EXPECT_EQ(false, context.needle_found);
  }
  {
    Find_context<char> context{};
    auto p = find(s.begin(), s.begin(), 'x', &context);
    EXPECT_EQ(s.begin(), p);
    EXPECT_EQ(false, context.needle_found);
  }
}

TEST(import_table, find_needle) {
  const std::string s{"1234567890abcdef"};

  {
    Find_context<char> context{};
    const std::string single_char_needle{"4"};
    auto p = find(s.begin(), s.end(), single_char_needle.begin(),
                  single_char_needle.end(), &context);
    EXPECT_EQ('5', *p);
    EXPECT_EQ(true, context.needle_found);
    EXPECT_EQ('3', context.preceding_element);
  }

  {
    Find_context<char> context{};
    const std::string single_char_needle_missing{"x"};
    auto p = find(s.begin(), s.end(), single_char_needle_missing.begin(),
                  single_char_needle_missing.end(), &context);
    EXPECT_EQ(s.end(), p);
    EXPECT_EQ(false, context.needle_found);

    p = find(s.begin(), s.begin(), single_char_needle_missing.begin(),
             single_char_needle_missing.end(), &context);
    EXPECT_EQ(s.begin(), p);
    EXPECT_EQ(false, context.needle_found);
  }

  {
    Find_context<char> context{};
    const std::string multi_char_needle{"789"};
    auto p = find(s.begin(), s.end(), multi_char_needle.begin(),
                  multi_char_needle.end(), &context);
    EXPECT_EQ('0', *p);
    EXPECT_EQ(true, context.needle_found);
    EXPECT_EQ('6', context.preceding_element);
  }
  {
    Find_context<char> context{};
    const std::string multi_char_needle_past_end{"defg"};
    auto p = find(s.begin(), s.end(), multi_char_needle_past_end.begin(),
                  multi_char_needle_past_end.end(), &context);
    EXPECT_EQ(s.end(), p);
    EXPECT_EQ(false, context.needle_found);
  }
}

TEST(import_table, chunks) {
  const std::string line_terminator{"\n"};
  const std::string path{"import_table_1k_chunks.dump"};

  std::string test_string{};
  std::string row_string(511, '_');
  row_string += line_terminator;
  std::vector<Range> expected_ranges;

  for (size_t i = 0; i < 100; i++) {
    auto chunk_start = test_string.size();
    test_string += row_string;
    auto chunk_end = test_string.size();
    expected_ranges.emplace_back(Range{chunk_start, chunk_end});
  }

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, 1, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, chunks_2) {
  const std::string line_terminator{"\n"};
  const std::string path{"import_table_1k_chunks.dump"};

  std::string test_string{};
  std::string row_string(511, '_');
  row_string += line_terminator;
  std::vector<Range> expected_ranges;

  for (size_t i = 0; i < 100; i++) {
    auto r = Range{row_string.size() * i, row_string.size() * (i + 1)};
    expected_ranges.push_back(r);
    test_string += row_string;
  }

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, 1, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, false_line_termination) {
  const std::string line_terminator{"abc"};
  const char escape = '\\';
  const std::string path{"import_table_false_termination.dump"};

  const std::string test_string{"021345a\\nbcdefgh5678abcsecond_rowabc"};
  std::vector<Range> expected_ranges{};
  expected_ranges.emplace_back(Range{0, 23});
  expected_ranges.emplace_back(Range{23, 36});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, 1, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, escape_char_in_the_middle_if_line_terminator) {
  const std::string line_terminator{"a\\b"};
  const char escape = '\\';
  const std::string path{
      "import_table_escape_char_in_the_middle_if_line_terminator.dump"};

  const std::string test_string{
      "0123456789a\\\nb01234567890a\\b01234567a\\bnext rowa\\b"};
  std::vector<Range> expected_ranges{};
  expected_ranges.emplace_back(Range{0, 28});
  expected_ranges.emplace_back(Range{28, 39});
  expected_ranges.emplace_back(Range{39, 50});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, 1, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, escape_at_the_end_of_a_buffer) {
  const std::string line_terminator{"\n"};
  const char escape = '\\';
  const std::string path{"import_table_escape_at_the_end_of_a_buffer.dump"};

  std::vector<Range> expected_ranges;

  std::string test_string(kBufferSize, '_');
  test_string += escape;
  test_string += line_terminator;

  size_t row_start = 0;
  for (size_t i = 0; i < 10; i++) {
    test_string += std::string((i + 1) * 5, 'x');
    test_string += line_terminator;
    size_t row_end = test_string.size();
    expected_ranges.push_back(Range{row_start, row_end});
    row_start = row_end;
  }

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, 1, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, multichar_line_terminator_with_buffer_invalidation) {
  const std::string line_terminator{"ababab"};
  const std::string path{
      "import_table_multichar_line_terminator_with_buffer_invalidation.dump"};
  constexpr const int kSkipBytes = 1;

  // line terminator: ababab
  // buffer:            11abab|babababxx
  //                            ^     ^ correct row end offset
  // invalidated buffer, current buffer overwritten by next:
  //                    22baba|babababxx
  //                         ^      ^ wrong row end offset

  std::vector<Range> expected_ranges;

  std::string test_string(kSkipBytes, '_');

  std::string buf0(kBufferSize, 'p');

  // buf1: ...abab
  std::string buf1(kBufferSize, 'q');
  buf1[buf1.size() - 4] = 'a';
  buf1[buf1.size() - 3] = 'b';
  buf1[buf1.size() - 2] = 'a';
  buf1[buf1.size() - 1] = 'b';

  // buf2: bababab...
  std::string buf2(kBufferSize, 'r');
  buf2[0] = 'b';
  buf2[1] = 'a';
  buf2[2] = 'b';
  buf2[3] = 'a';
  buf2[4] = 'b';
  buf2[5] = 'a';
  buf2[6] = 'b';

  // buf3: ...baba
  std::string buf3(kBufferSize, 's');
  buf3[buf3.size() - 4] = 'b';
  buf3[buf3.size() - 3] = 'a';
  buf3[buf3.size() - 2] = 'b';
  buf3[buf3.size() - 1] = 'a';

  // buf4: ...ababab...
  std::string buf4(400, 'x');
  buf4 += line_terminator;

  test_string += buf0;
  test_string += buf1;
  test_string += buf2;
  test_string += buf3;
  test_string += buf4;

  size_t first_row_end =
      kSkipBytes + buf0.size() + buf1.size() + 1 + line_terminator.size();
  expected_ranges.push_back(Range{0, first_row_end});
  expected_ranges.push_back(Range{first_row_end, test_string.size()});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, kSkipBytes, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }
  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, false_line_terminator_after_eof) {
  const std::string line_terminator{"aaaabbbbbbbb"};
  const std::string path{"false_line_terminator_after_eof.dump"};
  constexpr const int kSkipBytes = 1;

  std::vector<Range> expected_ranges;

  std::string test_string(kSkipBytes, '_');

  const std::string buf0(kBufferSize, 'b');
  const std::string buf1(kBufferSize, 'b');
  const std::string buf2(kBufferSize - 4, 'a');

  test_string += buf0;
  test_string += buf1;
  test_string += buf2;

  expected_ranges.push_back(Range{0, test_string.size()});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, kSkipBytes, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, last_character_same_as_terminator) {
  // select * from info into outfile 'abc.txt' fields terminated by ';' lines
  // terminated by 'a';
  const std::string line_terminator{"a"};
  const char escape = '\\';
  const std::string path{"last_character_same_as_terminator.dump"};
  constexpr const int kSkipBytes = 1;

  std::vector<Range> expected_ranges;

  const std::string test_string{
      "\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a;"
      "bbbbbbbbbbbbbbbbbbbba\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      ";bbbbbbbbbbbbbbbbbbbb3333a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333\\a\\a\\a\\aa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a\\a\\a\\a\\a1111\\a;bbbbbbbbbbbbbbbbbbbb555\\aa"};

  expected_ranges.push_back(Range{0, 56});
  expected_ranges.push_back(Range{56, 116});
  expected_ranges.push_back(Range{116, 180});
  expected_ranges.push_back(Range{180, 252});
  expected_ranges.push_back(Range{252, 317});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, kSkipBytes, &r,
                       File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, onechar_a_line_terminator_same_as_row_data) {
  // select * from info into outfile 'abc.txt' fields terminated by ';' lines
  // terminated by 'a';
  const std::string line_terminator{"a"};
  const char escape = '\\';
  const std::string path{"onechar_a_line_terminator_same_as_row_data.dump"};
  constexpr const int kSkipBytes = 1;

  std::vector<Range> expected_ranges;

  const std::string test_string{
      "\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a;"
      "bbbbbbbbbbbbbbbbbbbba\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      ";bbbbbbbbbbbbbbbbbbbb3333a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333\\a\\a\\a\\aa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a\\a\\a\\a\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb555\\aa\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\aba\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\aba\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44"
      ";b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456a\\ab\\ab\\ab\\ab\\ab\\abccccc"
      "ccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abba\\ab\\ab\\ab\\ab\\ab\\"
      "abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbcca\\ab\\ab\\ab\\ab\\ab"
      "\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbcc\\aba"};

  expected_ranges.push_back(Range{0, 56});
  expected_ranges.push_back(Range{56, 116});
  expected_ranges.push_back(Range{116, 180});
  expected_ranges.push_back(Range{180, 252});
  expected_ranges.push_back(Range{252, 317});
  expected_ranges.push_back(Range{317, 362});
  expected_ranges.push_back(Range{362, 421});
  expected_ranges.push_back(Range{421, 486});
  expected_ranges.push_back(Range{486, 557});
  expected_ranges.push_back(Range{557, 630});
  expected_ranges.push_back(Range{630, 706});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, kSkipBytes, &r,
                       File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, twochar_ab_line_terminator_same_as_row_data) {
  // select * from info into outfile 'abc.txt' fields terminated by ';' lines
  // terminated by 'ab';
  const std::string line_terminator{"ab"};
  const char escape = '\\';
  const std::string path{"twochar_ab_line_terminator_same_as_row_data.dump"};
  constexpr const int kSkipBytes = 1;

  std::vector<Range> expected_ranges;

  const std::string test_string{
      "\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a;"
      "bbbbbbbbbbbbbbbbbbbbab\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a;"
      "bbbbbbbbbbbbbbbbbbbb3333ab\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333ab\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333\\a\\a\\a\\aab\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a\\a\\a\\a\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb555\\aab\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abab\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\abab\\ab\\ab\\ab\\ab\\ab\\abcccccccccc4"
      "4;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456ab\\ab\\ab\\ab\\ab\\ab\\abccccc"
      "ccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbab\\ab\\ab\\ab\\ab\\ab"
      "\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbccab\\ab\\ab\\ab\\ab\\a"
      "b\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbcc\\abab"};

  expected_ranges.push_back(Range{0, 57});
  expected_ranges.push_back(Range{57, 118});
  expected_ranges.push_back(Range{118, 183});
  expected_ranges.push_back(Range{183, 256});
  expected_ranges.push_back(Range{256, 322});
  expected_ranges.push_back(Range{322, 368});
  expected_ranges.push_back(Range{368, 428});
  expected_ranges.push_back(Range{428, 494});
  expected_ranges.push_back(Range{494, 566});
  expected_ranges.push_back(Range{566, 640});
  expected_ranges.push_back(Range{640, 717});

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, kSkipBytes, &r,
                       File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  {
    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

TEST(import_table, twochar_aa_line_terminator_same_as_row_data) {
  // select * from info into outfile 'abc.txt' fields terminated by ';' lines
  // terminated by 'aa';
  const std::string line_terminator{"aa"};
  const char escape = '\\';
  const std::string path{"twochar_aa_line_terminator_same_as_row_data.dump"};
  constexpr const int kSkipBytes = 1;

  const std::string test_string{
      "\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a;"
      "bbbbbbbbbbbbbbbbbbbbaa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\"
      "a;"
      "bbbbbbbbbbbbbbbbbbbb3333aa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333aa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb3333\\a\\a\\a\\aaa\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a\\a"
      "\\a\\a\\a\\a\\a1111\\a;"
      "bbbbbbbbbbbbbbbbbbbb555\\aaa\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abaa\\ab\\ab\\ab\\ab\\ab\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\abaa\\ab\\ab\\ab\\ab\\ab\\abcccccccccc4"
      "4;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456aa\\ab\\ab\\ab\\ab\\ab\\abccccc"
      "ccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbaa\\ab\\ab\\ab\\ab\\ab"
      "\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbccaa\\ab\\ab\\ab\\ab\\a"
      "b\\abcccccccccc44;"
      "b\\ab\\ab\\ab\\abxx\\ab\\ab\\ab\\ab123456\\a\\abbcc\\abaa"};

  shcore::create_file(path, test_string, true);
  const size_t needle_size = line_terminator.size();

  {
    std::vector<Range> expected_ranges;
    expected_ranges.push_back(Range{0, 57});
    expected_ranges.push_back(Range{57, 118});
    expected_ranges.push_back(Range{118, 183});
    // expected_ranges.push_back(Range{183, 256}); // false negative
    // expected_ranges.push_back(Range{256, 322}); // false negative

    // expected_ranges.push_back(Range{322, 368});
    expected_ranges.push_back(
        Range{183, 368});  // match with previous end range
    expected_ranges.push_back(Range{368, 428});
    expected_ranges.push_back(Range{428, 494});
    expected_ranges.push_back(Range{494, 566});
    expected_ranges.push_back(Range{566, 640});
    expected_ranges.push_back(Range{640, 717});

    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, kSkipBytes, &r,
                       File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  // jump to row line terminator, but we don't know if it is escaped, therefore
  // we skip it
  {
    std::vector<Range> expected_ranges;
    expected_ranges.push_back(Range{0, 368});
    expected_ranges.push_back(Range{368, 640});
    expected_ranges.push_back(Range{640, 717});

    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<File_import_info> r;
    chunk_by_max_bytes(fh.begin(needle_size), fh.end(needle_size),
                       line_terminator, escape, 254, &r, File_import_info());

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.range.first);
      EXPECT_EQ(expected_ranges[i].end, range.range.second);
    }

    EXPECT_TRUE(r.empty());
  }

  {
    std::vector<Range> expected_ranges;
    expected_ranges.push_back(Range{0, 57});
    expected_ranges.push_back(Range{57, 118});
    expected_ranges.push_back(Range{118, 183});
    expected_ranges.push_back(Range{183, 256});
    expected_ranges.push_back(Range{256, 322});
    expected_ranges.push_back(Range{322, 368});
    expected_ranges.push_back(Range{368, 428});
    expected_ranges.push_back(Range{428, 494});
    expected_ranges.push_back(Range{494, 566});
    expected_ranges.push_back(Range{566, 640});
    expected_ranges.push_back(Range{640, 717});

    auto fh_ptr = mysqlshdk::storage::make_file(path);
    File_handler fh{fh_ptr.get()};
    std::queue<Range> r;
    auto row = fh.begin(needle_size);
    auto last = fh.end(needle_size);
    while (row != last) {
      auto previous = row;
      row = skip_rows(row, fh.end(needle_size), line_terminator, 1, escape);
      r.push(Range{previous.offset(), row.offset()});
    }

    EXPECT_EQ(expected_ranges.size(), r.size());

    for (size_t i = 0; i < expected_ranges.size(); i++) {
      auto range = r.front();
      r.pop();

      EXPECT_EQ(expected_ranges[i].begin, range.begin);
      EXPECT_EQ(expected_ranges[i].end, range.end);
    }

    EXPECT_TRUE(r.empty());
  }

  shcore::delete_file(path, true);
}

}  // namespace import_table
}  // namespace mysqlsh

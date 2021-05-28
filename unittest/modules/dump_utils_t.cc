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

#include <gtest/gtest_prod.h>
#include <cstdlib>
#include "modules/util/dump/dump_utils.h"
#include "unittest/gtest_clean.h"

#include "modules/util/load/dump_reader.h"

namespace mysqlsh {
namespace dump {

TEST(Dump_utils, encode_schema_basename) {
  EXPECT_EQ(".sql", get_schema_filename(encode_schema_basename("")));
  EXPECT_EQ("sakila.sql",
            get_schema_filename(encode_schema_basename("sakila")));
  EXPECT_EQ("sak%20ila.sql",
            get_schema_filename(encode_schema_basename("sak ila")));
  EXPECT_EQ("sakila.table.sql",
            get_schema_filename(encode_schema_basename("sakila.table")));
  EXPECT_EQ(
      "sakila.table%40real_table.sql",
      get_schema_filename(encode_schema_basename("sakila.table@real_table")));
  EXPECT_EQ("sákila.sql",
            get_schema_filename(encode_schema_basename("sákila")));
}

TEST(Dump_utils, encode_table_basename) {
  EXPECT_EQ("@.sql", get_table_filename(encode_table_basename("", "")));
  EXPECT_EQ("sakila@actor.sql",
            get_table_filename(encode_table_basename("sakila", "actor")));
  EXPECT_EQ("sak%20ila@acto%20.sql",
            get_table_filename(encode_table_basename("sak ila", "acto ")));
  EXPECT_EQ("sakila.table@bla.sql",
            get_table_filename(encode_table_basename("sakila.table", "bla")));
  EXPECT_EQ("sakila.table%40real_table@actual_real_table.sql",
            get_table_filename(encode_table_basename("sakila.table@real_table",
                                                     "actual_real_table")));
  EXPECT_EQ("sákila@tablê%40.sql",
            get_table_filename(encode_table_basename("sákila", "tablê@")));
}

TEST(Dump_utils, encode_table_data_filename) {
  EXPECT_EQ("@.csv",
            get_table_data_filename(encode_table_basename("", ""), "csv"));
  EXPECT_EQ(
      "sakila@actor.csv",
      get_table_data_filename(encode_table_basename("sakila", "actor"), "csv"));
  EXPECT_EQ("sak%20ila@acto%20.csv",
            get_table_data_filename(encode_table_basename("sak ila", "acto "),
                                    "csv"));
  EXPECT_EQ("sakila.table@bla.csv",
            get_table_data_filename(
                encode_table_basename("sakila.table", "bla"), "csv"));
  EXPECT_EQ(
      "sakila.table%40real_table@actual_real_table.csv",
      get_table_data_filename(
          encode_table_basename("sakila.table@real_table", "actual_real_table"),
          "csv"));
  EXPECT_EQ("sákila@tablê%40.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv"));
}

TEST(Dump_utils, encode_table_data_filename_chunks) {
  EXPECT_EQ("@@1.csv", get_table_data_filename(encode_table_basename("", ""),
                                               "csv", 1, false));
  EXPECT_EQ("sakila@actor@2.csv",
            get_table_data_filename(encode_table_basename("sakila", "actor"),
                                    "csv", 2, false));
  EXPECT_EQ("sakila@actor@@3.csv",
            get_table_data_filename(encode_table_basename("sakila", "actor"),
                                    "csv", 3, true));

  EXPECT_EQ("sak%20ila@acto%20@123.csv",
            get_table_data_filename(encode_table_basename("sak ila", "acto "),
                                    "csv", 123, false));
  EXPECT_EQ("sakila.table@bla@0.csv",
            get_table_data_filename(
                encode_table_basename("sakila.table", "bla"), "csv", 0, false));
  EXPECT_EQ(
      "sakila.table%40real_table@actual_real_table@42.csv",
      get_table_data_filename(
          encode_table_basename("sakila.table@real_table", "actual_real_table"),
          "csv", 42, false));
  EXPECT_EQ("sákila@tablê%40@4.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv", 4, false));

  EXPECT_EQ("sákila@tablê%40@@4.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv", 4, true));
}

}  // namespace dump

class Dump_scheduler : public ::testing::Test {
 public:
  Dump_reader::Table_info make_table(const std::string &name, size_t chunks,
                                     size_t min_chunk_size,
                                     size_t size_variation) {
    Dump_reader::Table_info info;

    info.schema = "myschema";
    info.table = name;
    info.basename = info.table;
    info.data_info.emplace_back();

    auto &di = info.data_info.back();

    // di.owner will be set by test_scheduling()

    di.basename = info.basename;
    di.extension = "csv";
    di.last_chunk_seen = true;

    di.chunked = chunks > 0;
    di.num_chunks = chunks > 0 ? chunks : 1;

    while (di.available_chunk_sizes.size() < di.num_chunks) {
      size_t size = min_chunk_size + rand() % size_variation;
      if (size > 0) di.available_chunk_sizes.push_back(size);
    }

    assert(di.has_data_available());

    return info;
  }

  template <typename SchedF>
  std::vector<std::string> test_scheduling(
      SchedF f, const std::vector<Dump_reader::Table_info> &tables,
      size_t nthreads) {
    std::vector<std::string> schedule_order;

    std::unordered_multimap<std::string, size_t> tables_being_loaded;
    std::unordered_set<Dump_reader::Table_data_info *> tables_with_data;

    auto copy = tables;
    for (auto &t : copy) {
      for (auto &di : t.data_info) {
        di.owner = &t;
        tables_with_data.insert(&di);
      }
    }

    auto schedule_one = [&](std::string *out_table, std::string *out_file,
                            size_t *out_size) {
      auto iter = f(tables_being_loaded, &tables_with_data);

      if (iter != tables_with_data.end()) {
        *out_table = partition_key((*iter)->owner->schema,
                                   (*iter)->owner->table, (*iter)->partition);

        size_t chunk_index;
        size_t chunks_total;

        if ((*iter)->chunked) {
          chunk_index = (*iter)->chunks_consumed;
          if ((*iter)->last_chunk_seen)
            chunks_total = (*iter)->num_chunks;
          else
            chunks_total = 0;
          (*iter)->chunks_consumed++;

          *out_file = dump::get_table_data_filename(
              (*iter)->basename, (*iter)->extension, chunk_index,
              chunk_index + 1 == chunks_total);

          *out_size = (*iter)->available_chunk_sizes[chunk_index];
        } else {
          chunk_index = 0;
          chunks_total = 0;
          (*iter)->chunks_consumed++;

          *out_size = (*iter)->available_chunk_sizes[0];

          *out_file = dump::get_table_data_filename((*iter)->basename,
                                                    (*iter)->extension);
        }
        if (!(*iter)->has_data_available()) tables_with_data.erase(iter);
        return true;
      }
      return false;
    };

    struct Thread {
      std::string table;
      std::string file;
      size_t count = 0;
      size_t count_left = 0;
    };

    std::vector<Thread> threads(nthreads);

    auto debug = [&]() {
      std::cout << "Threads:\n";
      size_t i = 0;
      for (const auto &t : threads) {
        std::cout << i++ << "\t" << t.table << "\t" << t.file << "\t"
                  << t.count_left << "\n";
      }

      std::cout << "\nTables with data:\n";
      for (auto t : tables_with_data) {
        std::cout << t->owner->schema << "." << t->owner->table << "\t"
                  << t->bytes_available() << "\n";
      }
      std::cout << "\n";
    };

    while (1) {
      std::string table;
      std::string file;
      size_t count;

      auto old_tables_with_data = tables_with_data;

      int n_busy_threads = 0;
      // schedule work until all threads busy
      for (auto &t : threads) {
        if (t.table.empty()) {
          if (schedule_one(&table, &file, &count)) {
            t.table = table;
            t.file = file;
            t.count = count;
            t.count_left = count;
            schedule_order.push_back(file);
            tables_being_loaded.emplace(table, count);
            n_busy_threads++;
          }
        } else {
          n_busy_threads++;
        }
      }
      if (n_busy_threads == 0 && tables_with_data.empty()) break;

      if (old_tables_with_data.size() < nthreads) {
        // if there are more threads than tables

        std::map<std::string, size_t> data_loaded;

        for (const auto &thd : threads) {
          data_loaded[thd.table] += thd.count;
        }
        // debug();
        // std::cout << "Proportion per table:\n";
        // check that all pending tables are scheduled
        for (auto tbl : old_tables_with_data) {
          auto table_key = partition_key(tbl->owner->schema, tbl->owner->table,
                                         tbl->partition);
          if (std::count_if(threads.begin(), threads.end(),
                            [&](const Thread &thd) {
                              return thd.table == table_key;
                            }) == 0)
            debug();
          EXPECT_GT(std::count_if(threads.begin(), threads.end(),
                                  [&](const Thread &thd) {
                                    return thd.table == table_key;
                                  }),
                    0);

          // print proportion being loaded
          // std::cout << table_key << "\t" << data_loaded[table_key] << " out
          // of "
          //           << tbl->bytes_available() << "\n";
        }
      } else {
        // if there are fewer threads than tables

        std::set<std::string> unique_tables_seen;

        // check that the same table doesn't appear twice
        for (const auto &t : threads) {
          if (!t.table.empty()) {
            EXPECT_EQ(unique_tables_seen.count(t.table), 0);
            if (unique_tables_seen.count(t.table) != 0) {
              debug();
              throw std::logic_error("bug");
            }
            unique_tables_seen.insert(t.table);
          }
        }
        // all threads must be busy
        EXPECT_EQ(unique_tables_seen.size(), nthreads);
      }

      // consume work from all threads until one of them is done
      bool done = false;
      while (!done) {
        for (auto &t : threads) {
          if (t.count_left > 0 && --t.count_left == 0) {
            tables_being_loaded.erase(t.table);
            t.table = "";
            done = true;
          }
        }
      }
    }

    EXPECT_TRUE(tables_being_loaded.empty());

    return schedule_order;
  }
};

TEST_F(Dump_scheduler, load_scheduler) {
  std::vector<Dump_reader::Table_info> tables;
  tables.push_back(make_table("mytable-1", 100, 20, 5));

  // 1 table
  // just 1 thread
  {
    SCOPED_TRACE("1-1");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 1);
  }

  // fewer threads than tables
  {
    SCOPED_TRACE("1-3");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 3);
  }

  // 2 tables
  tables.push_back(make_table("mytable-2", 200, 20, 5));

  // just 1 thread
  {
    SCOPED_TRACE("2-1");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 1);
  }

  // fewer threads than tables
  {
    SCOPED_TRACE("2-4");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 4);
  }

  // 5 tables
  tables.push_back(make_table("mytable-3", 10, 20, 5));
  tables.push_back(make_table("mytable-4", 5, 20, 5));
  tables.push_back(make_table("mytable-5", 1000, 20, 5));

  // just 1 thread
  {
    SCOPED_TRACE("1");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 1);
  }

  // fewer threads than tables
  {
    SCOPED_TRACE("3");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 3);
  }

  // same as tables
  {
    SCOPED_TRACE("5");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 5);
  }

  // more than tables
  {
    SCOPED_TRACE("16");
    test_scheduling(Dump_reader::schedule_chunk_proportionally, tables, 16);
  }
}
}  // namespace mysqlsh

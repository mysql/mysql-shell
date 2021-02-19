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

#include "modules/util/dump/instance_cache.h"

#include <set>
#include <string>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {
namespace tests {

namespace {

void verify(const Instance_cache &cache, const std::string &name,
            const Instance_cache::Schema &expected) {
  const auto it = cache.schemas.find(name);
  ASSERT_TRUE(cache.schemas.end() != it)
      << "cache does not contain schema `" << name << "`";

  const auto keys = [](const auto &m) {
    std::set<std::string> s;

    for (const auto &e : m) {
      s.emplace(e.first);
    }

    return s;
  };

  EXPECT_EQ(keys(expected.tables), keys(it->second.tables));
  EXPECT_EQ(keys(expected.views), keys(it->second.views));
}

template <typename C>
::testing::AssertionResult contains(const C &map, const std::string &name) {
  std::set<std::string> objects;

  for (const auto &e : map) {
    objects.emplace(e.first);
  }

  if (map.end() != map.find(name)) {
    return ::testing::AssertionSuccess()
           << "Object found: " << name
           << ", contents: " << shcore::str_join(objects, ", ");
  } else {
    return ::testing::AssertionFailure()
           << "Object not found: " << name
           << ", contents: " << shcore::str_join(objects, ", ");
  }
}

}  // namespace

class Instance_cache_test : public Shell_core_test_wrapper {
 protected:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    m_session = connect_session();

    cleanup();
  }

  void TearDown() override {
    cleanup();

    m_session->close();

    Shell_core_test_wrapper::TearDown();
  }

  static std::shared_ptr<mysqlshdk::db::ISession> connect_session() {
    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(shcore::get_connection_options(_mysql_uri));
    return session;
  }

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

 private:
  void cleanup() const {
    m_session->execute("DROP SCHEMA IF EXISTS first;");
    m_session->execute("DROP SCHEMA IF EXISTS second;");
    m_session->execute("DROP SCHEMA IF EXISTS third;");
    m_session->execute("DROP SCHEMA IF EXISTS First;");
    m_session->execute("DROP SCHEMA IF EXISTS Second;");
  }
};

TEST_F(Instance_cache_test, filter_schemas_and_tables) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute("CREATE TABLE first.two (id INT);");
    m_session->execute("CREATE VIEW first.three AS SELECT * FROM first.one;");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("CREATE TABLE second.one (id INT);");
    m_session->execute("CREATE TABLE second.two (id INT);");
    m_session->execute("CREATE VIEW second.three AS SELECT * FROM second.one;");
    m_session->execute("CREATE SCHEMA third;");
    m_session->execute("CREATE TABLE third.one (id INT);");
    m_session->execute("CREATE TABLE third.two (id INT);");
    m_session->execute("CREATE VIEW third.three AS SELECT * FROM third.one;");
  }

  {
    SCOPED_TRACE("all filters are empty");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude table from non-existing schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"fourth", {"four"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude non-existing table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"third", {"four"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude existing table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"third", {"two"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude existing, non-existing table and a table in non-existing "
        "schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {}, {},
                           {{"third", {"two", "four"}}, {"fourth", {"four"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude existing tables/views from the same schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {},
                                              {{"third", {"two", "three"}}})
                           .build();
    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude all tables/views from the same schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {},
                               {{"third", {"one", "two", "three"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude existing tables/views from different schemas");

    const auto cache =
        Instance_cache_builder(
            m_session, {}, {}, {},
            {{"first", {"one"}}, {"second", {"two"}}, {"third", {"three"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include table from non-existing schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"fourth", {"four"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include non-existing table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"third", {"four"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"third", {"two"}}}, {}, {})
            .build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing and non-existing tables");

    const auto cache =
        Instance_cache_builder(
            m_session, {}, {{"third", {"two", "four"}}, {"fourth", {"four"}}},
            {}, {})
            .build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing table and view from the same schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"third", {"two", "three"}}}, {}, {})
                           .build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing tables/views from different schemas");

    const auto cache =
        Instance_cache_builder(
            m_session, {},
            {{"first", {"one"}}, {"second", {"two"}}, {"third", {"three"}}}, {},
            {})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"two", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include and exclude the same existing table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"third", {"two"}}}, {},
                               {{"third", {"two"}}})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include and exclude the same existing table + some more");

    const auto cache =
        Instance_cache_builder(
            m_session, {}, {{"third", {"two"}}}, {},
            {{"second", {"two"}}, {"third", {"two"}}, {"fourth", {"four"}}})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include and exclude existing tables/views from different schemas");

    const auto cache =
        Instance_cache_builder(
            m_session, {},
            {{"first", {"one"}}, {"second", {"two"}}, {"third", {"three"}}}, {},
            {{"first", {"three"}}, {"second", {"two"}}, {"third", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema third;
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude non-existing schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"fourth"}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude existing schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"third"}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "third"));
  }

  {
    SCOPED_TRACE("exclude existing and non-existing schemas");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"third", "fourth"}, {})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "third"));
  }

  {
    SCOPED_TRACE("exclude multiple existing schemas");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"second", "third"}, {})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "second"));
    EXPECT_FALSE(contains(cache.schemas, "third"));
  }

  {
    SCOPED_TRACE(
        "exclude non-existing schema and a table in another non-existing "
        "schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"fourth"},
                                              {{"fifth", {"five"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude non-existing schema and a non-existing table");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"fourth"},
                                              {{"third", {"four"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude non-existing schema and an existing table");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"fourth"},
                                              {{"third", {"three"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude existing schema and an existing table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"third"},
                                              {{"third", {"three"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}, {"two", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "third"));
  }

  {
    SCOPED_TRACE(
        "exclude existing schema and an existing table in another schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"third"},
                                              {{"second", {"two"}}})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"three", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "third"));
  }

  {
    SCOPED_TRACE("exclude non-existing schema, include existing table");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"second", {"two"}}}, {"fourth"}, {})
                           .build();

    Instance_cache::Schema second;
    second.tables = {{"two", {}}};
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "exclude existing schema, include existing table in another schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"second", {"two"}}}, {"third"}, {})
                           .build();

    Instance_cache::Schema second;
    second.tables = {{"two", {}}};
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "exclude existing schema, include existing table in the same schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"second", {"two"}}}, {"second"}, {})
                           .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "second"));

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude an existing schema, an existing table, include tables");

    const auto cache =
        Instance_cache_builder(m_session, {},
                               {{"second", {"two"}}, {"third", {"three"}}},
                               {"second"}, {{"third", {"three"}}})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "second"));

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude an existing schema, an existing table, include more tables");

    const auto cache =
        Instance_cache_builder(
            m_session, {},
            {{"first", {"one"}}, {"second", {"two"}}, {"third", {"three"}}},
            {"second"}, {{"third", {"three"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include non-existing schema -> empty result set");

    EXPECT_THROW_LIKE(Instance_cache_builder(m_session, {"fourth"}, {}, {}, {}),
                      std::logic_error,
                      "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE("include existing schema");

    const auto cache =
        Instance_cache_builder(m_session, {"third"}, {}, {}, {}).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing and non-existing schemas");

    const auto cache =
        Instance_cache_builder(m_session, {"third", "fourth"}, {}, {}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include multiple existing schemas");

    const auto cache =
        Instance_cache_builder(m_session, {"first", "third"}, {}, {}, {})
            .build();

    EXPECT_EQ(2, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing schema, exclude non-existing one");

    const auto cache =
        Instance_cache_builder(m_session, {"third"}, {}, {"fourth"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include and exclude the same schema -> empty result set");

    EXPECT_THROW_LIKE(
        Instance_cache_builder(m_session, {"third"}, {}, {"third"}, {}),
        std::logic_error, "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE(
        "include and exclude the same schema + some more -> empty result set");

    EXPECT_THROW_LIKE(Instance_cache_builder(m_session, {"third"}, {},
                                             {"first", "third"}, {}),
                      std::logic_error,
                      "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE("exclude and include the same schema + some more");

    const auto cache =
        Instance_cache_builder(m_session, {"first", "third"}, {}, {"third"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"two", {}}};
    first.views = {{"three", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include non-existing schema, include existing table -> empty result "
        "set");

    EXPECT_THROW_LIKE(Instance_cache_builder(m_session, {"fourth"},
                                             {{"first", {"one"}}}, {}, {}),
                      std::logic_error,
                      "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE(
        "include existing schema, include existing table in another schema");

    const auto cache = Instance_cache_builder(m_session, {"third"},
                                              {{"first", {"one"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, include existing table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"first", {"one"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include existing schemas, include existing table in one of the "
        "schemas");

    const auto cache = Instance_cache_builder(m_session, {"first", "second"},
                                              {{"first", {"one"}}}, {}, {})
                           .build();

    EXPECT_EQ(2, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "include existing schemas, include existing table in one of the "
        "schemas, exclude the other schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first", "second"},
                               {{"first", {"one"}}}, {"second"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include existing schemas, include existing table in one of the "
        "schemas, exclude the same schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first", "second"},
                               {{"first", {"one"}}}, {"first"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema second;
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude table in non-existing schema");

    const auto cache = Instance_cache_builder(m_session, {"third"}, {}, {},
                                              {{"fourth", {"one"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude non-existing table in another "
        "schema");

    const auto cache = Instance_cache_builder(m_session, {"third"}, {}, {},
                                              {{"second", {"four"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude non-existing table in the same "
        "schema");

    const auto cache = Instance_cache_builder(m_session, {"third"}, {}, {},
                                              {{"third", {"four"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing schema, exclude table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {"third"}, {}, {},
                                              {{"third", {"two"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude table/view in the same schema");

    const auto cache = Instance_cache_builder(m_session, {"third"}, {}, {},
                                              {{"third", {"two", "three"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("a little bit of everything");

    const auto cache = Instance_cache_builder(
                           m_session, {"first", "third"},
                           {{"first", {"two", "three"}}, {"third", {"one"}}},
                           {"third"}, {{"first", {"two"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.views = {{"three", {}}};
    verify(cache, "first", first);
  }
}

TEST_F(Instance_cache_test, schema_collation) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first COLLATE utf8mb4_polish_ci;");
    m_session->execute("CREATE SCHEMA second COLLATE utf8mb4_bin;");
    m_session->execute("CREATE SCHEMA third COLLATE utf8mb4_unicode_ci;");
  }

  {
    SCOPED_TRACE("test collations");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    EXPECT_EQ("utf8mb4_polish_ci", cache.schemas.at("first").collation);
    EXPECT_EQ("utf8mb4_bin", cache.schemas.at("second").collation);
    EXPECT_EQ("utf8mb4_unicode_ci", cache.schemas.at("third").collation);
  }
}

TEST_F(Instance_cache_test, table_metadata) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute(
        "CREATE TABLE first.one (id INT) "
        "ENGINE = InnoDB "
        "PARTITION BY KEY (id)"
        ";");
    m_session->execute(
        "CREATE TABLE first.two (id INT) "
        "ENGINE = MyISAM "
        "COMMENT = 'qwerty\\'asdfg'"
        ";");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute(
        "CREATE TABLE second.three (id INT) "
        "ENGINE = BLACKHOLE "
        "COMMENT = 'important table'"
        ";");
  }

  {
    SCOPED_TRACE("test table metadata");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    {
      const auto &one = cache.schemas.at("first").tables.at("one");
      EXPECT_EQ("InnoDB", one.engine);
      EXPECT_EQ("", one.comment);
      EXPECT_EQ("partitioned", one.create_options);
    }

    {
      const auto &two = cache.schemas.at("first").tables.at("two");
      EXPECT_EQ("MyISAM", two.engine);
      EXPECT_EQ("qwerty'asdfg", two.comment);
      EXPECT_EQ("", two.create_options);
    }

    {
      const auto &three = cache.schemas.at("second").tables.at("three");
      EXPECT_EQ("BLACKHOLE", three.engine);
      EXPECT_EQ("important table", three.comment);
      EXPECT_EQ("", three.create_options);
    }
  }
}

TEST_F(Instance_cache_test, view_metadata) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("SET @@session.character_set_client = 'latin1';");
    m_session->execute(
        "SET @@session.collation_connection = 'latin1_spanish_ci';");
    m_session->execute("CREATE VIEW second.one AS SELECT * FROM first.one;");
    m_session->execute("SET @@session.character_set_client = 'utf8mb4';");
    m_session->execute(
        "SET @@session.collation_connection = 'utf8mb4_polish_ci';");
    m_session->execute("CREATE VIEW second.two AS SELECT * FROM first.one;");
    m_session->execute("CREATE SCHEMA third");
    m_session->execute("SET @@session.character_set_client = 'binary';");
    m_session->execute("SET @@session.collation_connection = 'binary';");
    m_session->execute("CREATE VIEW third.three AS SELECT * FROM first.one;");
  }

  {
    SCOPED_TRACE("test view metadata");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    {
      const auto &one = cache.schemas.at("second").views.at("one");
      EXPECT_EQ("latin1", one.character_set_client);
      EXPECT_EQ("latin1_spanish_ci", one.collation_connection);
    }

    {
      const auto &two = cache.schemas.at("second").views.at("two");
      EXPECT_EQ("utf8mb4", two.character_set_client);
      EXPECT_EQ("utf8mb4_polish_ci", two.collation_connection);
    }

    {
      const auto &three = cache.schemas.at("third").views.at("three");
      EXPECT_EQ("binary", three.character_set_client);
      EXPECT_EQ("binary", three.collation_connection);
    }
  }
}

TEST_F(Instance_cache_test, table_columns) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute(
        "CREATE TABLE first.one ("
        "c0 INT, "
        "gen INT GENERATED ALWAYS AS (c0 + 1) VIRTUAL, "
        "c1 CHAR(32) "
        ");");
    m_session->execute(
        "CREATE TABLE first.two ("
        "c0 TINYINT, "
        "c1 TINYINT UNSIGNED, "
        "c2 SMALLINT, "
        "c3 SMALLINT UNSIGNED, "
        "c4 MEDIUMINT, "
        "c5 MEDIUMINT UNSIGNED, "
        "c6 INT, "
        "c7 INT UNSIGNED, "
        "c8 INTEGER, "
        "c9 INTEGER UNSIGNED, "
        "c10 BIGINT, "
        "c11 BIGINT UNSIGNED, "
        "c12 DECIMAL(2,1), "
        "c13 DECIMAL(2,1) UNSIGNED, "
        "c14 NUMERIC(64,30), "
        "c15 NUMERIC(64,30) UNSIGNED, "
        "c16 REAL, "
        "c17 REAL UNSIGNED, "
        "c18 FLOAT, "
        "c19 FLOAT UNSIGNED, "
        "c20 DOUBLE, "
        "c21 DOUBLE UNSIGNED, "
        "c22 DATE, "
        "c23 TIME, "
        "c24 TIMESTAMP, "
        "c25 DATETIME, "
        "c26 YEAR, "
        "c27 TINYBLOB, "
        "c28 BLOB, "
        "c29 MEDIUMBLOB, "
        "c30 LONGBLOB, "
        "c31 TINYTEXT, "
        "c32 TEXT, "
        "c33 MEDIUMTEXT, "
        "c34 LONGTEXT, "
        "c35 TINYTEXT BINARY, "
        "c36 TEXT BINARY, "
        "c37 MEDIUMTEXT BINARY, "
        "c38 LONGTEXT BINARY, "
        "c39 CHAR(32), "
        "c40 VARCHAR(32), "
        "c41 BINARY(32), "
        "c42 VARBINARY(32), "
        "c43 BIT(64), "
        "c44 ENUM('v1','v2','v3'), "
        "c45 SET('v1','v2','v3'), "
        "c46 JSON, "
        "c47 GEOMETRY, "
        "c48 POINT, "
        "c49 LINESTRING, "
        "c50 POLYGON, "
        "c51 MULTIPOINT, "
        "c52 MULTILINESTRING, "
        "c53 MULTIPOLYGON, "
        "c54 GEOMETRYCOLLECTION"
        ");");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute(
        "CREATE TABLE second.three ("
        "c0 INT, "
        "gen INT GENERATED ALWAYS AS (c0 + 2) STORED, "
        "c1 TINYBLOB "
        ");");
  }

  {
    SCOPED_TRACE("test table columns");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    const auto validate = [&cache](const std::string &schema,
                                   const std::string &table,
                                   const std::vector<bool> &expected) {
      SCOPED_TRACE("testing table " + schema + "." + table);

      const auto &actual = cache.schemas.at(schema).tables.at(table).columns;
      const auto size = expected.size();

      ASSERT_EQ(size, actual.size());

      for (std::size_t i = 0; i < size; ++i) {
        const auto column = "c" + std::to_string(i);

        SCOPED_TRACE("checking column " + column);

        EXPECT_EQ(column, actual[i].name);
        EXPECT_EQ(expected[i], actual[i].csv_unsafe);
      }
    };

    validate("first", "one", {false, false});
    validate("first", "two",
             {
                 false,  // c0 TINYINT
                 false,  // c1 TINYINT UNSIGNED
                 false,  // c2 SMALLINT
                 false,  // c3 SMALLINT UNSIGNED
                 false,  // c4 MEDIUMINT
                 false,  // c5 MEDIUMINT UNSIGNED
                 false,  // c6 INT
                 false,  // c7 INT UNSIGNED
                 false,  // c8 INTEGER
                 false,  // c9 INTEGER UNSIGNED
                 false,  // c10 BIGINT
                 false,  // c11 BIGINT UNSIGNED
                 false,  // c12 DECIMAL(2,1)
                 false,  // c13 DECIMAL(2,1) UNSIGNED
                 false,  // c14 NUMERIC(64,30)
                 false,  // c15 NUMERIC(64,30) UNSIGNED
                 false,  // c16 REAL
                 false,  // c17 REAL UNSIGNED
                 false,  // c18 FLOAT
                 false,  // c19 FLOAT UNSIGNED
                 false,  // c20 DOUBLE
                 false,  // c21 DOUBLE UNSIGNED
                 false,  // c22 DATE
                 false,  // c23 TIME
                 false,  // c24 TIMESTAMP
                 false,  // c25 DATETIME
                 false,  // c26 YEAR
                 true,   // c27 TINYBLOB
                 true,   // c28 BLOB
                 true,   // c29 MEDIUMBLOB
                 true,   // c30 LONGBLOB
                 false,  // c31 TINYTEXT
                 false,  // c32 TEXT
                 false,  // c33 MEDIUMTEXT
                 false,  // c34 LONGTEXT
                 false,  // c35 TINYTEXT BINARY
                 false,  // c36 TEXT BINARY
                 false,  // c37 MEDIUMTEXT BINARY
                 false,  // c38 LONGTEXT BINARY
                 false,  // c39 CHAR(32)
                 false,  // c40 VARCHAR(32)
                 true,   // c41 BINARY(32)
                 true,   // c42 VARBINARY(32)
                 true,   // c43 BIT(64)
                 false,  // c44 ENUM('v1','v2','v3')
                 false,  // c45 SET('v1','v2','v3')
                 false,  // c46 JSON
                 true,   // c47 GEOMETRY
                 true,   // c48 POINT
                 true,   // c49 LINESTRING
                 true,   // c50 POLYGON
                 true,   // c51 MULTIPOINT
                 true,   // c52 MULTILINESTRING
                 true,   // c53 MULTIPOLYGON
                 true,   // c54 GEOMETRYCOLLECTION
             });
    validate("second", "three", {false, true});
  }
}

TEST_F(Instance_cache_test, table_indexes) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute(
        "CREATE TABLE first.one ("
        "id INT, data INT, hash INT, "
        "PRIMARY KEY (id, hash), "
        "UNIQUE INDEX (data, hash), "
        "INDEX (hash)"
        ");");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute(
        "CREATE TABLE second.two ("
        "id INT, data INT, hash INT, "
        "PRIMARY KEY (id, hash), "
        "UNIQUE INDEX (data, hash)"
        ");");
    m_session->execute(
        "CREATE TABLE second.three ("
        "id INT, data INT, hash INT, "
        "PRIMARY KEY (id, hash), "
        "INDEX (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE second.four ("
        "id INT, data INT, hash INT, "
        "UNIQUE INDEX (data, hash), "
        "INDEX (hash)"
        ");");
    m_session->execute("CREATE SCHEMA third;");
    m_session->execute(
        "CREATE TABLE third.five ("
        "id INT, data INT, hash INT, "
        "PRIMARY KEY (id, hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.six ("
        "id INT, data INT, hash INT, "
        "UNIQUE INDEX (data, hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.seven ("
        "id INT, data INT, hash INT, "
        "INDEX (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.eight ("
        "id INT, data INT, hash INT, "
        "UNIQUE INDEX b (data, hash), "
        "UNIQUE INDEX a (hash)"
        ");");
  }

  {
    SCOPED_TRACE("test table indexes");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    const auto validate = [&cache](const std::string &schema,
                                   const std::string &table,
                                   const Instance_cache::Index &expected) {
      SCOPED_TRACE("testing table " + schema + "." + table);

      const auto &actual = cache.schemas.at(schema).tables.at(table).index;
      const auto size = expected.columns.size();

      ASSERT_EQ(size, actual.columns.size());

      EXPECT_EQ(expected.primary, actual.primary);

      for (std::size_t i = 0; i < size; ++i) {
        EXPECT_EQ(expected.columns[i], actual.columns[i]);
      }
    };

    validate("first", "one", {{"id", "hash"}, true});
    validate("second", "two", {{"id", "hash"}, true});
    validate("second", "three", {{"id", "hash"}, true});
    validate("second", "four", {{"data", "hash"}, false});
    validate("third", "five", {{"id", "hash"}, true});
    validate("third", "six", {{"data", "hash"}, false});
    validate("third", "seven", {{}, false});
    validate("third", "eight", {{"hash"}, false});
  }
}

TEST_F(Instance_cache_test, table_histograms) {
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    SKIP_TEST("This test requires running against MySQL server version 8.0");
  }

  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT, data INT);");
    m_session->execute("ANALYZE TABLE first.one UPDATE HISTOGRAM ON id;");
    m_session->execute(
        "ANALYZE TABLE first.one UPDATE HISTOGRAM ON data "
        "WITH 50 buckets;");
    m_session->execute("CREATE TABLE first.two (id INT, data INT);");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("CREATE TABLE second.three (id INT, data INT);");
    m_session->execute(
        "ANALYZE TABLE second.three UPDATE HISTOGRAM ON data,id "
        "WITH 25 buckets;");
  }

  {
    SCOPED_TRACE("test table histograms");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    const auto validate =
        [&cache](const std::string &schema, const std::string &table,
                 const std::vector<Instance_cache::Histogram> &expected) {
          SCOPED_TRACE("testing table " + schema + "." + table);

          const auto &actual =
              cache.schemas.at(schema).tables.at(table).histograms;
          const auto size = expected.size();

          ASSERT_EQ(size, actual.size());

          for (std::size_t i = 0; i < size; ++i) {
            EXPECT_EQ(expected[i].column, actual[i].column);
            EXPECT_EQ(expected[i].buckets, actual[i].buckets);
          }
        };

    validate("first", "one", {{"data", 50}, {"id", 100}});
    validate("first", "two", {});
    validate("second", "three", {{"data", 25}, {"id", 25}});
  }
}

#if defined(_WIN32) || defined(__APPLE__)
TEST_F(Instance_cache_test, filter_schemas_and_tables_case_sensitive) {
  {
    // setup
    // on Windows: lower_case_table_names is 1, names are stored in lowercase,
    //             comparison is NOT case-sensitive
    // on Mac: lower_case_table_names is 2, names are stored as given,
    //         comparison is NOT case-sensitive. InnoDB table names and view
    //         names are stored in lowercase, as for lower_case_table_names=1.
    // 'first' and 'First' cannot be created at the same time, we're using
    // lowercase names to match what is actually created by the server
    // we're forcing case-sensitive comparison, lowercase names should match
    // what's in the DB, uppercase should not match anything
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute("CREATE VIEW first.two AS SELECT * FROM first.one;");

    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("CREATE TABLE second.one (id INT);");
    m_session->execute("CREATE VIEW second.two AS SELECT * FROM second.one;");
  }

  {
    SCOPED_TRACE("all filters are empty");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));
    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {}, {}, {}).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema - uppercase");

    EXPECT_THROW_LIKE(Instance_cache_builder(m_session, {"FIRST"}, {}, {}, {}),
                      std::logic_error,
                      "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE("include table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"FIRST", {"one"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"ONE"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include view");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"two"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include view - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"FIRST", {"two"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include view - view uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"TWO"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"first"}, {}).build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema - uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"FIRST"}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude table - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"FIRST", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude table - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"ONE"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude view");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"two"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude view - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"FIRST", {"two"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude view - view uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"TWO"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include schema, include table");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"first", {"one"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table - schema uppercase");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"FIRST", {"one"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table - table uppercase");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"first", {"ONE"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude schema");

    EXPECT_THROW_LIKE(
        Instance_cache_builder(m_session, {"first"}, {}, {"first"}, {}),
        std::logic_error, "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE("include schema, exclude schema - uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {}, {"FIRST"}, {}).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table");

    const auto cache = Instance_cache_builder(m_session, {"first"}, {}, {},
                                              {{"first", {"one"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table - schema uppercase");

    const auto cache = Instance_cache_builder(m_session, {"first"}, {}, {},
                                              {{"FIRST", {"one"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table - table uppercase");

    const auto cache = Instance_cache_builder(m_session, {"first"}, {}, {},
                                              {{"first", {"ONE"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude the same schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"first", {"one"}}}, {"first"}, {})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude schema - uppercase");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"first", {"one"}}}, {"FIRST"}, {})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude the same table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {},
                               {{"first", {"one"}}})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude table - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {},
                               {{"FIRST", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude table - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {},
                               {{"first", {"ONE"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema, exclude table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"first"},
                                              {{"first", {"one"}}})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema, exclude table - schema uppercase");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"first"},
                                              {{"FIRST", {"one"}}})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema, exclude table - table uppercase");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"first"},
                                              {{"first", {"ONE"}}})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    second.tables = {{"one", {}}};
    second.views = {{"two", {}}};
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include schema, include table, exclude schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}},
                               {"second"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude schema - uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}},
                               {"SECOND"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}}, {},
                               {{"first", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include schema, include table, exclude table - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}}, {},
                               {{"FIRST", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include schema, include table, exclude table - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}}, {},
                               {{"first", {"ONE"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude schema, exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {"second"},
                               {{"second", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    EXPECT_FALSE(contains(cache.schemas, "second"));

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE(
        "include table, exclude schema, exclude table - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {"second"},
                               {{"SECOND", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    EXPECT_FALSE(contains(cache.schemas, "second"));

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE(
        "include table, exclude schema, exclude table - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {"second"},
                               {{"second", {"ONE"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    EXPECT_FALSE(contains(cache.schemas, "second"));

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("all filters");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}},
                               {"second"}, {{"first", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("all filters - schema uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}},
                               {"second"}, {{"FIRST", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("all filters - table uppercase");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"one"}}},
                               {"second"}, {{"first", {"ONE"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }
}
#else
TEST_F(Instance_cache_test, filter_schemas_and_tables_case_sensitive) {
  {
    // setup
    // on Unix: lower_case_table_names is 0, names are stored as given,
    //          comparison is case-sensitive
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute("CREATE TABLE first.One (id INT);");
    m_session->execute("CREATE VIEW first.two AS SELECT * FROM first.one;");
    m_session->execute("CREATE VIEW first.Two AS SELECT * FROM first.One;");
    m_session->execute("CREATE SCHEMA First;");
    m_session->execute("CREATE TABLE First.one (id INT);");
    m_session->execute("CREATE TABLE First.One (id INT);");
    m_session->execute("CREATE VIEW First.two AS SELECT * FROM First.one;");
    m_session->execute("CREATE VIEW First.Two AS SELECT * FROM First.One;");
  }

  {
    SCOPED_TRACE("all filters are empty");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    First.tables = {{"one", {}}, {"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {}, {}, {}).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include view");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"Two"}}}, {}, {})
            .build();

    Instance_cache::Schema first;
    first.views = {{"Two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {"First"}, {}).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    First.tables = {{"one", {}}, {"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude view");

    const auto cache =
        Instance_cache_builder(m_session, {}, {}, {}, {{"first", {"Two"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    First.tables = {{"one", {}}, {"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include schema, include table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"first", {"One"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table in another schema");

    const auto cache = Instance_cache_builder(m_session, {"first"},
                                              {{"First", {"One"}}}, {}, {})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude another schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {}, {"First"}, {}).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude the same schema");

    EXPECT_THROW_LIKE(
        Instance_cache_builder(m_session, {"First"}, {}, {"First"}, {}),
        std::logic_error, "Filters for schemas result in an empty set.");
  }

  {
    SCOPED_TRACE("include schema, exclude table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {"first"}, {}, {},
                                              {{"first", {"One"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table in another schema");

    const auto cache = Instance_cache_builder(m_session, {"first"}, {}, {},
                                              {{"First", {"One"}}})
                           .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude the same schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"first", {"one"}}}, {"first"}, {})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include table, exclude another schema");

    const auto cache = Instance_cache_builder(
                           m_session, {}, {{"first", {"one"}}}, {"First"}, {})
                           .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("include table, exclude the same table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {},
                               {{"first", {"one"}}})
            .build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include table, exclude another table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"one"}}}, {},
                               {{"first", {"One"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema, exclude table in the same schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"first"},
                                              {{"first", {"one"}}})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    First.tables = {{"one", {}}, {"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema, exclude table in another schema");

    const auto cache = Instance_cache_builder(m_session, {}, {}, {"first"},
                                              {{"First", {"one"}}})
                           .build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    First.tables = {{"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude schema");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"One"}}},
                               {"First"}, {})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"One"}}}, {},
                               {{"first", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude schema, exclude table");

    const auto cache =
        Instance_cache_builder(m_session, {}, {{"first", {"One"}}}, {"First"},
                               {{"first", {"one"}}})
            .build();

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("all filters");

    const auto cache =
        Instance_cache_builder(m_session, {"first"}, {{"first", {"One"}}},
                               {"First"}, {{"first", {"one"}}})
            .build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }
}
#endif

TEST_F(Instance_cache_test, bug32540460) {
  // create some number of schemas, only one of them contains a table and a view
  // this is not guaranteed to reproduce the bug, as tables and views are held
  // in an unordered map
  const std::size_t schemas = 50;
  // select schema which is going to hold a table and a view, it should not be
  // the last one
  const std::string target_schema =
      "test_schema_" + std::to_string(rand() % (schemas - 1));
  // exclude all existing schemas, to make sure only newly created ones are in
  // the result
  Instance_cache_builder::Object_filters excluded_schemas;

  const auto cleanup = [this](std::size_t s) {
    for (std::size_t i = 0; i < s; ++i) {
      m_session->execute("DROP SCHEMA IF EXISTS test_schema_" +
                         std::to_string(i));
    }
  };

  cleanup(schemas);

  {
    // fetch existing schemas
    const auto result =
        m_session->query("SELECT SCHEMA_NAME FROM information_schema.schemata");

    while (const auto row = result->fetch_one()) {
      excluded_schemas.emplace(row->get_string(0));
    }

    // create new schemas
    for (std::size_t i = 0; i < schemas; ++i) {
      m_session->execute("CREATE SCHEMA test_schema_" + std::to_string(i));
    }

    // create table and view in one of the schemas
    m_session->execute("CREATE TABLE " + target_schema + ".one (id INT);");
    m_session->execute("CREATE VIEW " + target_schema +
                       ".two AS SELECT * FROM " + target_schema + ".one;");
  }

  {
    // create the cache, do not fetch metadata, this will return just list of
    // schemas, tables and views
    auto cache =
        Instance_cache_builder(m_session, {}, {}, excluded_schemas, {}, false)
            .build();
    // recreate the cache, use the existing one, fetch metadata
    auto builder = Instance_cache_builder(m_session, std::move(cache));
    cache = builder.build();

    EXPECT_EQ(schemas, cache.schemas.size());

    Instance_cache::Schema target;
    target.tables = {{"one", {}}};
    target.views = {{"two", {}}};
    verify(cache, target_schema, target);

    EXPECT_FALSE(
        cache.schemas.at(target_schema).tables.at("one").columns.empty());
    EXPECT_FALSE(cache.schemas.at(target_schema)
                     .views.at("two")
                     .collation_connection.empty());
  }

  cleanup(schemas);
}

}  // namespace tests
}  // namespace dump
}  // namespace mysqlsh

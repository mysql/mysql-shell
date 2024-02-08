/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/util/dump/instance_cache.h"

#include <array>
#include <set>
#include <string>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/indexes.h"

namespace mysqlsh {
namespace dump {
namespace tests {

namespace {

using common::Filtering_options;
using mysqlshdk::utils::Version;

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
    m_session->execute("DROP USER IF EXISTS first;");
    m_session->execute("DROP USER IF EXISTS second;");
    m_session->execute("DROP USER IF EXISTS third;");
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

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("fourth", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("third", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("third", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("third", std::array{"two", "four"});
    filters.tables().exclude("fourth", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("third", std::array{"two", "three"});
    const auto cache = Instance_cache_builder(m_session, filters).build();
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

    Filtering_options filters;
    filters.tables().exclude("third", std::array{"one", "two", "three"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "one");
    filters.tables().exclude("second", "two");
    filters.tables().exclude("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("fourth", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include non-existing table");

    Filtering_options filters;
    filters.tables().include("third", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing table");

    Filtering_options filters;
    filters.tables().include("third", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing and non-existing tables");

    Filtering_options filters;
    filters.tables().include("third", std::array{"two", "four"});
    filters.tables().include("fourth", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing table and view from the same schema");

    Filtering_options filters;
    filters.tables().include("third", std::array{"two", "three"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema third;
    third.tables = {{"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing tables/views from different schemas");

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().include("second", "two");
    filters.tables().include("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("third", "two");
    filters.tables().exclude("third", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema second;
    verify(cache, "second", second);

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include and exclude the same existing table + some more");

    Filtering_options filters;
    filters.tables().include("third", "two");
    filters.tables().exclude("second", "two");
    filters.tables().exclude("third", "two");
    filters.tables().exclude("fourth", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().include("second", "two");
    filters.tables().include("third", "three");
    filters.tables().exclude("first", "three");
    filters.tables().exclude("second", "two");
    filters.tables().exclude("third", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema third;
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("exclude non-existing schema");

    Filtering_options filters;
    filters.schemas().exclude("fourth");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("third");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude(std::array{"third", "fourth"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude(std::array{"second", "third"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("fourth");
    filters.tables().exclude("fifth", "five");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("fourth");
    filters.tables().exclude("third", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("fourth");
    filters.tables().exclude("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("third");
    filters.tables().exclude("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("third");
    filters.tables().exclude("second", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("fourth");
    filters.tables().include("second", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema second;
    second.tables = {{"two", {}}};
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "exclude existing schema, include existing table in another schema");

    Filtering_options filters;
    filters.schemas().exclude("third");
    filters.tables().include("second", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema second;
    second.tables = {{"two", {}}};
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "exclude existing schema, include existing table in the same schema");

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("second", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "second"));

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude an existing schema, an existing table, include tables");

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("second", "two");
    filters.tables().include("third", "three");
    filters.tables().exclude("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "second"));

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "exclude an existing schema, an existing table, include more tables");

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().include("second", "two");
    filters.tables().include("third", "three");
    filters.tables().exclude("third", "three");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include non-existing schema -> empty result set");

    Filtering_options filters;
    filters.schemas().include("fourth");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE("include existing schema");

    Filtering_options filters;
    filters.schemas().include("third");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing and non-existing schemas");

    Filtering_options filters;
    filters.schemas().include(std::array{"third", "fourth"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include multiple existing schemas");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "third"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("third");
    filters.schemas().exclude("fourth");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include and exclude the same schema -> empty result set");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.schemas().exclude("third");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE(
        "include and exclude the same schema + some more -> empty result set");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.schemas().exclude(std::array{"first", "third"});
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE("exclude and include the same schema + some more");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "third"});
    filters.schemas().exclude("third");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("fourth");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE(
        "include existing schema, include existing table in another schema");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, include existing table in the same schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include existing schemas, include existing table in one of the "
        "schemas");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second"});
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second"});
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include existing schemas, include existing table in one of the "
        "schemas, exclude the same schema");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second"});
    filters.schemas().exclude("first");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema second;
    verify(cache, "second", second);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude table in non-existing schema");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().exclude("fourth", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().exclude("second", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().exclude("third", "four");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}, {"two", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("include existing schema, exclude table in the same schema");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().exclude("third", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    third.views = {{"three", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE(
        "include existing schema, exclude table/view in the same schema");

    Filtering_options filters;
    filters.schemas().include("third");
    filters.tables().exclude("third", std::array{"two", "three"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema third;
    third.tables = {{"one", {}}};
    verify(cache, "third", third);
  }

  {
    SCOPED_TRACE("a little bit of everything");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "third"});
    filters.schemas().exclude("third");
    filters.tables().include("first", std::array{"two", "three"});
    filters.tables().include("third", "one");
    filters.tables().exclude("first", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).metadata({}).build();

    {
      const auto &one = cache.schemas.at("second").views.at("one");
      EXPECT_EQ("latin1", one.character_set_client);
      EXPECT_EQ("latin1_spanish_ci", one.collation_connection);
      ASSERT_EQ(1, one.all_columns.size());
      EXPECT_EQ("id", one.all_columns[0].name);
      EXPECT_EQ("`id`", one.all_columns[0].quoted_name);
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
        "c0 INT NOT NULL, "
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
        // explicitly set to NULL to avoid problems with
        // explicit_defaults_for_timestamp
        "c24 TIMESTAMP NULL, "
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
        "c1 TINYBLOB NULL"
        ");");

    if (_target_server_version >= Version(8, 0, 13)) {
      // EXTRA == DEFAULT_GENERATED
      m_session->execute(
          "CREATE TABLE second.four ("
          "c0 FLOAT DEFAULT (RAND() * RAND())"
          ");");
    }

    if (_target_server_version >= Version(8, 0, 23)) {
      // INVISIBLE + GENERATED
      m_session->execute(
          "CREATE TABLE second.five ("
          "c0 INT NOT NULL INVISIBLE, "
          "gen INT GENERATED ALWAYS AS (c0 + 1) VIRTUAL INVISIBLE, "
          "c1 CHAR(32)"
          ");");
      m_session->execute(
          "CREATE TABLE second.six ("
          "c0 INT INVISIBLE, "
          "gen INT GENERATED ALWAYS AS (c0 + 2) STORED INVISIBLE, "
          "c1 CHAR(32)"
          ");");
    }
  }

  {
    using mysqlshdk::db::Type;
    SCOPED_TRACE("test table columns");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).metadata({}).build();

    const auto validate = [this, &cache](const std::string &schema,
                                         const std::string &table,
                                         const std::vector<bool> &csv_unsafe,
                                         const std::vector<bool> &nullable) {
      SCOPED_TRACE("testing table " + schema + "." + table);

      const auto &table_info = cache.schemas.at(schema).tables.at(table);
      const auto &actual = table_info.all_columns;
      const auto size = csv_unsafe.size();
      const auto types = m_session->queryf(
          "SELECT " +
              shcore::str_join(actual, ",",
                               [](const auto &c) { return c.quoted_name; }) +
              " FROM !.! LIMIT 0;",
          schema, table);

      ASSERT_EQ(size, actual.size());
      ASSERT_EQ(size, nullable.size());
      ASSERT_EQ(size, types->get_metadata().size());

      for (std::size_t i = 0, idx = 0; i < size; ++i) {
        const auto column =
            actual[i].generated ? "gen" : "c" + std::to_string(idx);

        SCOPED_TRACE("checking column " + column);

        EXPECT_EQ(column, actual[i].name);
        EXPECT_EQ(shcore::quote_identifier(column), actual[i].quoted_name);
        EXPECT_EQ(csv_unsafe[i], actual[i].csv_unsafe);
        EXPECT_EQ(nullable[i], actual[i].nullable);
        EXPECT_EQ(types->get_metadata()[i].get_type(), actual[i].type);

        if (!actual[i].generated) {
          EXPECT_EQ(table_info.columns[idx++], &actual[i]);
        }
      }
    };

    validate("first", "one", {false, false, false}, {false, true, true});
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
             },
             {
                 true,  // c0 TINYINT
                 true,  // c1 TINYINT UNSIGNED
                 true,  // c2 SMALLINT
                 true,  // c3 SMALLINT UNSIGNED
                 true,  // c4 MEDIUMINT
                 true,  // c5 MEDIUMINT UNSIGNED
                 true,  // c6 INT
                 true,  // c7 INT UNSIGNED
                 true,  // c8 INTEGER
                 true,  // c9 INTEGER UNSIGNED
                 true,  // c10 BIGINT
                 true,  // c11 BIGINT UNSIGNED
                 true,  // c12 DECIMAL(2,1)
                 true,  // c13 DECIMAL(2,1) UNSIGNED
                 true,  // c14 NUMERIC(64,30)
                 true,  // c15 NUMERIC(64,30) UNSIGNED
                 true,  // c16 REAL
                 true,  // c17 REAL UNSIGNED
                 true,  // c18 FLOAT
                 true,  // c19 FLOAT UNSIGNED
                 true,  // c20 DOUBLE
                 true,  // c21 DOUBLE UNSIGNED
                 true,  // c22 DATE
                 true,  // c23 TIME
                 true,  // c24 TIMESTAMP
                 true,  // c25 DATETIME
                 true,  // c26 YEAR
                 true,  // c27 TINYBLOB
                 true,  // c28 BLOB
                 true,  // c29 MEDIUMBLOB
                 true,  // c30 LONGBLOB
                 true,  // c31 TINYTEXT
                 true,  // c32 TEXT
                 true,  // c33 MEDIUMTEXT
                 true,  // c34 LONGTEXT
                 true,  // c35 TINYTEXT BINARY
                 true,  // c36 TEXT BINARY
                 true,  // c37 MEDIUMTEXT BINARY
                 true,  // c38 LONGTEXT BINARY
                 true,  // c39 CHAR(32)
                 true,  // c40 VARCHAR(32)
                 true,  // c41 BINARY(32)
                 true,  // c42 VARBINARY(32)
                 true,  // c43 BIT(64)
                 true,  // c44 ENUM('v1','v2','v3')
                 true,  // c45 SET('v1','v2','v3')
                 true,  // c46 JSON
                 true,  // c47 GEOMETRY
                 true,  // c48 POINT
                 true,  // c49 LINESTRING
                 true,  // c50 POLYGON
                 true,  // c51 MULTIPOINT
                 true,  // c52 MULTILINESTRING
                 true,  // c53 MULTIPOLYGON
                 true,  // c54 GEOMETRYCOLLECTION
             });
    validate("second", "three", {false, false, true}, {true, true, true});

    if (_target_server_version >= Version(8, 0, 13)) {
      validate("second", "four", {false}, {true});
    }

    if (_target_server_version >= Version(8, 0, 23)) {
      validate("second", "five", {false, false, false}, {false, true, true});
      validate("second", "six", {false, false, false}, {true, true, true});
    }
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
    // shorter index is used
    m_session->execute(
        "CREATE TABLE third.eight ("
        "id INT, data INT, hash INT, "
        "UNIQUE INDEX b (data, hash), "
        "UNIQUE INDEX a (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.nine ("
        "id INT, data INT, hash INT, "
        "UNIQUE INDEX a (data, hash), "
        "UNIQUE INDEX b (hash)"
        ");");
    // shorter index is used even though it's not an integer
    m_session->execute(
        "CREATE TABLE third.ten ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX b (data, hash), "
        "UNIQUE INDEX a (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.eleven ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX a (data, hash), "
        "UNIQUE INDEX b (hash)"
        ");");
    // PK is used even though unique index is shorter
    m_session->execute(
        "CREATE TABLE third.twelve ("
        "id INT, data INT, hash INT, "
        "PRIMARY KEY (data, hash), "
        "UNIQUE INDEX a (id)"
        ");");
    // integer-based index is used
    m_session->execute(
        "CREATE TABLE third.thirteen ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX b (data), "
        "UNIQUE INDEX a (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.fourteen ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX a (data), "
        "UNIQUE INDEX b (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.fifteen ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX b (data, hash), "
        "UNIQUE INDEX a (id, data)"
        ");");
    m_session->execute(
        "CREATE TABLE third.sixteen ("
        "id INT, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX a (data, hash), "
        "UNIQUE INDEX b (id, data)"
        ");");
    // generated index
    m_session->execute(
        "CREATE TABLE third.seventeen ("
        "data INT, gen INT GENERATED ALWAYS AS (data + 2) STORED, "
        "PRIMARY KEY (gen)"
        ");");
    m_session->execute(
        "CREATE TABLE third.eighteen ("
        "data INT, gen INT GENERATED ALWAYS AS (data + 2) STORED, "
        "UNIQUE INDEX a (gen)"
        ");");
    // NOT NULL index is used
    m_session->execute(
        "CREATE TABLE third.nineteen ("
        "id INT NOT NULL, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX b (data), "
        "UNIQUE INDEX a (id)"
        ");");
    m_session->execute(
        "CREATE TABLE third.twenty ("
        "id INT NOT NULL, data INT, hash VARCHAR(32), "
        "UNIQUE INDEX a (data), "
        "UNIQUE INDEX b (id)"
        ");");
    m_session->execute(
        "CREATE TABLE third.`twenty-one` ("
        "id INT NOT NULL, data VARCHAR(32), hash VARCHAR(32) NOT NULL, "
        "UNIQUE INDEX b (data), "
        "UNIQUE INDEX a (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.`twenty-two` ("
        "id INT NOT NULL, data VARCHAR(32), hash VARCHAR(32) NOT NULL, "
        "UNIQUE INDEX a (data), "
        "UNIQUE INDEX b (hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.`twenty-three` ("
        "id INT NOT NULL, data VARCHAR(32), hash VARCHAR(32) NOT NULL, "
        "UNIQUE INDEX b (id, data), "
        "UNIQUE INDEX a (id, hash)"
        ");");
    m_session->execute(
        "CREATE TABLE third.`twenty-four` ("
        "id INT, data VARCHAR(32), hash VARCHAR(32) NOT NULL, "
        "UNIQUE INDEX a (id, data), "
        "UNIQUE INDEX b (id, hash)"
        ");");
  }

  {
    SCOPED_TRACE("test table indexes");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).metadata({}).build();

    const auto validate =
        [&cache](const std::string &schema, const std::string &table,
                 const std::vector<std::string> &expected_columns,
                 bool expected_pke) {
          SCOPED_TRACE("testing table " + schema + "." + table);

          const auto actual =
              select_index(cache.schemas.at(schema).tables.at(table));
          const auto size = expected_columns.size();

          EXPECT_EQ(expected_pke, actual.second);
          ASSERT_EQ(size, actual.first ? actual.first->columns().size() : 0);

          for (std::size_t i = 0; i < size; ++i) {
            EXPECT_EQ(expected_columns[i], actual.first->columns()[i]->name);
          }
        };

    validate("first", "one", {"id", "hash"}, true);
    validate("second", "two", {"id", "hash"}, true);
    validate("second", "three", {"id", "hash"}, true);
    validate("second", "four", {"data", "hash"}, false);
    validate("third", "five", {"id", "hash"}, true);
    validate("third", "six", {"data", "hash"}, false);
    validate("third", "seven", {}, false);
    validate("third", "eight", {"hash"}, false);
    validate("third", "nine", {"hash"}, false);
    validate("third", "ten", {"hash"}, false);
    validate("third", "eleven", {"hash"}, false);
    validate("third", "twelve", {"data", "hash"}, true);
    validate("third", "thirteen", {"data"}, false);
    validate("third", "fourteen", {"data"}, false);
    validate("third", "fifteen", {"id", "data"}, false);
    validate("third", "sixteen", {"id", "data"}, false);
    validate("third", "seventeen", {"gen"}, true);
    validate("third", "eighteen", {"gen"}, false);
    validate("third", "nineteen", {"id"}, true);
    validate("third", "twenty", {"id"}, true);
    validate("third", "twenty-one", {"hash"}, true);
    validate("third", "twenty-two", {"hash"}, true);
    validate("third", "twenty-three", {"id", "hash"}, true);
    validate("third", "twenty-four", {"id", "hash"}, false);
  }
}

TEST_F(Instance_cache_test, table_histograms) {
  if (_target_server_version < Version(8, 0, 0)) {
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

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).metadata({}).build();

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

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("first");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema - uppercase");

    Filtering_options filters;
    filters.schemas().include("FIRST");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE("include table");

    Filtering_options filters;
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table - table uppercase");

    Filtering_options filters;
    filters.tables().include("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include view");

    Filtering_options filters;
    filters.tables().include("first", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("FIRST", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include view - view uppercase");

    Filtering_options filters;
    filters.tables().include("first", "TWO");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("exclude schema");

    Filtering_options filters;
    filters.schemas().exclude("first");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("FIRST");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("FIRST", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "TWO");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table - schema uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table - table uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("first");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE("include schema, exclude schema - uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("FIRST");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table - schema uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table - table uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude the same schema");

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude schema - uppercase");

    Filtering_options filters;
    filters.schemas().exclude("FIRST");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    Instance_cache::Schema second;
    verify(cache, "second", second);

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("include table, exclude table - schema uppercase");

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude schema - uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("SECOND");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude table");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include schema, include table, exclude table - schema uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "one");
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE(
        "include schema, include table, exclude table - table uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude schema, exclude table");

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("second", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("SECOND", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("second", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "FIRST"));

    EXPECT_FALSE(contains(cache.schemas, "second"));

    EXPECT_FALSE(contains(cache.schemas, "SECOND"));
  }

  {
    SCOPED_TRACE("all filters");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("all filters - schema uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("FIRST", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("all filters - table uppercase");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("second");
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "ONE");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("first");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table");

    Filtering_options filters;
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include view");

    Filtering_options filters;
    filters.tables().include("first", "Two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.views = {{"Two", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema");

    Filtering_options filters;
    filters.schemas().exclude("First");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("exclude table");

    Filtering_options filters;
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.tables().exclude("first", "Two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table in another schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("First", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude another schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("First");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude the same schema");

    Filtering_options filters;
    filters.schemas().include("First");
    filters.schemas().exclude("First");
    const auto cache = Instance_cache_builder(m_session, filters).build();
    EXPECT_TRUE(cache.schemas.empty());
  }

  {
    SCOPED_TRACE("include schema, exclude table in the same schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().exclude("first", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, exclude table in another schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().exclude("First", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"one", {}}, {"One", {}}};
    first.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude the same schema");

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include table, exclude another schema");

    Filtering_options filters;
    filters.schemas().exclude("First");
    filters.tables().include("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("include table, exclude the same table");

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include table, exclude another table");

    Filtering_options filters;
    filters.tables().include("first", "one");
    filters.tables().exclude("first", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"one", {}}};
    verify(cache, "first", first);

    Instance_cache::Schema First;
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema, exclude table in the same schema");

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    First.tables = {{"one", {}}, {"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("exclude schema, exclude table in another schema");

    Filtering_options filters;
    filters.schemas().exclude("first");
    filters.tables().exclude("First", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_FALSE(contains(cache.schemas, "first"));

    Instance_cache::Schema First;
    First.tables = {{"One", {}}};
    First.views = {{"two", {}}, {"Two", {}}};
    verify(cache, "First", First);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude schema");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("First");
    filters.tables().include("first", "One");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include schema, include table, exclude table");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.tables().include("first", "One");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_EQ(1, cache.schemas.size());

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);
  }

  {
    SCOPED_TRACE("include table, exclude schema, exclude table");

    Filtering_options filters;
    filters.schemas().exclude("First");
    filters.tables().include("first", "One");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    Instance_cache::Schema first;
    first.tables = {{"One", {}}};
    verify(cache, "first", first);

    EXPECT_FALSE(contains(cache.schemas, "First"));
  }

  {
    SCOPED_TRACE("all filters");

    Filtering_options filters;
    filters.schemas().include("first");
    filters.schemas().exclude("First");
    filters.tables().include("first", "One");
    filters.tables().exclude("first", "one");
    const auto cache = Instance_cache_builder(m_session, filters).build();

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
  std::unordered_set<std::string> excluded_schemas;

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
    Filtering_options filters;
    filters.schemas().exclude(excluded_schemas);
    auto cache = Instance_cache_builder(m_session, filters).build();
    // recreate the cache, use the existing one, fetch metadata
    auto builder = Instance_cache_builder(m_session, filters, std::move(cache));
    cache = builder.metadata({}).build();

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

TEST_F(Instance_cache_test, filter_events) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute(
        "CREATE EVENT first.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT first.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute(
        "CREATE EVENT second.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT second.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute("CREATE SCHEMA third;");
    m_session->execute(
        "CREATE EVENT third.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT third.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
  }

  const auto EXPECT_EVENTS =
      [](const Instance_cache &cache, const std::string &schema,
         const std::unordered_set<std::string> &expected) {
        SCOPED_TRACE("schema: " + schema);

        const auto it = cache.schemas.find(schema);
        ASSERT_TRUE(cache.schemas.end() != it)
            << "cache does not contain schema `" << schema << "`";

        EXPECT_EQ(expected, it->second.events);
      };

  {
    SCOPED_TRACE("all filters are empty");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude event from non-existing schema");

    Filtering_options filters;
    filters.events().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude non-existing event");

    Filtering_options filters;
    filters.events().exclude("third", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude existing event");

    Filtering_options filters;
    filters.events().exclude("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {"one"});
  }

  {
    SCOPED_TRACE(
        "exclude existing, non-existing event and an event in non-existing "
        "schema");

    Filtering_options filters;
    filters.events().exclude("third", std::array{"two", "four"});
    filters.events().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {"one"});
  }

  {
    SCOPED_TRACE("exclude all events from the same schema");

    Filtering_options filters;
    filters.events().exclude("third", std::array{"one", "two", "three"});
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one", "two"});
    EXPECT_EVENTS(cache, "second", {"one", "two"});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("exclude existing events from different schemas");

    Filtering_options filters;
    filters.events().exclude("first", "one");
    filters.events().exclude("second", "two");
    filters.events().exclude("third", "one");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"two"});
    EXPECT_EVENTS(cache, "second", {"one"});
    EXPECT_EVENTS(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include event from non-existing schema");

    Filtering_options filters;
    filters.events().include("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("include non-existing event");

    Filtering_options filters;
    filters.events().include("third", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("include existing event");

    Filtering_options filters;
    filters.events().include("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include existing and non-existing events");

    Filtering_options filters;
    filters.events().include("third", std::array{"two", "four"});
    filters.events().include("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include existing events from the same schema");

    Filtering_options filters;
    filters.events().include("third", std::array{"one", "two"});
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("include existing events from different schemas");

    Filtering_options filters;
    filters.events().include("first", "one");
    filters.events().include("second", "two");
    filters.events().include("third", "three");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one"});
    EXPECT_EVENTS(cache, "second", {"two"});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing event");

    Filtering_options filters;
    filters.events().include("third", "two");
    filters.events().exclude("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing event + some more");

    Filtering_options filters;
    filters.events().include("third", "two");
    filters.events().exclude("second", "two");
    filters.events().exclude("third", "two");
    filters.events().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {});
  }

  {
    SCOPED_TRACE("include and exclude existing events from different schemas");

    Filtering_options filters;
    filters.events().include("first", "one");
    filters.events().include("second", "two");
    filters.events().include("third", "two");
    filters.events().exclude("first", "two");
    filters.events().exclude("second", "two");
    filters.events().exclude("third", "one");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    EXPECT_EVENTS(cache, "first", {"one"});
    EXPECT_EVENTS(cache, "second", {});
    EXPECT_EVENTS(cache, "third", {"two"});
  }
}

TEST_F(Instance_cache_test, filter_routines) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute(
        "CREATE FUNCTION first.one() RETURNS INT DETERMINISTIC RETURN 1;");
    m_session->execute("CREATE PROCEDURE first.two() DETERMINISTIC BEGIN END;");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute(
        "CREATE PROCEDURE second.one() DETERMINISTIC BEGIN END;");
    m_session->execute(
        "CREATE FUNCTION second.two() RETURNS INT DETERMINISTIC RETURN 1;");
    m_session->execute("CREATE SCHEMA third;");
    m_session->execute(
        "CREATE FUNCTION third.one() RETURNS INT DETERMINISTIC RETURN 1;");
    m_session->execute("CREATE PROCEDURE third.two() DETERMINISTIC BEGIN END;");
  }

  const auto EXPECT_ROUTINES =
      [](const Instance_cache &cache, const std::string &schema,
         const std::unordered_set<std::string> &expected) {
        SCOPED_TRACE("schema: " + schema);

        const auto it = cache.schemas.find(schema);
        ASSERT_TRUE(cache.schemas.end() != it)
            << "cache does not contain schema `" << schema << "`";

        std::unordered_set<std::string> routines = it->second.functions;
        routines.insert(it->second.procedures.begin(),
                        it->second.procedures.end());
        EXPECT_EQ(expected, routines);
      };

  {
    SCOPED_TRACE("all filters are empty");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude routine from non-existing schema");

    Filtering_options filters;
    filters.routines().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude non-existing routine");

    Filtering_options filters;
    filters.routines().exclude("third", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("exclude existing routine");

    Filtering_options filters;
    filters.routines().exclude("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {"one"});
  }

  {
    SCOPED_TRACE(
        "exclude existing, non-existing routine and a routine in non-existing "
        "schema");

    Filtering_options filters;
    filters.routines().exclude("third", std::array{"two", "four"});
    filters.routines().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {"one"});
  }

  {
    SCOPED_TRACE("exclude all routines from the same schema");

    Filtering_options filters;
    filters.routines().exclude("third", std::array{"one", "two", "three"});
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one", "two"});
    EXPECT_ROUTINES(cache, "second", {"one", "two"});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE("exclude existing routines from different schemas");

    Filtering_options filters;
    filters.routines().exclude("first", "one");
    filters.routines().exclude("second", "two");
    filters.routines().exclude("third", "one");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"two"});
    EXPECT_ROUTINES(cache, "second", {"one"});
    EXPECT_ROUTINES(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include routine from non-existing schema");

    Filtering_options filters;
    filters.routines().include("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE("include non-existing routine");

    Filtering_options filters;
    filters.routines().include("third", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE("include existing routine");

    Filtering_options filters;
    filters.routines().include("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include existing and non-existing routines");

    Filtering_options filters;
    filters.routines().include("third", std::array{"two", "four"});
    filters.routines().include("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {"two"});
  }

  {
    SCOPED_TRACE("include existing routines from the same schema");

    Filtering_options filters;
    filters.routines().include("third", std::array{"one", "two"});
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {"one", "two"});
  }

  {
    SCOPED_TRACE("include existing routines from different schemas");

    Filtering_options filters;
    filters.routines().include("first", "one");
    filters.routines().include("second", "two");
    filters.routines().include("third", "three");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one"});
    EXPECT_ROUTINES(cache, "second", {"two"});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing routine");

    Filtering_options filters;
    filters.routines().include("third", "two");
    filters.routines().exclude("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing routine + some more");

    Filtering_options filters;
    filters.routines().include("third", "two");
    filters.routines().exclude("second", "two");
    filters.routines().exclude("third", "two");
    filters.routines().exclude("fourth", "four");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {});
  }

  {
    SCOPED_TRACE(
        "include and exclude existing routines from different schemas");

    Filtering_options filters;
    filters.routines().include("first", "one");
    filters.routines().include("second", "two");
    filters.routines().include("third", "two");
    filters.routines().exclude("first", "two");
    filters.routines().exclude("second", "two");
    filters.routines().exclude("third", "one");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    EXPECT_ROUTINES(cache, "first", {"one"});
    EXPECT_ROUTINES(cache, "second", {});
    EXPECT_ROUTINES(cache, "third", {"two"});
  }
}

TEST_F(Instance_cache_test, filter_triggers) {
  {
    // setup
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute(
        "CREATE TRIGGER first.t1 AFTER DELETE ON first.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER first.t2 AFTER DELETE ON first.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute("CREATE TABLE first.two (id INT);");
    m_session->execute(
        "CREATE TRIGGER first.t3 AFTER DELETE ON first.two FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER first.t4 AFTER DELETE ON first.two FOR EACH ROW BEGIN "
        "END;");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("CREATE TABLE second.one (id INT);");
    m_session->execute(
        "CREATE TRIGGER second.t1 AFTER DELETE ON second.one FOR EACH ROW "
        "BEGIN END;");
    m_session->execute(
        "CREATE TRIGGER second.t2 AFTER DELETE ON second.one FOR EACH ROW "
        "BEGIN END;");
    m_session->execute("CREATE TABLE second.two (id INT);");
    m_session->execute(
        "CREATE TRIGGER second.t3 AFTER DELETE ON second.two FOR EACH ROW "
        "BEGIN END;");
    m_session->execute(
        "CREATE TRIGGER second.t4 AFTER DELETE ON second.two FOR EACH ROW "
        "BEGIN END;");
    m_session->execute("CREATE SCHEMA third;");
    m_session->execute("CREATE TABLE third.one (id INT);");
    m_session->execute(
        "CREATE TRIGGER third.t1 AFTER DELETE ON third.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER third.t2 AFTER DELETE ON third.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute("CREATE TABLE third.two (id INT);");
    m_session->execute(
        "CREATE TRIGGER third.t3 AFTER DELETE ON third.two FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER third.t4 AFTER DELETE ON third.two FOR EACH ROW BEGIN "
        "END;");
  }

  const auto EXPECT_TRIGGERS =
      [](const Instance_cache &cache, const std::string &schema,
         const std::string &table, const std::vector<std::string> &expected) {
        SCOPED_TRACE("schema: " + schema + ", table: " + table);

        const auto s = cache.schemas.find(schema);
        ASSERT_TRUE(cache.schemas.end() != s)
            << "cache does not contain schema `" << schema << "`";
        const auto t = s->second.tables.find(table);
        ASSERT_TRUE(s->second.tables.end() != t)
            << "schema does not contain table `" << table << "`";

        EXPECT_EQ(expected, t->second.triggers);
      };

  {
    SCOPED_TRACE("all filters are empty");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude trigger from non-existing schema");

    Filtering_options filters;
    filters.triggers().exclude("fourth", "four", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE(
        "exclude trigger from non-existing table (name does not match)");

    Filtering_options filters;
    filters.triggers().exclude("third", "three", "t9");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude trigger from non-existing table (name matches)");

    Filtering_options filters;
    filters.triggers().exclude("third", "three", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude non-existing trigger");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "t7");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude existing trigger");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude existing trigger from another table");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "t4");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE(
        "exclude existing, non-existing trigger and a trigger in non-existing "
        "schema");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "t1");
    filters.triggers().exclude("third", "one", "t4");
    filters.triggers().exclude("third", "one", "t7");
    filters.triggers().exclude("fourth", "four", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude all triggers from the same table");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "t1");
    filters.triggers().exclude("third", "one", "t2");
    filters.triggers().exclude("third", "one", "t3");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude all triggers from the same table (2)");

    Filtering_options filters;
    filters.triggers().exclude("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("exclude existing triggers from different schemas");

    Filtering_options filters;
    filters.triggers().exclude("first", "one", "t1");
    filters.triggers().exclude("first", "one", "t5");
    filters.triggers().exclude("second", "two", "t3");
    filters.triggers().exclude("second", "two", "t5");
    filters.triggers().exclude("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t2"});
    EXPECT_TRIGGERS(cache, "first", "two", {"t3", "t4"});
    EXPECT_TRIGGERS(cache, "second", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "second", "two", {"t4"});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {"t3", "t4"});
  }

  {
    SCOPED_TRACE("include trigger from non-existing schema");

    Filtering_options filters;
    filters.triggers().include("fourth", "four", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE(
        "include trigger from non-existing table (name does not match)");

    Filtering_options filters;
    filters.triggers().include("third", "three", "t9");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include trigger from non-existing table (name matches)");

    Filtering_options filters;
    filters.triggers().include("third", "three", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include non-existing trigger");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t7");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include existing trigger");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include existing trigger from another table");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t4");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include all triggers");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    filters.triggers().include("third", "one", "t2");
    filters.triggers().include("third", "one", "t3");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include all triggers (2)");

    Filtering_options filters;
    filters.triggers().include("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include existing and non-existing triggers");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    filters.triggers().include("third", "one", "t4");
    filters.triggers().include("third", "one", "t7");
    filters.triggers().include("fourth", "four", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include existing triggers from different schemas");

    Filtering_options filters;
    filters.triggers().include("first", "one", "t1");
    filters.triggers().include("second", "two", "t3");
    filters.triggers().include("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {"t1"});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {"t3"});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1", "t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing triggers");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    filters.triggers().exclude("third", "one", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include and exclude triggers from the same table");

    Filtering_options filters;
    filters.triggers().include("third", "one", "");
    filters.triggers().exclude("third", "one", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t2"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include and exclude triggers from the same table (2)");

    Filtering_options filters;
    filters.triggers().include("third", "one", "");
    filters.triggers().exclude("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include and exclude triggers from the same table (3)");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    filters.triggers().exclude("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE("include and exclude the same existing triggers + some more");

    Filtering_options filters;
    filters.triggers().include("third", "one", "t1");
    filters.triggers().exclude("first", "one", "t1");
    filters.triggers().exclude("third", "one", "t1");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }

  {
    SCOPED_TRACE(
        "include and exclude existing triggers from different schemas");

    Filtering_options filters;
    filters.triggers().include("first", "one", "t1");
    filters.triggers().include("second", "two", "t3");
    filters.triggers().include("third", "one", "");
    filters.triggers().exclude("first", "one", "");
    filters.triggers().exclude("second", "two", "t3");
    filters.triggers().exclude("third", "one", "t2");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    EXPECT_TRIGGERS(cache, "first", "one", {});
    EXPECT_TRIGGERS(cache, "first", "two", {});
    EXPECT_TRIGGERS(cache, "second", "one", {});
    EXPECT_TRIGGERS(cache, "second", "two", {});
    EXPECT_TRIGGERS(cache, "third", "one", {"t1"});
    EXPECT_TRIGGERS(cache, "third", "two", {});
  }
}

TEST_F(Instance_cache_test, stats) {
  {
    // setup
    // schemas
    m_session->execute("CREATE SCHEMA first;");
    m_session->execute("CREATE SCHEMA second;");
    m_session->execute("CREATE SCHEMA third;");

    // tables
    m_session->execute("CREATE TABLE first.one (id INT);");
    m_session->execute("CREATE TABLE first.two (id INT);");

    m_session->execute("CREATE TABLE second.one (id INT);");
    m_session->execute("CREATE TABLE second.two (id INT);");

    m_session->execute("CREATE TABLE third.one (id INT);");
    m_session->execute("CREATE TABLE third.two (id INT);");

    // views
    m_session->execute("CREATE VIEW first.three AS SELECT * FROM first.one;");
    m_session->execute("CREATE VIEW second.three AS SELECT * FROM second.one;");
    m_session->execute("CREATE VIEW third.three AS SELECT * FROM third.one;");

    // events
    m_session->execute(
        "CREATE EVENT first.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT first.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");

    m_session->execute(
        "CREATE EVENT second.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT second.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");

    m_session->execute(
        "CREATE EVENT third.one ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");
    m_session->execute(
        "CREATE EVENT third.two ON SCHEDULE EVERY 1 YEAR DO SELECT 1;");

    // routines
    m_session->execute(
        "CREATE FUNCTION first.one() RETURNS INT DETERMINISTIC RETURN 1;");
    m_session->execute("CREATE PROCEDURE first.two() DETERMINISTIC BEGIN END;");

    m_session->execute(
        "CREATE PROCEDURE second.one() DETERMINISTIC BEGIN END;");
    m_session->execute(
        "CREATE FUNCTION second.two() RETURNS INT DETERMINISTIC RETURN 1;");

    m_session->execute(
        "CREATE FUNCTION third.one() RETURNS INT DETERMINISTIC RETURN 1;");
    m_session->execute("CREATE PROCEDURE third.two() DETERMINISTIC BEGIN END;");

    // triggers
    m_session->execute(
        "CREATE TRIGGER first.t1 AFTER DELETE ON first.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER first.t2 AFTER DELETE ON first.one FOR EACH ROW BEGIN "
        "END;");

    m_session->execute(
        "CREATE TRIGGER first.t3 AFTER DELETE ON first.two FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER first.t4 AFTER DELETE ON first.two FOR EACH ROW BEGIN "
        "END;");

    m_session->execute(
        "CREATE TRIGGER second.t1 AFTER DELETE ON second.one FOR EACH ROW "
        "BEGIN END;");
    m_session->execute(
        "CREATE TRIGGER second.t2 AFTER DELETE ON second.one FOR EACH ROW "
        "BEGIN END;");

    m_session->execute(
        "CREATE TRIGGER second.t3 AFTER DELETE ON second.two FOR EACH ROW "
        "BEGIN END;");
    m_session->execute(
        "CREATE TRIGGER second.t4 AFTER DELETE ON second.two FOR EACH ROW "
        "BEGIN END;");

    m_session->execute(
        "CREATE TRIGGER third.t1 AFTER DELETE ON third.one FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER third.t2 AFTER DELETE ON third.one FOR EACH ROW BEGIN "
        "END;");

    m_session->execute(
        "CREATE TRIGGER third.t3 AFTER DELETE ON third.two FOR EACH ROW BEGIN "
        "END;");
    m_session->execute(
        "CREATE TRIGGER third.t4 AFTER DELETE ON third.two FOR EACH ROW BEGIN "
        "END;");

    // users
    m_session->execute("CREATE USER first;");
    m_session->execute("CREATE USER second;");
    m_session->execute("CREATE USER third;");
  }

  const auto total_count = [this](const std::string &table,
                                  const std::string &where = {},
                                  const std::string &column = "*") {
    auto sql = "SELECT COUNT(" + column + ") FROM information_schema." + table;

    if (!where.empty()) {
      sql += " WHERE " + where;
    }

    return m_session->query(sql)->fetch_one()->get_uint(0);
  };

  Instance_cache::Stats expected_total;

  expected_total.schemas = total_count("schemata");
  expected_total.tables = total_count("tables", "'BASE TABLE'=TABLE_TYPE");
  expected_total.views = total_count("tables", "'VIEW'=TABLE_TYPE");

  const auto total_users =
      total_count("user_privileges", {}, "DISTINCT grantee");

  const auto EXPECT_STATS = [](const Instance_cache::Stats &expected,
                               const Instance_cache::Stats &actual) {
    EXPECT_EQ(expected.schemas, actual.schemas);
    EXPECT_EQ(expected.tables, actual.tables);
    EXPECT_EQ(expected.views, actual.views);
    EXPECT_EQ(expected.events, actual.events);
    EXPECT_EQ(expected.routines, actual.routines);
    EXPECT_EQ(expected.triggers, actual.triggers);
    EXPECT_EQ(expected.users, actual.users);
  };

  {
    SCOPED_TRACE("no filters - schemas and tables");

    Filtering_options filters;
    const auto cache = Instance_cache_builder(m_session, filters).build();

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("no filters - schemas, tables and events");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    expected_total.events = total_count("events");
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("no filters - schemas, tables and routines");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    expected_total.events = 0;
    expected_total.routines = total_count("routines");
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("no filters - schemas, tables and triggers");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = total_count("triggers");
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("no filters - schemas, tables and users");

    Filtering_options filters;
    const auto cache =
        Instance_cache_builder(m_session, filters).users().build();

    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = total_users;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("filter users");

    Filtering_options filters;
    filters.users().include(
        std::array{"'first'@'%'", "'second'@'%'", "'third'@'%'"});
    const auto cache =
        Instance_cache_builder(m_session, filters).users().build();

    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = total_users;

    EXPECT_STATS(expected_total, cache.total);

    expected_total.users = 3;

    EXPECT_STATS(expected_total, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + all tables");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    const auto cache = Instance_cache_builder(m_session, filters).build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered tables");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.tables().include("first", std::array{"one", "three"});
    filters.tables().include("third", "two");
    const auto cache = Instance_cache_builder(m_session, filters).build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 2, 1}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + all events");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 6;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 6}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered events");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.events().include("first", "one");
    filters.events().include("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).events().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 6;
    expected_total.routines = 0;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 2}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + all routines");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 6;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 0, 6}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered routines");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.routines().include("first", "one");
    filters.routines().include("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).routines().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 6;
    expected_total.triggers = 0;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 0, 2}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + all triggers");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 12;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 0, 0, 12}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered triggers");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.triggers().include("first", "one", "t1");
    filters.triggers().include("second", "two", "t3");
    filters.triggers().include("third", "one", "");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 12;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 6, 3, 0, 0, 4}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered tables + all triggers");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.tables().include("first", std::array{"one", "three"});
    filters.tables().include("third", "two");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 4;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 2, 1, 0, 0, 4}, cache.filtered);
  }

  {
    SCOPED_TRACE("filter schemas + filtered tables + filtered triggers");

    Filtering_options filters;
    filters.schemas().include(std::array{"first", "second", "third"});
    filters.tables().include("first", std::array{"one", "three"});
    filters.tables().include("third", "two");
    filters.triggers().include("first", "one", "t1");
    filters.triggers().include("third", "two", "t3");
    const auto cache =
        Instance_cache_builder(m_session, filters).triggers().build();

    expected_total.tables = 6;
    expected_total.views = 3;
    expected_total.events = 0;
    expected_total.routines = 0;
    expected_total.triggers = 4;
    expected_total.users = 0;

    EXPECT_STATS(expected_total, cache.total);
    EXPECT_STATS({3, 2, 1, 0, 0, 2}, cache.filtered);
  }
}

}  // namespace tests
}  // namespace dump
}  // namespace mysqlsh

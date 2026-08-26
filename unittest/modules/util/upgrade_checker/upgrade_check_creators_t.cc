/*
 * Copyright (c) 2024, 2026, Oracle and/or its affiliates.
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
#include "unittest/gprod_clean.h"

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include "unittest/modules/util/upgrade_checker/test_utils.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session_pool.h"

#include "mysqlshdk/libs/db/filtering_options.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_check_creators, get_syntax_check_test) {
  auto server_info = upgrade_info(Version(8, 0, 0), Version(8, 0, 0));

  auto check = get_syntax_check(server_info);

  {
    // Verifies the original queries are created
    auto msession = std::make_shared<testing::Mock_session>();
    auto mock_pool = std::make_shared<testing::Mock_session_pool>();
    mysqlsh::upgrade_checker::Upgrade_check_options options;
    Checker_cache cache(options.filters);

    msession
        ->expect_query({"SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE, "
                        "ROUTINE_TYPE FROM information_schema.routines WHERE "
                        "(STRCMP(ROUTINE_SCHEMA COLLATE "
                        "utf8_bin,'mysql')&STRCMP(ROUTINE_SCHEMA COLLATE "
                        "utf8_bin,'sys')&STRCMP(ROUTINE_SCHEMA COLLATE "
                        "utf8_bin,'performance_schema')&STRCMP(ROUTINE_SCHEMA "
                        "COLLATE utf8_bin,'information_schema'))<>0",
                        [](const std::string &query) {
                          return remove_quoted_strings(query, k_sys_schemas);
                        }})
        .then({"ROUTINE_SCHEMA", "ROUTINE_NAME"});

    msession
        ->expect_query({"SELECT TRIGGER_SCHEMA, TRIGGER_NAME, SQL_MODE, "
                        "EVENT_OBJECT_TABLE FROM information_schema.triggers "
                        "WHERE (STRCMP(TRIGGER_SCHEMA COLLATE "
                        "utf8_bin,'mysql')&STRCMP(TRIGGER_SCHEMA COLLATE "
                        "utf8_bin,'sys')&STRCMP(TRIGGER_SCHEMA COLLATE "
                        "utf8_bin,'performance_schema')&STRCMP(TRIGGER_SCHEMA "
                        "COLLATE utf8_bin,'information_schema'))<>0",
                        [](const std::string &query) {
                          return remove_quoted_strings(query, k_sys_schemas);
                        }})
        .then({"TRIGGER_SCHEMA", "TRIGGER_NAME"});

    msession
        ->expect_query({"SELECT EVENT_SCHEMA, EVENT_NAME, SQL_MODE FROM "
                        "information_schema.events WHERE (STRCMP(EVENT_SCHEMA "
                        "COLLATE utf8_bin,'mysql')&STRCMP(EVENT_SCHEMA COLLATE "
                        "utf8_bin,'sys')&STRCMP(EVENT_SCHEMA COLLATE "
                        "utf8_bin,'performance_schema')&STRCMP(EVENT_SCHEMA "
                        "COLLATE utf8_bin,'information_schema'))<>0",
                        [](const std::string &query) {
                          return remove_quoted_strings(query, k_sys_schemas);
                        }})
        .then({"EVENT_SCHEMA", "EVENT_NAME"});

    mock_pool->setup_repeated_session(msession);

    EXPECT_NO_THROW(check->run({msession, server_info, mock_pool, &cache}));

    EXPECT_TRUE(msession->queries().empty());
  }

  {
    // Verifies queries using filtered objects
    auto mock_pool = std::make_shared<testing::Mock_session_pool>();
    auto msession = std::make_shared<testing::Mock_session>();
    mysqlshdk::db::Filtering_options options;
    options.schemas().include("sakila");
    options.schemas().exclude("exclude");
    options.routines().include("sakila.includedRoutine");
    options.routines().exclude("sakila.excludedRoutine");
    options.triggers().include("sakila.trigger_table.includedTrigger");
    options.triggers().exclude("sakila.trigger_table.excludedTrigger");
    options.events().include("sakila.includedEvent");
    options.events().exclude("sakila.excludedEvent");
    Checker_cache cache(options);

    msession
        ->expect_query(
            "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE, ROUTINE_TYPE FROM "
            "information_schema.routines WHERE "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila'))=0 AND "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'exclude'))<>0 AND "
            "((STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila')=0 AND "
            "(ROUTINE_NAME IN('includedRoutine')))) AND NOT "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila')=0 AND "
            "(ROUTINE_NAME IN('excludedRoutine')))")
        .then({"ROUTINE_SCHEMA", "ROUTINE_NAME"});

    msession
        ->expect_query(
            "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, SQL_MODE, EVENT_OBJECT_TABLE "
            "FROM information_schema.triggers WHERE (STRCMP(TRIGGER_SCHEMA "
            "COLLATE utf8_bin,'sakila'))=0 AND (STRCMP(TRIGGER_SCHEMA COLLATE "
            "utf8_bin,'exclude'))<>0 AND ((STRCMP(TRIGGER_SCHEMA COLLATE "
            "utf8_bin,'sakila')=0 AND STRCMP(EVENT_OBJECT_TABLE COLLATE "
            "utf8_bin,'trigger_table')=0 AND(STRCMP(TRIGGER_NAME COLLATE "
            "utf8_bin,'includedTrigger'))=0)) AND NOT((STRCMP(TRIGGER_SCHEMA "
            "COLLATE utf8_bin,'sakila')=0 AND STRCMP(EVENT_OBJECT_TABLE "
            "COLLATE utf8_bin,'trigger_table')=0 AND(STRCMP(TRIGGER_NAME "
            "COLLATE utf8_bin,'excludedTrigger'))=0))")
        .then({"TRIGGER_SCHEMA", "TRIGGER_NAME"});

    msession
        ->expect_query(
            "SELECT EVENT_SCHEMA, EVENT_NAME, SQL_MODE FROM "
            "information_schema.events "
            "WHERE (STRCMP(EVENT_SCHEMA COLLATE utf8_bin,'sakila'))=0 AND "
            "(STRCMP(EVENT_SCHEMA COLLATE utf8_bin,'exclude'))<>0 AND "
            "((STRCMP(EVENT_SCHEMA COLLATE utf8_bin,'sakila')=0 AND "
            "(EVENT_NAME IN('includedEvent')))) AND NOT (STRCMP(EVENT_SCHEMA "
            "COLLATE utf8_bin,'sakila')=0 AND (EVENT_NAME "
            "IN('excludedEvent')))")
        .then({"EVENT_SCHEMA", "EVENT_NAME"});

    mock_pool->setup_repeated_session(msession);

    EXPECT_NO_THROW(check->run(
        {msession, server_info,
         std::dynamic_pointer_cast<mysqlshdk::db::Session_pool>(mock_pool),
         &cache}));

    EXPECT_TRUE(msession->queries().empty());
  }
}

TEST(Upgrade_check_creators,
     foreign_key_references_check_uses_schema_qualified_target_names) {
  auto check = get_foreign_key_references_check();
  auto sql_check = dynamic_cast<Sql_upgrade_check *>(check.get());
  ASSERT_NE(nullptr, sql_check);

  const auto &queries = sql_check->get_queries();
  ASSERT_EQ(2, queries.size());

  EXPECT_NE(std::string::npos,
            queries[0].first.find("CONCAT(kc.REFERENCED_TABLE_SCHEMA,'.',"
                                  "rc.REFERENCED_TABLE_NAME) as "
                                  "target_table"));

  EXPECT_NE(std::string::npos,
            queries[1].first.find("CONCAT(fk.referenced_table_schema,'.',"
                                  "fk.referenced_table_name) AS "
                                  "target_table"));
  EXPECT_EQ(std::string::npos,
            queries[1].first.find("fk.referenced_table_name AS target_table"));
}

TEST(Upgrade_check_creators,
     foreign_key_references_check_uses_referenced_schema_for_non_unique_keys) {
  auto check = get_foreign_key_references_check();
  auto sql_check = dynamic_cast<Sql_upgrade_check *>(check.get());
  ASSERT_NE(nullptr, sql_check);

  const auto &queries = sql_check->get_queries();
  ASSERT_EQ(2, queries.size());

  EXPECT_NE(std::string::npos,
            queries[0].first.find("rc.UNIQUE_CONSTRAINT_SCHEMA = "
                                  "kc.REFERENCED_TABLE_SCHEMA"));
  EXPECT_EQ(std::string::npos,
            queries[0].first.find("rc.constraint_schema = "
                                  "kc.REFERENCED_TABLE_SCHEMA"));
}

TEST(Upgrade_check_creators,
     foreign_key_references_check_does_not_user_filter_referenced_indexes) {
  auto check = get_foreign_key_references_check();
  auto sql_check = dynamic_cast<Sql_upgrade_check *>(check.get());
  ASSERT_NE(nullptr, sql_check);

  const auto &queries = sql_check->get_queries();
  ASSERT_EQ(2, queries.size());

  for (const auto &query : queries) {
    EXPECT_EQ(std::string::npos,
              query.first.find("<<schema_filter:table_schema>>"));
    EXPECT_NE(std::string::npos,
              query.first.find(
                  "<<schema_filter_without_user_filters:table_schema>>"));
  }
}

TEST(Upgrade_check_creators, get_reserved_keywords_check_test) {
  // Every per-object-type query produced by the reserved keywords check embeds
  // the same keyword IN-list, so inspecting the first query is enough to verify
  // which keywords are included for a given source -> target version pair.
  const auto keyword_list = [](const Version &server, const Version &target) {
    const auto info = upgrade_info(server, target);
    const auto check = get_reserved_keywords_check(info);
    return check->get_queries().front().first;
  };
  const auto has = [](const std::string &query, const char *keyword) {
    return query.find(keyword) != std::string::npos;
  };

  // 8.0 -> 8.4.8: QUALIFY/TABLESAMPLE plus MANUAL/PARALLEL, which are still
  // reserved below 8.4.11. LIBRARY/EXTERNAL do not apply yet (target < 9.x).
  {
    const auto query = keyword_list(Version(8, 0, 0), Version(8, 4, 8));
    EXPECT_TRUE(has(query, "'QUALIFY'"));
    EXPECT_TRUE(has(query, "'TABLESAMPLE'"));
    EXPECT_TRUE(has(query, "'MANUAL'"));
    EXPECT_TRUE(has(query, "'PARALLEL'"));
    EXPECT_FALSE(has(query, "'LIBRARY'"));
    EXPECT_FALSE(has(query, "'EXTERNAL'"));
  }

  // 8.0 -> 8.4.11: MANUAL/PARALLEL became nonreserved in 8.4.11 and must drop
  // out (upper bound of the ranged add_keywords call); QUALIFY/TABLESAMPLE stay.
  {
    const auto query = keyword_list(Version(8, 0, 0), Version(8, 4, 11));
    EXPECT_TRUE(has(query, "'QUALIFY'"));
    EXPECT_TRUE(has(query, "'TABLESAMPLE'"));
    EXPECT_FALSE(has(query, "'MANUAL'"));
    EXPECT_FALSE(has(query, "'PARALLEL'"));
  }

  // 8.4.0 -> 9.2.0: LIBRARY (reserved since 9.2.0) applies; EXTERNAL (9.4.0)
  // does not. The 8.4.0 words are not re-reported since the source is 8.4.0.
  {
    const auto query = keyword_list(Version(8, 4, 0), Version(9, 2, 0));
    EXPECT_TRUE(has(query, "'LIBRARY'"));
    EXPECT_FALSE(has(query, "'EXTERNAL'"));
    EXPECT_FALSE(has(query, "'QUALIFY'"));
    EXPECT_FALSE(has(query, "'MANUAL'"));
  }

  // 8.4.0 -> 9.4.0: both LIBRARY (9.2.0) and EXTERNAL (9.4.0) apply.
  {
    const auto query = keyword_list(Version(8, 4, 0), Version(9, 4, 0));
    EXPECT_TRUE(has(query, "'LIBRARY'"));
    EXPECT_TRUE(has(query, "'EXTERNAL'"));
  }

  // 9.2.0 -> 9.4.0: LIBRARY is already reserved on the source, so only EXTERNAL
  // is newly reserved on the target.
  {
    const auto query = keyword_list(Version(9, 2, 0), Version(9, 4, 0));
    EXPECT_TRUE(has(query, "'EXTERNAL'"));
    EXPECT_FALSE(has(query, "'LIBRARY'"));
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh

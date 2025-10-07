/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/filtering_options.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_check_creators, get_syntax_check_test) {
  auto server_info = upgrade_info(Version(8, 0, 0), Version(8, 0, 0));

  auto check = get_syntax_check(server_info);

  {
    // Verifies the original queries are created
    auto msession = std::make_shared<testing::Mock_session>();
    Checker_cache cache;

    msession
        ->expect_query(
            {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE FROM "
             "information_schema.routines WHERE ROUTINE_TYPE = 'PROCEDURE' AND "
             "(ROUTINE_SCHEMA NOT "
             "IN('mysql','sys','performance_schema','information_schema'))",
             [](const std::string &query) {
               return remove_quoted_strings(query, k_sys_schemas);
             }})
        .then({"ROUTINE_SCHEMA", "ROUTINE_NAME"});

    msession
        ->expect_query(
            {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE FROM "
             "information_schema.routines WHERE ROUTINE_TYPE = 'FUNCTION' AND "
             "(ROUTINE_SCHEMA NOT "
             "IN('mysql','sys','performance_schema','information_schema'))",
             [](const std::string &query) {
               return remove_quoted_strings(query, k_sys_schemas);
             }})

        .then({"ROUTINE_SCHEMA", "ROUTINE_NAME"});

    msession
        ->expect_query(
            {"SELECT TRIGGER_SCHEMA, TRIGGER_NAME, SQL_MODE, "
             "EVENT_OBJECT_TABLE FROM information_schema.triggers WHERE "
             "(TRIGGER_SCHEMA NOT "
             "IN('mysql','sys','performance_schema','information_schema'))",
             [](const std::string &query) {
               return remove_quoted_strings(query, k_sys_schemas);
             }})
        .then({"TRIGGER_SCHEMA", "TRIGGER_NAME"});

    msession
        ->expect_query(
            {"SELECT EVENT_SCHEMA, EVENT_NAME, SQL_MODE FROM "
             "information_schema.events WHERE (EVENT_SCHEMA NOT "
             "IN('mysql','sys','performance_schema','information_schema'))",
             [](const std::string &query) {
               return remove_quoted_strings(query, k_sys_schemas);
             }})
        .then({"EVENT_SCHEMA", "EVENT_NAME"});

    EXPECT_NO_THROW(check->run(msession, server_info, &cache));

    EXPECT_TRUE(msession->queries().empty());
  }

  {
    // Verifies queries using filtered objects
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
    Checker_cache cache(&options);

    msession
        ->expect_query(
            "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE FROM "
            "information_schema.routines WHERE ROUTINE_TYPE = 'PROCEDURE' AND "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila'))=0 AND "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'exclude'))<>0 AND "
            "((STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila')=0 AND "
            "(ROUTINE_NAME IN('includedRoutine')))) AND NOT "
            "(STRCMP(ROUTINE_SCHEMA COLLATE utf8_bin,'sakila')=0 AND "
            "(ROUTINE_NAME IN('excludedRoutine')))")
        .then({"ROUTINE_SCHEMA", "ROUTINE_NAME"});

    msession
        ->expect_query(
            "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, SQL_MODE FROM "
            "information_schema.routines WHERE ROUTINE_TYPE = 'FUNCTION' AND "
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

    EXPECT_NO_THROW(check->run(msession, server_info, &cache));

    EXPECT_TRUE(msession->queries().empty());
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh

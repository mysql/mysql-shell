/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_INCLUDE_GTEST_H_
#define UNITTEST_INCLUDE_GTEST_H_

// Include and avoid warnings from v8
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>
#include "utils/utils_string.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern std::vector<std::pair<std::string, std::string>> g_skipped_tests;
extern std::vector<std::pair<std::string, std::string>> g_skipped_chunks;
extern std::vector<std::pair<std::string, std::string>> g_skipped_validations;
extern std::vector<std::pair<std::string, std::string>> g_pending_fixes;

#define SKIP_TEST(note)                                                      \
  {                                                                          \
    const ::testing::TestInfo *const test_info =                             \
        ::testing::UnitTest::GetInstance()->current_test_info();             \
    g_skipped_tests.push_back(                                               \
        {std::string(test_info->test_case_name()) + "." + test_info->name(), \
         note});                                                             \
    return;                                                                  \
  }

#define SKIP_CHUNK(note)                                                     \
  {                                                                          \
    const ::testing::TestInfo *const test_info =                             \
        ::testing::UnitTest::GetInstance()->current_test_info();             \
    g_skipped_chunks.push_back(                                              \
        {std::string(test_info->test_case_name()) + "." + test_info->name(), \
         note});                                                             \
  }

#define SKIP_VALIDATION(note)                                                \
  {                                                                          \
    const ::testing::TestInfo *const test_info =                             \
        ::testing::UnitTest::GetInstance()->current_test_info();             \
    g_skipped_validations.push_back(                                         \
        {std::string(test_info->test_case_name()) + "." + test_info->name(), \
         note});                                                             \
  }

#define PENDING_BUG_TEST(note)                                           \
  do {                                                                   \
    g_pending_fixes.push_back({__FILE__ ":" STRINGIFY(__LINE__), note}); \
  } while (0)

#endif  // UNITTEST_INCLUDE_GTEST_H_

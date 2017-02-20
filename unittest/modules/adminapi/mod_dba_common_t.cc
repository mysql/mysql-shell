/*
* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/
#include <string>
#include <gtest/gtest.h>
#include "modules/adminapi/mod_dba_common.h"

TEST(mod_dba_common, validate_label) {
  std::string t{};

  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (alpha)
    t = "Valid1"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (_)
    t = "_Valid_"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, contains valid synbols
    t = "Valid_3"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, contains valid synbols (:.-)
    t = "Valid:.-4"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (numeric)
    t = "2_Valid"; mysqlsh::dba::validate_label(t.c_str()));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty label
                   mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "not_allowed?"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "(not*valid)"; mysqlsh::dba::validate_label("(not_valid)"));
  EXPECT_ANY_THROW(
      // Invalid too long label (over 256 characteres)
      t = "over256chars_"
          "12345678901234567890123456789901234567890123456789012345678901234567"
          "89012345678901234567890123456789012345678901234567890123456789012345"
          "67890123456789012345678901234567890123456789012345678901234567890123"
          "4567890123456789012345678901234567890123";
      mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, begins with invalid synbol
      t = "#not_allowed"; mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "_not-allowed?"; mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "(*)%?"; mysqlsh::dba::validate_label(t.c_str()););
}

TEST(mod_dba_common, is_valid_identifier) {
  std::string t{};

  EXPECT_NO_THROW(
      // Valid identifier, begins with valid synbols (alpha)
      t = "Valid1"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_NO_THROW(
      // Valid identifier, begins with valid synbols (_)
      t = "_Valid_"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_NO_THROW(
      // Valid identifier, contains valid synbols
      t = "Valid_3"; mysqlsh::dba::validate_cluster_name(t));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty identifier
                   mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid too long identifier (over 40 characteres)
      t = "over40chars_12345678901234567890123456789";
      mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid synbol
      t = "#not_allowed"; mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "not_allowed?"; mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
    // Invalid identifier, begins with invalid synbols (numeric)
    t = "2_not_Valid"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "(*)%?"; mysqlsh::dba::validate_cluster_name(t););
}

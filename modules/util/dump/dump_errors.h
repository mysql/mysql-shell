/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DUMP_ERRORS_H_
#define MODULES_UTIL_DUMP_DUMP_ERRORS_H_

#include "modules/util/dump/common_errors.h"

#define SHERR_DUMP_MIN 52000

#define SHERR_DUMP_FIRST SHERR_DUMP_MIN

#define SHERR_DUMP_LOCK_TABLES_MISSING_PRIVILEGES 52000
#define SHERR_DUMP_LOCK_TABLES_MISSING_PRIVILEGES_MSG \
  "User %s is missing the following privilege(s) for %s: %s."

#define SHERR_DUMP_GLOBAL_READ_LOCK_FAILED 52001
#define SHERR_DUMP_GLOBAL_READ_LOCK_FAILED_MSG \
  "Unable to acquire global read lock"

#define SHERR_DUMP_LOCK_TABLES_FAILED 52002
#define SHERR_DUMP_LOCK_TABLES_FAILED_MSG "Unable to lock tables: %s."

#define SHERR_DUMP_CONSISTENCY_CHECK_FAILED 52003
#define SHERR_DUMP_CONSISTENCY_CHECK_FAILED_MSG "Consistency check has failed"

#define SHERR_DUMP_COMPATIBILITY_ISSUES_FOUND 52004
#define SHERR_DUMP_COMPATIBILITY_ISSUES_FOUND_MSG \
  "Compatibility issues were found"

#define SHERR_DUMP_COMPATIBILITY_OPTIONS_FAILED 52005
#define SHERR_DUMP_COMPATIBILITY_OPTIONS_FAILED_MSG \
  "Could not apply some of the compatibility options"

#define SHERR_DUMP_WORKER_THREAD_FATAL_ERROR 52006
#define SHERR_DUMP_WORKER_THREAD_FATAL_ERROR_MSG "Fatal error during dump"

#define SHERR_DUMP_MISSING_GLOBAL_PRIVILEGES 52007
#define SHERR_DUMP_MISSING_GLOBAL_PRIVILEGES_MSG \
  "User %s is missing the following global privilege(s): %s."

#define SHERR_DUMP_MISSING_SCHEMA_PRIVILEGES 52008
#define SHERR_DUMP_MISSING_SCHEMA_PRIVILEGES_MSG \
  "User %s is missing the following privilege(s) for schema %s: %s."

#define SHERR_DUMP_MISSING_TABLE_PRIVILEGES 52009
#define SHERR_DUMP_MISSING_TABLE_PRIVILEGES_MSG \
  "User %s is missing the following privilege(s) for table %s: %s."

#define SHERR_DUMP_NO_SCHEMAS_SELECTED 52010
#define SHERR_DUMP_NO_SCHEMAS_SELECTED_MSG \
  "Filters for schemas result in an empty set."

#define SHERR_DUMP_MANIFEST_PAR_CREATION_FAILED 52011
#define SHERR_DUMP_MANIFEST_PAR_CREATION_FAILED_MSG \
  "Failed creating PAR for object '%s': %s"

#define SHERR_DUMP_DW_WRITE_FAILED 52012
#define SHERR_DUMP_DW_WRITE_FAILED_MSG "Failed to write %s into file %s"

#define SHERR_DUMP_IC_FAILED_TO_FETCH_VERSION 52013
#define SHERR_DUMP_IC_FAILED_TO_FETCH_VERSION_MSG \
  "Failed to fetch version of the server."

#define SHERR_DUMP_SD_CHARSET_NOT_FOUND 52014
#define SHERR_DUMP_SD_CHARSET_NOT_FOUND_MSG "Unable to find charset: %s"

#define SHERR_DUMP_SD_WRITE_FAILED 52015
#define SHERR_DUMP_SD_WRITE_FAILED_MSG "Got errno %d on write"

#define SHERR_DUMP_SD_QUERY_FAILED 52016
#define SHERR_DUMP_SD_QUERY_FAILED_MSG "Could not execute '%s': %s"

#define SHERR_DUMP_SD_COLLATION_DATABASE_ERROR 52017
#define SHERR_DUMP_SD_COLLATION_DATABASE_ERROR_MSG \
  "Error processing select @@collation_database; results"

#define SHERR_DUMP_SD_CHARACTER_SET_RESULTS_ERROR 52018
#define SHERR_DUMP_SD_CHARACTER_SET_RESULTS_ERROR_MSG \
  "Unable to set character_set_results to: %s"

#define SHERR_DUMP_SD_CANNOT_CREATE_DELIMITER 52019
#define SHERR_DUMP_SD_CANNOT_CREATE_DELIMITER_MSG \
  "Can't create delimiter for event: %s"

#define SHERR_DUMP_SD_INSUFFICIENT_PRIVILEGES 52020
#define SHERR_DUMP_SD_INSUFFICIENT_PRIVILEGES_MSG \
  "%s has insufficient privileges to %s!"

#define SHERR_DUMP_SD_MISSING_TABLE 52021
#define SHERR_DUMP_SD_MISSING_TABLE_MSG "%s not present in information_schema"

#define SHERR_DUMP_SD_SHOW_CREATE_TABLE_FAILED 52022
#define SHERR_DUMP_SD_SHOW_CREATE_TABLE_FAILED_MSG \
  "Failed running: show create table %s with error: %s"

#define SHERR_DUMP_SD_SHOW_CREATE_TABLE_EMPTY 52023
#define SHERR_DUMP_SD_SHOW_CREATE_TABLE_EMPTY_MSG \
  "Empty create table for table: %s"

#define SHERR_DUMP_SD_SHOW_FIELDS_FAILED 52024
#define SHERR_DUMP_SD_SHOW_FIELDS_FAILED_MSG \
  "SHOW FIELDS FROM failed on view: %s"

#define SHERR_DUMP_SD_SHOW_KEYS_FAILED 52025
#define SHERR_DUMP_SD_SHOW_KEYS_FAILED_MSG "Can't get keys for table %s: %s"

#define SHERR_DUMP_SD_SHOW_CREATE_VIEW_FAILED 52026
#define SHERR_DUMP_SD_SHOW_CREATE_VIEW_FAILED_MSG "Failed: SHOW CREATE TABLE %s"

#define SHERR_DUMP_SD_SHOW_CREATE_VIEW_EMPTY 52027
#define SHERR_DUMP_SD_SHOW_CREATE_VIEW_EMPTY_MSG "No information about view: %s"

#define SHERR_DUMP_SD_SCHEMA_DDL_ERROR 52028
#define SHERR_DUMP_SD_SCHEMA_DDL_ERROR_MSG \
  "Error while dumping DDL for schema '%s': %s"

#define SHERR_DUMP_SD_TABLE_DDL_ERROR 52029
#define SHERR_DUMP_SD_TABLE_DDL_ERROR_MSG \
  "Error while dumping DDL for table '%s'.'%s': %s"

#define SHERR_DUMP_SD_VIEW_TEMPORARY_DDL_ERROR 52030
#define SHERR_DUMP_SD_VIEW_TEMPORARY_DDL_ERROR_MSG \
  "Error while dumping temporary DDL for view '%s'.'%s': %s"

#define SHERR_DUMP_SD_VIEW_DDL_ERROR 52031
#define SHERR_DUMP_SD_VIEW_DDL_ERROR_MSG \
  "Error while dumping DDL for view '%s'.'%s': %s"

#define SHERR_DUMP_SD_TRIGGER_COUNT_ERROR 52032
#define SHERR_DUMP_SD_TRIGGER_COUNT_ERROR_MSG \
  "Unable to check trigger count for table: '%s'.'%s'"

#define SHERR_DUMP_SD_TRIGGER_DDL_ERROR 52033
#define SHERR_DUMP_SD_TRIGGER_DDL_ERROR_MSG \
  "Error while dumping triggers for table '%s'.'%s': %s"

#define SHERR_DUMP_SD_EVENT_DDL_ERROR 52034
#define SHERR_DUMP_SD_EVENT_DDL_ERROR_MSG \
  "Error while dumping events for schema '%s': %s"

#define SHERR_DUMP_SD_ROUTINE_DDL_ERROR 52035
#define SHERR_DUMP_SD_ROUTINE_DDL_ERROR_MSG \
  "Error while dumping routines for schema '%s': %s"

#define SHERR_DUMP_ACCOUNT_WITH_APOSTROPHE 52036
#define SHERR_DUMP_ACCOUNT_WITH_APOSTROPHE_MSG \
  "Account %s contains the ' character, which is not supported"

#define SHERR_DUMP_LAST 52036

#define SHERR_DUMP_MAX 52999

#endif  // MODULES_UTIL_DUMP_DUMP_ERRORS_H_

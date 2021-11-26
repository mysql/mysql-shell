/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_LOAD_LOAD_ERRORS_H_
#define MODULES_UTIL_LOAD_LOAD_ERRORS_H_

#include "modules/util/dump/common_errors.h"

#define SHERR_LOAD_MIN 53000

#define SHERR_LOAD_FIRST SHERR_LOAD_MIN

#define SHERR_LOAD_MANIFEST_EXPIRED_PARS 53000
#define SHERR_LOAD_MANIFEST_EXPIRED_PARS_MSG                                 \
  "The PARs in the manifest file have expired, the expiration time was set " \
  "to: %s"

#define SHERR_LOAD_MANIFEST_PAR_MISMATCH 53001
#define SHERR_LOAD_MANIFEST_PAR_MISMATCH_MSG \
  "The provided PAR must be a file on the dump location: '%s'"

#define SHERR_LOAD_SPLITTING_DDL_FAILED 53002
#define SHERR_LOAD_SPLITTING_DDL_FAILED_MSG \
  "Error splitting DDL script for table %s: %s"

#define SHERR_LOAD_SECONDARY_ENGINE_ERROR 53003
#define SHERR_LOAD_SECONDARY_ENGINE_ERROR_MSG                               \
  "The table %s has a secondary engine set, but not all indexes have been " \
  "recreated"

#define SHERR_LOAD_FAILED_TO_DISABLE_BINLOG 53004
#define SHERR_LOAD_FAILED_TO_DISABLE_BINLOG_MSG \
  "'SET sql_log_bin=0' failed with error: %s"

#define SHERR_LOAD_WORKER_THREAD_FATAL_ERROR 53005
#define SHERR_LOAD_WORKER_THREAD_FATAL_ERROR_MSG "Error loading dump"

#define SHERR_LOAD_UNSUPPORTED_DUMP_VERSION 53006
#define SHERR_LOAD_UNSUPPORTED_DUMP_VERSION_MSG "Unsupported dump version"

#define SHERR_LOAD_UNSUPPORTED_DUMP_CAPABILITIES 53007
#define SHERR_LOAD_UNSUPPORTED_DUMP_CAPABILITIES_MSG \
  "Unsupported dump capabilities"

#define SHERR_LOAD_INCOMPLETE_DUMP 53008
#define SHERR_LOAD_INCOMPLETE_DUMP_MSG "Incomplete dump"

#define SHERR_LOAD_UNSUPPORTED_SERVER_VERSION 53009
#define SHERR_LOAD_UNSUPPORTED_SERVER_VERSION_MSG \
  "Loading dumps is only supported in MySQL 5.7 or newer"

#define SHERR_LOAD_DUMP_NOT_MDS_COMPATIBLE 53010
#define SHERR_LOAD_DUMP_NOT_MDS_COMPATIBLE_MSG "Dump is not MDS compatible"

#define SHERR_LOAD_SERVER_VERSION_MISMATCH 53011
#define SHERR_LOAD_SERVER_VERSION_MISMATCH_MSG "MySQL version mismatch"

#define SHERR_LOAD_UPDATE_GTID_GR_IS_RUNNING 53012
#define SHERR_LOAD_UPDATE_GTID_GR_IS_RUNNING_MSG                              \
  "The updateGtidSet option cannot be used on server with group replication " \
  "running."

#define SHERR_LOAD_UPDATE_GTID_APPEND_NOT_SUPPORTED 53013
#define SHERR_LOAD_UPDATE_GTID_APPEND_NOT_SUPPORTED_MSG \
  "Target MySQL server does not support updateGtidSet:'append'."

#define SHERR_LOAD_UPDATE_GTID_REQUIRES_SKIP_BINLOG 53014
#define SHERR_LOAD_UPDATE_GTID_REQUIRES_SKIP_BINLOG_MSG                      \
  "The updateGtidSet option on MySQL 5.7 target server can only be used if " \
  "the skipBinlog option is enabled."

#define SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_EMPTY_VARIABLES 53015
#define SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_EMPTY_VARIABLES_MSG          \
  "The updateGtidSet:'replace' option can be used on target server version " \
  "only if GTID_PURGED and GTID_EXECUTED are empty, but they are not."

#define SHERR_LOAD_UPDATE_GTID_REPLACE_SETS_INTERSECT 53016
#define SHERR_LOAD_UPDATE_GTID_REPLACE_SETS_INTERSECT_MSG               \
  "The updateGtidSet:'replace' option can only be used if "             \
  "gtid_subtract(gtid_executed,gtid_purged) on target server does not " \
  "intersect with the dumped GTID set."

#define SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_SUPERSET 53017
#define SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_SUPERSET_MSG                \
  "The updateGtidSet:'replace' option can only be used if the dumped GTID " \
  "set is a superset of the current value of gtid_purged on target server."

#define SHERR_LOAD_UPDATE_GTID_APPEND_SETS_INTERSECT 53018
#define SHERR_LOAD_UPDATE_GTID_APPEND_SETS_INTERSECT_MSG                    \
  "The updateGtidSet:'append' option can only be used if gtid_executed on " \
  "target server does not intersect with the dumped GTID set."

#define SHERR_LOAD_INVISIBLE_PKS_UNSUPPORTED_SERVER_VERSION 53019
#define SHERR_LOAD_INVISIBLE_PKS_UNSUPPORTED_SERVER_VERSION_MSG \
  "The 'createInvisiblePKs' option requires server 8.0.24 or newer."

#define SHERR_LOAD_REQUIRE_PRIMARY_KEY_ENABLED 53020
#define SHERR_LOAD_REQUIRE_PRIMARY_KEY_ENABLED_MSG \
  "sql_require_primary_key enabled at destination server"

#define SHERR_LOAD_DUPLICATE_OBJECTS_FOUND 53021
#define SHERR_LOAD_DUPLICATE_OBJECTS_FOUND_MSG \
  "Duplicate objects found in destination database"

#define SHERR_LOAD_DUMP_WAIT_TIMEOUT 53022
#define SHERR_LOAD_DUMP_WAIT_TIMEOUT_MSG "Dump timeout"

#define SHERR_LOAD_INVALID_METADATA_FILE 53023
#define SHERR_LOAD_INVALID_METADATA_FILE_MSG "Invalid metadata file %s"

#define SHERR_LOAD_PARSING_METADATA_FILE_FAILED 53024
#define SHERR_LOAD_PARSING_METADATA_FILE_FAILED_MSG \
  "Could not parse metadata file %s: %s"

#define SHERR_LOAD_LOCAL_INFILE_DISABLED 53025
#define SHERR_LOAD_LOCAL_INFILE_DISABLED_MSG "local_infile disabled in server"

#define SHERR_LOAD_PROGRESS_FILE_ERROR 53026
#define SHERR_LOAD_PROGRESS_FILE_ERROR_MSG \
  "Error loading load progress file '%s': %s"

#define SHERR_LOAD_PROGRESS_FILE_UUID_MISMATCH 53027
#define SHERR_LOAD_PROGRESS_FILE_UUID_MISMATCH_MSG                         \
  "Progress file was created for a server with UUID %s, while the target " \
  "server has UUID: %s"

#define SHERR_LOAD_MANIFEST_UNKNOWN_OBJECT 53028
#define SHERR_LOAD_MANIFEST_UNKNOWN_OBJECT_MSG "Unknown object in manifest: %s"

#define SHERR_LOAD_LAST 53028

#define SHERR_LOAD_MAX 53999

#endif  // MODULES_UTIL_LOAD_LOAD_ERRORS_H_

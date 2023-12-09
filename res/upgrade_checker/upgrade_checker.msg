# Copyright (c) 2018, 2023, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# This file defines translations one by one (comments can be added for readability starting with
# '#' character). Format for single translation is as follows:
#
# -------------------------------------------------------------------
# * Translation id: one or more lines (if id contains new line characters)
#
# # Zero or more lines: text - originally given in source code with formatting suggestions
#
# One or more lines - translation. Line breaks will be ignored and can be put in translations for
# readability - if line break is intended deliberately, line must end with '\n' character sequence.
#
# Empty line to terminate single translation
# -------------------------------------------------------------------

* oldTemporalCheck.title
# Usage of old temporal type

* oldTemporalCheck.description
# Error: Following table columns use a deprecated and no longer supported temporal disk storage
# format. They must be converted to the new format before upgrading. It can by done by rebuilding
# the table using 'ALTER TABLE <table_name> FORCE' command

* oldTemporalCheck.docLink
https://mysqlserverteam.com/mysql-8-0-removing-support-for-old-temporal-datatypes/

* routinesSyntaxCheck.title
# MySQL 8.0 syntax check for routine-like objects

* routinesSyntaxCheck.description
# The following objects did not pass a syntax check with the latest MySQL 8.0 grammar. A common
# reason is that they reference names that conflict with new reserved keywords. You must update
# these routine definitions and `quote` any such references before upgrading.

* routinesSyntaxCheck.docLink
https://dev.mysql.com/doc/refman/en/keywords.html

* reservedKeywordsCheck.title
# Usage of db objects with names conflicting with new reserved keywords

* reservedKeywordsCheck.description
# Warning: The following objects have names that conflict with new reserved keywords. Ensure
# queries sent by your applications use `quotes` when referring to them or they will result in
# errors.

* reservedKeywordsCheck.docLink
https://dev.mysql.com/doc/refman/en/keywords.html

* utf8mb3Check.title
# Usage of utf8mb3 charset

* utf8mb3Check.description
# Warning: The following objects use the utf8mb3 character set. It is recommended to convert them
# to use utf8mb4 instead, for improved Unicode support.

* utf8mb3Check.docLink
https://dev.mysql.com/doc/refman/8.0/en/charset-unicode-utf8mb3.html

* mysqlSchemaCheck.title
# Table names in the mysql schema conflicting with new tables in 8.0

* mysqlSchemaCheck.description
# Error: The following tables in mysql schema have names that will conflict with the ones
# introduced in 8.0 version. They must be renamed or removed before upgrading (use RENAME TABLE
# command). This may also entail changes to applications that use the affected tables.

* mysqlSchemaCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-strategies.html#upgrade-prerequisites

* nonNativePartitioningCheck.title
# Partitioned tables using engines with non native partitioning

* nonNativePartitioningCheck.description
# Error: In MySQL 8.0 storage engine is responsible for providing its own partitioning handler, and
# the MySQL server no longer provides generic partitioning support. InnoDB and NDB are the only
# storage engines that provide a native partitioning handler that is supported in MySQL 8.0. A
# partitioned table using any other storage engine must be altered—either to convert it to InnoDB
# or NDB, or to remove its partitioning—before upgrading the server, else it cannot be used
# afterwards.

* nonNativePartitioningCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-configuration-changes

* foreignKeyLengthCheck.title
# Foreign key constraint names longer than 64 characters

* foreignKeyLengthCheck.description
# Error: The following tables must be altered to have constraint names shorter than 64 characters
# (use ALTER TABLE).

* foreignKeyLengthCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-strategies.html#upgrade-prerequisites

* maxdbFlagCheck.title
# Usage of obsolete MAXDB sql_mode flag

* maxdbFlagCheck.description
# Warning: The following DB objects have the obsolete MAXDB option persisted for sql_mode, which
# will be cleared during upgrade to 8.0. It can potentially change the datatype DATETIME into
# TIMESTAMP if it is used inside object's definition, and this in turn can change the behavior in
# case of dates earlier than 1970 or later than 2037. If this is a concern, please redefine these
# objects so that they do not rely on the MAXDB flag before running the upgrade to 8.0.

* maxdbFlagCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* sqlModeFlagCheck.title
# Usage of obsolete sql_mode flags

* sqlModeFlagCheck.description
# Notice: The following DB objects have obsolete options persisted for sql_mode, which will be
# cleared during upgrade to 8.0.

* sqlModeFlagCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* enumSetElementLenghtCheck.title
# ENUM/SET column definitions containing elements longer than 255 characters

* enumSetElementLenghtCheck.description
# Error: The following columns are defined as either ENUM or SET and contain at least one element
# longer that 255 characters. They need to be altered so that all elements fit into the 255
# characters limit.

* enumSetElementLenghtCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/string-type-overview.html

* partitionedTablesInSharedTablespaceCheck.title
# Usage of partitioned tables in shared tablespaces

* partitionedTablesInSharedTablespaceCheck.description
# Error: The following tables have partitions in shared tablespaces. Before upgrading to 8.0 they
# need to be moved to file-per-table tablespace. You can do this by running query like 'ALTER TABLE
# table_name REORGANIZE PARTITION X INTO (PARTITION X VALUES LESS THAN (30)
# TABLESPACE=innodb_file_per_table);'

* partitionedTablesInSharedTablespaceCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* circularDirectoryCheck.title
# Circular directory references in tablespace data file paths

* circularDirectoryCheck.description
# Error: Following tablespaces contain circular directory references (e.g. the reference '/../') in
# data file paths which as of MySQL 8.0.17 are not permitted by the CREATE TABLESPACE ... ADD
# DATAFILE clause. An exception to the restriction exists on Linux, where a circular directory
# reference is permitted if the preceding directory is a symbolic link. To avoid upgrade issues,
# remove any circular directory references from tablespace data file paths before upgrading.

* circularDirectoryCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-innodb-changes

* removedFunctionsCheck.title
# Usage of removed functions

* removedFunctionsCheck.description
# Error: Following DB objects make use of functions that have been removed in version 8.0. Please
# make sure to update them to use supported alternatives before upgrade.

* removedFunctionsCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* groupByAscCheck.title
# Usage of removed GROUP BY ASC/DESC syntax

* groupByAscCheck.description
# Error: The following DB objects use removed GROUP BY ASC/DESC syntax. They need to be altered so
# that ASC/DESC keyword is removed from GROUP BY clause and placed in appropriate ORDER BY clause.

* groupByAscCheck.docLink
https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-13.html#mysqld-8-0-13-sql-syntax

* removedSysLogVars.title
# Removed system variables for error logging to the system log configuration

* removedSysLogVars.description
# Error: System variables related to logging using OS facilities (the Event Log on Windows, and
# syslog on Unix and Unix-like systems) have been removed. Where appropriate, the removed system
# variables were replaced with new system variables managed by the log_sink_syseventlog error log
# component. Installations that used the old system variable names must update their configuration
# to use the new variable names.

* removedSysLogVars.docLink
https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-13.html#mysqld-8-0-13-logging

* removedSysVars.title
# Removed system variables

* removedSysVars.description
# Error: Following system variables that were detected as being used will be removed. Please update
# your system to not rely on them before the upgrade.

* removedSysVars.docLink
https://dev.mysql.com/doc/refman/8.0/en/added-deprecated-removed.html#optvars-removed

* sysVarsNewDefaults.title
# System variables with new default values

* sysVarsNewDefaults.description
# Warning: Following system variables that are not defined in your configuration file will have new
# default values. Please review if you rely on their current values and if so define them before
# performing upgrade.

* sysVarsNewDefaults.docLink
https://mysqlserverteam.com/new-defaults-in-mysql-8-0/

* zeroDatesCheck.title
# Zero Date, Datetime, and Timestamp values

* zeroDatesCheck.description
# Warning: By default zero date/datetime/timestamp values are no longer allowed in MySQL, as of
# 5.7.8 NO_ZERO_IN_DATE and NO_ZERO_DATE are included in SQL_MODE by default. These modes should be
# used with strict mode as they will be merged with strict mode in a future release. If you do not
# include these modes in your SQL_MODE setting, you are able to insert date/datetime/timestamp
# values that contain zeros. It is strongly advised to replace zero values with valid ones, as they
# may not work correctly in the future.

* zeroDatesCheck.docLink
https://lefred.be/content/mysql-8-0-and-wrong-dates/

* schemaInconsistencyCheck.title
# Schema inconsistencies resulting from file removal or corruption

* schemaInconsistencyCheck.description
# Error: Following tables show signs that either table datadir directory or frm file was
# removed/corrupted. Please check server logs, examine datadir to detect the issue and fix it
# before upgrade

* schemaInconsistencyCheck.docLink

* ftsTablenameCheck.title
# Table names containing 'FTS'

* ftsTablenameCheck.description
# Error: Upgrading from 5.7 to 8.0 does not support tables with name containing 'FTS' character
# string. The workaround is to rename the table for the upgrade - e.g. it is enough to change any
# letter of the 'FTS' part to a lower case. It can be renamed back again after the upgrade.

* ftsTablenameCheck.docLink

* engineMixupCheck.title
# Tables recognized by InnoDB that belong to a different engine

* engineMixupCheck.description
# Error
Error: Following tables are recognized by InnoDB engine while the SQL layer believes they belong to
a different engine. Such situation may happen when one removes InnoDB table files manually from the
disk and creates e.g. a MyISAM table with the same name.\n
\n
A possible way to solve this situation is to e.g. in case of MyISAM table:\n
\n
1. Rename the MyISAM table to a temporary name (RENAME TABLE).\n
2. Create some dummy InnoDB table (its definition does not need to match), then copy (copy, not
move) and rename the dummy .frm and .ibd files to the orphan name using OS file commands.\n
3. The orphan table can be then dropped (DROP TABLE), as well as the dummy table.\n
4. Finally the MyISAM table can be renamed back to its original name.

* engineMixupCheck.docLink

* oldGeometryCheck.title
# Spatial data columns created in MySQL 5.6

* oldGeometryCheck.description
# Error
Error: The following columns are spatial data columns created in MySQL Server version 5.6. MySQL
8.0 servers before 8.0.24 do not support upgrading these tables and will reject them. In order for
the upgrade to succeed we encourage you to upgrade directly to MySQL Server version 8.0.24 or
newer. Otherwise, you must rebuild these tables using ALTER TABLE FORCE before upgrading.

* oldGeometryCheck.docLink

* checkTableOutput.title
# Issues reported by 'check table x for upgrade' command

* checkTableOutput.description

* checkTableOutput.docLink

* defaultAuthenticationPlugin.title
New default authentication plugin considerations

* defaultAuthenticationPlugin.description
# Warning
Warning: The new default authentication plugin 'caching_sha2_password' offers more secure password
hashing than previously used 'mysql_native_password' (and consequent improved client connection
authentication). However, it also has compatibility implications that may affect existing MySQL
installations.  If your MySQL installation must serve pre-8.0 clients and you encounter
compatibility issues after upgrading, the simplest way to address those issues is to reconfigure
the server to revert to the previous default authentication plugin (mysql_native_password). For
example, use these lines in the server option file:\n
\n
[mysqld]\n
default_authentication_plugin=mysql_native_password\n
\n
However, the setting should be viewed as temporary, not as a long term or permanent solution,
because it causes new accounts created with the setting in effect to forego the improved
authentication security.\n
If you are using replication please take time to understand how the authentication plugin changes
may impact you.

* defaultAuthenticationPlugin.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-compatibility-issues\n
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-replication

* defaultAuthenticationPluginMds.title
New default authentication plugin considerations

* defaultAuthenticationPluginMds.description
# Warning
Warning: The new default authentication plugin 'caching_sha2_password' offers more secure password
hashing than previously used 'mysql_native_password' (and consequent improved client connection
authentication). However, it also has compatibility implications that may affect existing MySQL
installations.  If your MySQL installation must serve pre-8.0 clients, you may encounter
compatibility issues after upgrading unless newly created accounts are created to use
'mysql_native_password'.

* defaultAuthenticationPluginMds.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-compatibility-issues\n
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-replication

* changedFunctionsInGeneratedColumnsCheck.title
# Indexes on functions with changed semantics

* changedFunctionsInGeneratedColumnsCheck.description
# Warning
Warning: The following tables have indexes on generated columns that use functions that have had
changes in their semantics. Replication problems and index corruption may arise unless the
underlying issue is resolved before upgrade. See documentation for more information.

* changedFunctionsInGeneratedColumnsCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-sql-changes

* columnsWhichCannotHaveDefaultsCheck.title
# Columns which cannot have default values

* columnsWhichCannotHaveDefaultsCheck.description
# Error
Error: The following columns are defined as either BLOB, TEXT, GEOMETRY or JSON and have a default
value set. These data types cannot have default values in MySQL versions prior to 8.0.13, while
starting with 8.0.13, the default value must be specified as an expression. In order to fix this
issue, please use the ALTER TABLE ... ALTER COLUMN ... DROP DEFAULT statement.

* columnsWhichCannotHaveDefaultsCheck.docLink
https://dev.mysql.com/doc/refman/8.0/en/data-type-defaults.html#data-type-defaults-explicit

* mysqlInvalid57NamesCheck.title
# Check for invalid table names and schema names used in 5.7

* mysqlInvalid57NamesCheck.description
# Error: The following tables and/or schemas have invalid names. In order to fix them use the
# mysqlcheck utility as follows:\n
# \n
#   $ mysqlcheck --check-upgrade --all-databases\n
#   $ mysqlcheck --fix-db-names --fix-table-names --all-databases\n
# \n
# OR via mysql client, for eg:\n
# \n
#   ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;
The following tables and/or schemas have invalid names. In order to fix them use the mysqlcheck
utility as follows:\n
\n
  $ mysqlcheck --check-upgrade --all-databases\n
  $ mysqlcheck --fix-db-names --fix-table-names --all-databases\n
\n
OR via mysql client, for eg:\n
\n
  ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;

* mysqlInvalid57NamesCheck.docLink
https://dev.mysql.com/doc/refman/5.7/en/identifier-mapping.html
https://dev.mysql.com/doc/refman/5.7/en/alter-database.html
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* mysqlOrphanedRoutinesCheck.title
# Check for orphaned routines in 5.7

* mysqlOrphanedRoutinesCheck.description
# Error: The following routines have been orphaned. Schemas that they are referencing no longer
# exists.\n
# They have to be cleaned up or the upgrade will fail.

* mysqlOrphanedRoutinesCheck.docLink

* mysqlDollarSignNameCheck.title
# Check for deprecated usage of single dollar signs in object names

* mysqlDollarSignNameCheck.description
# Warning: The following objects have names with deprecated usage of dollar sign ($) at the
# begining of the identifier. To correct this warning, ensure, that names starting with dollar
# sign, also end with it, similary to quotes ($example$). 

* mysqlDollarSignNameCheck.docLink

* mysqlIndexTooLargeCheck.title
# Check for indexes that are too large to work on higher versions of MySQL Server than 5.7

* mysqlIndexTooLargeCheck.description
# Error: The following indexes ware made too large for their format in an older version of MySQL
# (older than 5.7.34). Normally those indexes within tables with compact or redundant row formats
# shouldn't be larger than 767 bytes. To fix this problem those indexes should be dropped before
# upgrading or those tables will be inaccessible.

* mysqlIndexTooLargeCheck.docLink

* mysqlEmptyDotTableSyntaxCheck.title
# Check for deprecated '.<table>' syntax used in routines.

* mysqlEmptyDotTableSyntaxCheck.description
# Error: The following routines contain identifiers in deprecated identifier syntax (".<table>"),
# and should be corrected before upgrade:\n

* mysqlEmptyDotTableSyntaxCheck.docLink

* mysqlInvalidEngineForeignKeyCheck.title
# Check for columns that have foreign keys pointing to tables from a diffrent database engine.

* mysqlInvalidEngineForeignKeyCheck.description
# Error: The following columns have foreign keys pointing to tables from diffrent database engines
# than originating table. This is invalid and probably a mistake done when FOREIGN_KEY_CHECKS was
# turned OFF:\n

* mysqlInvalidEngineForeignKeyCheck.docLink


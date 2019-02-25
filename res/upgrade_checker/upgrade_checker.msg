# Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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
# Error: Following table columns use a deprecated and no longer supported timestamp disk storage
# format. They must be converted to the new format before upgrading. It can by done by rebuilding
# the table using 'ALTER TABLE <table_name> FORCE' command

* oldTemporalCheck.docLink
https://mysqlserverteam.com/mysql-8-0-removing-support-for-old-temporal-datatypes/

* reservedKeywordsCheck.title
# Usage of db objects with names conflicting with reserved keywords in 8.0

* reservedKeywordsCheck.description
# Warning: The following objects have names that conflict with reserved keywords that are new to
# 8.0. Ensure queries sent by your applications use `quotes` when referring to them or they will
# result in errors.

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

* schemaInconsistencyCheck.title
# Schema inconsistencies resulting from file removal or corruption

* schemaInconsistencyCheck.description
# Error: Following tables show signs that either table datadir directory or frm file was
# removed/corrupted. Please check server logs, examine datadir to detect the issue and fix it
# before upgrade

* schemaInconsistencyCheck.docLink

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


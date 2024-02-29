# Copyright (c) 2018, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
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

* oldTemporal.title
Usage of old temporal type

* oldTemporal.description
Error: Following table columns use a deprecated and no longer supported temporal disk storage
format. They must be converted to the new format before upgrading. It can by done by rebuilding
the table using 'ALTER TABLE <table_name> FORCE' command

* oldTemporal.docLink
https://dev.mysql.com/blog-archive/mysql-8-0-removing-support-for-old-temporal-datatypes/

* routineSyntax.title
MySQL syntax check for routine-like objects

* routineSyntax.description
The following objects did not pass a syntax check with the latest MySQL grammar. A common reason
is that they reference names that conflict with new reserved keywords. You must update these
routine definitions and `quote` any such references before upgrading.

* routineSyntax.docLink
https://dev.mysql.com/doc/refman/en/keywords.html

* reservedKeywords.title
Usage of db objects with names conflicting with new reserved keywords

* reservedKeywords.description
Warning: The following objects have names that conflict with new reserved keywords. Ensure
queries sent by your applications use `quotes` when referring to them or they will result in
errors.

* reservedKeywords.docLink
https://dev.mysql.com/doc/refman/en/keywords.html

* utf8mb3.title
Usage of utf8mb3 charset

* utf8mb3.description
Warning: The following objects use the utf8mb3 character set. It is recommended to convert them
to use utf8mb4 instead, for improved Unicode support.

* utf8mb3.docLink
https://dev.mysql.com/doc/refman/8.0/en/charset-unicode-utf8mb3.html

* mysqlSchema.title
Table names in the mysql schema conflicting with new tables in the latest MySQL.

* mysqlSchema.description
Error: The following tables in mysql schema have names that will conflict with the ones
introduced in the latest version. They must be renamed or removed before upgrading (use RENAME
TABLE command). This may also entail changes to applications that use the affected tables.

* mysqlSchema.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrade-before-you-begin.html

* nonNativePartitioning.title
Partitioned tables using engines with non native partitioning

* nonNativePartitioning.description
Error: In the latest MySQL storage engine is responsible for providing its own partitioning
handler, and the MySQL server no longer provides generic partitioning support. InnoDB and NDB are
the only storage engines that provide a native partitioning handler that is supported in the
latest MySQL. A partitioned table using any other storage engine must be altered—either to
convert it to InnoDB or NDB, or to remove its partitioning—before upgrading the server, else it
cannot be used afterwards.

* nonNativePartitioning.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-configuration-changes

* foreignKeyLength.title
Foreign key constraint names longer than 64 characters

* foreignKeyLength.description
Error: The following tables must be altered to have constraint names shorter than 64 characters
(use ALTER TABLE).

* foreignKeyLength.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrade-before-you-begin.html

* maxdbSqlModeFlags.title
Usage of obsolete MAXDB sql_mode flag

* maxdbSqlModeFlags.description
Warning: The following DB objects have the obsolete MAXDB option persisted for sql_mode, which
will be cleared during the upgrade. It can potentially change the datatype DATETIME into
TIMESTAMP if it is used inside object's definition, and this in turn can change the behavior in
case of dates earlier than 1970 or later than 2037. If this is a concern, please redefine these
objects so that they do not rely on the MAXDB flag before running the upgrade.

* maxdbSqlModeFlags.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* obsoleteSqlModeFlags.title
Usage of obsolete sql_mode flags

* obsoleteSqlModeFlags.description
Notice: The following DB objects have obsolete options persisted for sql_mode, which will be
cleared during the upgrade.

* obsoleteSqlModeFlags.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* enumSetElementLength.title
ENUM/SET column definitions containing elements longer than 255 characters

* enumSetElementLength.description
Error: The following columns are defined as either ENUM or SET and contain at least one element
longer that 255 characters. They need to be altered so that all elements fit into the 255
characters limit.

* enumSetElementLength.docLink
https://dev.mysql.com/doc/refman/8.0/en/string-type-overview.html

* partitionedTablesInSharedTablespaces.title
Usage of partitioned tables in shared tablespaces

* partitionedTablesInSharedTablespaces.description
Error: The following tables have partitions in shared tablespaces. They need to be moved to
file-per-table tablespace before upgrading. You can do this by running query like 'ALTER TABLE
table_name REORGANIZE PARTITION X INTO (PARTITION X VALUES LESS THAN (30)
TABLESPACE=innodb_file_per_table);'

* partitionedTablesInSharedTablespaces.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* circularDirectory.title
Circular directory references in tablespace data file paths

* circularDirectory.description
Error: Following tablespaces contain circular directory references (e.g. the reference '/../') in
data file paths which as of MySQL 8.0.17 are not permitted by the CREATE TABLESPACE ... ADD
DATAFILE clause. An exception to the restriction exists on Linux, where a circular directory
reference is permitted if the preceding directory is a symbolic link. To avoid upgrade issues,
remove any circular directory references from tablespace data file paths before upgrading.

* circularDirectory.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-innodb-changes

* removedFunctions.title
Usage of removed functions

* removedFunctions.description
Error: Following DB objects make use of functions that have been removed int the latest MySQL
version. Please make sure to update them to use supported alternatives before upgrade.

* removedFunctions.docLink
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* groupbyAscSyntax.title
Usage of removed GROUP BY ASC/DESC syntax

* groupbyAscSyntax.description
Error: The following DB objects use removed GROUP BY ASC/DESC syntax. They need to be altered so
that ASC/DESC keyword is removed from GROUP BY clause and placed in appropriate ORDER BY clause.

* groupbyAscSyntax.docLink
https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-13.html#mysqld-8-0-13-sql-syntax

* removedSysLogVars.title
Removed system variables for error logging to the system log configuration

* removedSysLogVars.description
Error: System variables related to logging using OS facilities (the Event Log on Windows, and
syslog on Unix and Unix-like systems) have been removed. Where appropriate, the removed system
variables were replaced with new system variables managed by the log_sink_syseventlog error log
component. Installations that used the old system variable names must update their configuration
to use the new variable names.

* removedSysLogVars.docLink
https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-13.html#mysqld-8-0-13-logging

* removedSysVars.title
Removed system variables

* removedSysVars.description
Error: Following system variables that were detected as being used will be removed. Please update
your system to not rely on them before the upgrade.

* removedSysVars.issue
The system variable '%item%' is set to %value% (%source%) and will be removed

* removedSysVars.replacement
consider using '%replacement%' instead

* removedSysVars.docLink
https://dev.mysql.com/doc/refman/8.0/en/added-deprecated-removed.html#optvars-removed

* sysVarsNewDefaults.title
System variables with new default values

* sysVarsNewDefaults.description
Warning: Following system variables that are not defined in your configuration file will have new
default values. Please review if you rely on their current values and if so define them before
performing upgrade.

* sysVarsNewDefaults.issue
default value will change

* sysVarsNewDefaults.docLink
https://dev.mysql.com/blog-archive/new-defaults-in-mysql-8-0/

* zeroDates.title
Zero Date, Datetime, and Timestamp values

* zeroDates.description
Warning: By default zero date/datetime/timestamp values are no longer allowed in MySQL, as of
5.7.8 NO_ZERO_IN_DATE and NO_ZERO_DATE are included in SQL_MODE by default. These modes should be
used with strict mode as they will be merged with strict mode in a future release. If you do not
include these modes in your SQL_MODE setting, you are able to insert date/datetime/timestamp
values that contain zeros. It is strongly advised to replace zero values with valid ones, as they
may not work correctly in the future.

* zeroDates.docLink
https://lefred.be/content/mysql-8-0-and-wrong-dates/

* schemaInconsistency.title
Schema inconsistencies resulting from file removal or corruption

* schemaInconsistency.description
Error: Following tables show signs that either table datadir directory or frm file was
removed/corrupted. Please check server logs, examine datadir to detect the issue and fix it
before upgrade

* ftsInTablename.title
Table names containing 'FTS'

* ftsInTablename.description
Error: Upgrading from 5.7 to the latest MySQL version does not support tables with name
containing 'FTS' character string. The workaround is to rename the table for the upgrade - e.g.
it is enough to change any letter of the 'FTS' part to a lower case. It can be renamed back again
after the upgrade.

* engineMixup.title
Tables recognized by InnoDB that belong to a different engine

* engineMixup.description
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

* oldGeometryTypes.title
Spatial data columns created in MySQL 5.6

* oldGeometryTypes.description
Error: The following columns are spatial data columns created in MySQL Server version 5.6. MySQL
8.0 servers before 8.0.24 do not support upgrading these tables and will reject them. In order for
the upgrade to succeed we encourage you to upgrade directly to MySQL Server version 8.0.24 or
newer. Otherwise, you must rebuild these tables using ALTER TABLE FORCE before upgrading.

* checkTableCommand.title
Issues reported by 'check table x for upgrade' command

* checkTableCommand.description
Issues reported by 'check table x for upgrade' command

* defaultAuthenticationPlugin.title
New default authentication plugin considerations

* defaultAuthenticationPlugin.description
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
Warning: The new default authentication plugin 'caching_sha2_password' offers more secure password
hashing than previously used 'mysql_native_password' (and consequent improved client connection
authentication). However, it also has compatibility implications that may affect existing MySQL
installations.  If your MySQL installation must serve pre-8.0 clients, you may encounter
compatibility issues after upgrading unless newly created accounts are created to use
'mysql_native_password'.

* defaultAuthenticationPluginMds.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-compatibility-issues\n
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-caching-sha2-password-replication

* changedFunctionsInGeneratedColumns.title
Indexes on functions with changed semantics

* changedFunctionsInGeneratedColumns.description
Warning: The following tables have indexes on generated columns that use functions that have had
changes in their semantics. Replication problems and index corruption may arise unless the
underlying issue is resolved before upgrade. See documentation for more information.

* changedFunctionsInGeneratedColumns.docLink
https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html#upgrade-sql-changes

* columnsWhichCannotHaveDefaults.title
Columns which cannot have default values

* columnsWhichCannotHaveDefaults.description
Error: The following columns are defined as either BLOB, TEXT, GEOMETRY or JSON and have a default
value set. These data types cannot have default values in MySQL versions prior to 8.0.13, while
starting with 8.0.13, the default value must be specified as an expression. In order to fix this
issue, please use the ALTER TABLE ... ALTER COLUMN ... DROP DEFAULT statement.

* columnsWhichCannotHaveDefaults.docLink
https://dev.mysql.com/doc/refman/8.0/en/data-type-defaults.html#data-type-defaults-explicit

* invalid57Names.title
Check for invalid table names and schema names used in 5.7

* invalid57Names.description
The following tables and/or schemas have invalid names. In order to fix them use the mysqlcheck
utility as follows:\n
\n
  $ mysqlcheck --check-upgrade --all-databases\n
  $ mysqlcheck --fix-db-names --fix-table-names --all-databases\n
\n
OR via mysql client, for eg:\n
\n
  ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;

* invalid57Names.docLink
https://dev.mysql.com/doc/refman/5.7/en/identifier-mapping.html
https://dev.mysql.com/doc/refman/5.7/en/alter-database.html
https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

* orphanedRoutines.title
Check for orphaned routines in 5.7

* orphanedRoutines.description
Error: The following routines have been orphaned. Schemas that they are referencing no longer
exists.\n
They have to be cleaned up or the upgrade will fail.

* dollarSignName.title
Check for deprecated usage of single dollar signs in object names

* dollarSignName.description
Warning: The following objects have names with deprecated usage of dollar sign ($) at the
begining of the identifier. To correct this warning, ensure, that names starting with dollar
sign, also end with it, similary to quotes ($example$). 

* indexTooLarge.title
Check for indexes that are too large to work on higher versions of MySQL Server than 5.7

* indexTooLarge.description
Error: The following indexes ware made too large for their format in an older version of MySQL
(older than 5.7.34). Normally those indexes within tables with compact or redundant row formats
shouldn't be larger than 767 bytes. To fix this problem those indexes should be dropped before
upgrading or those tables will be inaccessible.

* emptyDotTableSyntax.title
Check for deprecated '.<table>' syntax used in routines.

* emptyDotTableSyntax.description
Error: The following routines contain identifiers in deprecated identifier syntax (".<table>"),
and should be corrected before upgrade:\n

* invalidEngineForeignKey.title
Check for columns that have foreign keys pointing to tables from a different database engine.

* invalidEngineForeignKey.description
Error: The following columns have foreign keys pointing to tables from different database engines
than originating table. This is invalid and probably a mistake done when FOREIGN_KEY_CHECKS was
turned OFF:\n

* deprecatedDefaultAuth.title
Check for deprecated or invalid default authentication methods in system variables.

* deprecatedDefaultAuth.description
The following variables have problems with their set authentication method:\n

* deprecatedRouterAuthMethod.title
Check for deprecated or invalid authentication methods in use by MySQL Router internal accounts.

* deprecatedRouterAuthMethod.description
Warning: The following accounts are MySQL Router accounts that use a deprecated authentication
method.\n
Those accounts are automatically created at bootstrap time when the Router is not instructed to
use an existing account. Please upgrade MySQL Router to the latest version to ensure deprecated
authentication methods are no longer used.\n
Since version 8.0.19 it's also possible to instruct MySQL Router to use a dedicated account. That
account can be created using the AdminAPI.

* deprecatedRouterAuthMethod.docLink
https://dev.mysql.com/doc/mysql-shell/en/configuring-router-user.html\n
https://dev.mysql.com/doc/mysql-router/en/mysqlrouter.html#option_mysqlrouter_account

* deprecatedTemporalDelimiter.title
Check for deprecated temporal delimiters in table partitions.

* deprecatedTemporalDelimiter.description
Error: The following columns are referenced in partitioning function using custom temporal
delimiters.\n
Custom temporal delimiters were deprecated since 8.0.29. Partitions using them will make
partitioned tables unaccessible after upgrade.\n
These partitions need to be updated to standard temporal delimiters before the upgrade.

* deprecatedTemporalDelimiter.docLink
https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-29.html#mysqld-8-0-29-deprecation-removal
https://dev.mysql.com/doc/refman/8.2/en/datetime.html

* innodbRowFormat.title
InnoDB tables with non-default row format

* innodbRowFormat.description
The following tables use a non-default InnoDB row format


* zerofill.title
Usage of use ZEROFILL/display length type attributes

* zerofill.description
The following table columns specify a ZEROFILL/display length attributes.
Please be aware that they will be ignored in the latest MySQL version.

* authMethodUsage.title
Check for deprecated or invalid user authentication methods.

* authMethodUsage.description
Some users are using authentication methods that may be deprecated or removed,
please review the details below.\n

* authMethodUsage.description.Notice
The following users are using the '%feature_id%' authentication method which
will be deprecated as of MySQL %deprecated%.\n
Consider switching the users to a different authentication method.\n

* authMethodUsage.description.Warning
The following users are using the '%feature_id%' authentication method which
is deprecated as of MySQL %deprecated% and will be removed in a future release.\n
Consider switching the users to a different authentication method.\n

* authMethodUsage.description.Error
The following users are using the '%feature_id%' authentication method which
is removed as of MySQL %removed%.\n
The users must be deleted or re-created with a different authentication method.\n

* authMethodUsage.description.Notice.Replacement
The following users are using the '%feature_id%' authentication method which
will be deprecated as of MySQL %deprecated%.\n
Consider switching the users to a different authentication method (i.e. %replacement%).\n

* authMethodUsage.description.Warning.Replacement
The following users are using the '%feature_id%' authentication method which
is deprecated as of MySQL %deprecated% and will be removed in a future release.\n
Consider switching the users to a different authentication method (i.e. %replacement%).\n

* authMethodUsage.description.Error.Replacement
The following users are using the '%feature_id%' authentication method which
is removed as of MySQL %removed%.\n
The users must be deleted or re-created with a different authentication method (i.e. %replacement%).\n

* authMethodUsage.authentication_fido.docLink
https://dev.mysql.com/doc/refman/8.3/en/webauthn-pluggable-authentication.html

* authMethodUsage.sha256_password.docLink
https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html

* authMethodUsage.mysql_native_password.docLink
https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html

* pluginUsage.title
Check for deprecated or removed plugin usage.

* pluginUsage.description
The following plugins are ACTIVE and they have been deprecated or removed.\n

* pluginUsage.description.Notice
The '%item_id%' plugin will be deprecated as of MySQL %deprecated%.\n
Consider to stop using it.\n

* pluginUsage.description.Warning
The '%item_id%' plugin is deprecated as of MySQL %deprecated% and will be removed in a future release.\n
Consider to stop using it.\n

* pluginUsage.description.Error
The '%item_id%' plugin is removed as of MySQL %removed%.\n
It must not be used anymore.\n

* pluginUsage.description.Notice.Replacement
The '%item_id%' plugin will be deprecated as of MySQL %deprecated%.\n
Consider using %replacement% instead.\n

* pluginUsage.description.Warning.Replacement
The '%item_id%' plugin is deprecated as of MySQL %deprecated% and will be removed in a future release.\n
Consider using %replacement% instead.\n

* pluginUsage.description.Error.Replacement
The '%item_id%' plugin is removed as of MySQL %removed%.\n
It must not be used anymore, please use %replacement% instead.\n

* pluginUsage.authentication_fido.docLink
https://dev.mysql.com/doc/refman/8.3/en/webauthn-pluggable-authentication.html

* pluginUsage.keyring_file.docLink
https://dev.mysql.com/doc/refman/8.0/en/keyring-file-component.html

* pluginUsage.keyring_encrypted_file.docLink
https://dev.mysql.com/doc/refman/8.0/en/keyring-encrypted-file-component.html

* pluginUsage.keyring_oci.docLink
https://dev.mysql.com/doc/mysql-security-excerpt/8.3/en/keyring-oci-plugin.html

* sysvarAllowedValues.title
Check for allowed values in System Variables.

* sysvarAllowedValues.description
The following system variables are using values that are not allowed.\n

* sysvarAllowedValues.issue
The system variable '%variable%' is set to '%value%', allowed values include: %allowed%\n

* invalidPrivileges.title
Checks for user privileges that will be removed

* invalidPrivileges.description
Verifies for users containing grants to be removed, since privileges are removed as part of
the upgrade, raises a NOTICE to inform the user about users that will be losing invalid privileges\n

* invalidPrivileges.issue
The user %account% has the following privileges that will be removed as part of the upgrade process: %privileges%

* columnDefinition.title
Checks for errors in column definitions

* columnDefinition.description
Identifies column definitions that may not be supported in future versions of MySQL

* columnDefinition.floatAutoIncrement
The column is of type FLOAT and has the AUTO_INCREMENT flag set, this is no longer supported.

* columnDefinition.doubleAutoIncrement
The column is of type DOUBLE and has the AUTO_INCREMENT flag set, this is no longer supported.

* partitionsWithPrefixKeys.title
Checks for partitions by key using columns with prefix key indexes

* partitionsWithPrefixKeys.description
Indexes on column prefixes are not supported for key partitioning, they are ignored by the
partition function and so they are not allowed in 8.4.0.
This check identifies tables with partitions defined this way, they should be fixed
before upgrading to 8.4.0.

* partitionsWithPrefixKeys.issue
%level%: the `%schema%`.`%table%` table uses partition by KEY using the following columns with prefix index: %details%.

* partitionsWithPrefixKeys.docLink
https://dev.mysql.com/doc/refman/8.0/en/partitioning-limitations.html

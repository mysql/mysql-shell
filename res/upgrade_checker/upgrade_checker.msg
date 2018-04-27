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

* reservedKeywordsCheck.title
# Usage of db objects with names conflicting with reserved keywords in 8.0

* reservedKeywordsCheck.description
# Warning: The following objects have names that conflict with reserved keywords that are new to
# 8.0. Ensure queries sent by your applications use `quotes` when referring to them or they will
# result in errors.

* reservedKeywordsCheck.docLink

* utf8mb3Check.title
# Usage of utf8mb3 charset

* utf8mb3Check.description
# Warning: The following objects use the utf8mb3 character set. It is recommended to convert them
# to use utf8mb4 instead, for improved Unicode support.

* utf8mb3Check.docLink

* mysqlSchemaCheck.title
# Table names in the mysql schema conflicting with new tables in 8.0

* mysqlSchemaCheck.description
# Error: The following tables in mysql schema have names that will conflict with the ones
# introduced in 8.0 version. They must be renamed or removed before upgrading (use RENAME TABLE
# command). This may also entail changes to applications that use the affected tables.

* mysqlSchemaCheck.docLink

* foreignKeyLengthCheck.title
# Foreign key constraint names longer than 64 characters

* foreignKeyLengthCheck.description
# Error: The following tables must be altered to have constraint names shorter than 64 characters
# (use ALTER TABLE).

* foreignKeyLengthCheck.docLink

* maxdbFlagCheck.title
# Usage of obsolete MAXDB sql_mode flag

* maxdbFlagCheck.description
# Warning: The following DB objects have the obsolete MAXDB option persisted for sql_mode, which
# will be cleared during upgrade to 8.0. It can potentially change the datatype DATETIME into
# TIMESTAMP if it is used inside object's definition, and this in turn can change the behavior in
# case of dates earlier than 1970 or later than 2037. If this is a concern, please redefine these
# objects so that they do not rely on the MAXDB flag before running the upgrade to 8.0.

* maxdbFlagCheck.docLink

* sqlModeFlagCheck.title
# Usage of obsolete sql_mode flags

* sqlModeFlagCheck.description
# Notice: The following DB objects have obsolete options persisted for sql_mode, which will be
# cleared during upgrade to 8.0.

* sqlModeFlagCheck.docLink

* partitionedTablesInSharedTablespaceCheck.title
# Usage of partitioned tables in shared tablespaces

* partitionedTablesInSharedTablespaceCheck.description
# Error: The following tables have partitions in shared tablespaces. Before upgrading to 8.0 they
# need to be moved to file-per-table tablespace. You can do this by running query like 'ALTER TABLE
# table_name REORGANIZE PARTITION X INTO (PARTITION X VALUES LESS THAN (30)
# TABLESPACE=innodb_file_per_table);'

* partitionedTablesInSharedTablespaceCheck.docLink

* removedFunctionsCheck.title
# Usage of removed functions

* removedFunctionsCheck.description
# Error: Following DB objects make use of functions that have been removed in version 8.0. Please
# make sure to update them to use supported alternatives before upgrade.

* removedFunctionsCheck.docLink

* checkTableOutput.title
# Issues reported by 'check table x for upgrade' command

* checkTableOutput.description

* checkTableOutput.docLink


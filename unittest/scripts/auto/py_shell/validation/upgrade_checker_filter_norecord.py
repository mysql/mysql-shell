#@<OUT> WL15974-TSFR_3_3_1
Upgrade Consistency Checks

The MySQL Shell will now list checks for possible compatibility issues for
upgrade of MySQL Server...

Included:

- checkTableCommand
  Issues reported by 'check table x for upgrade' command
  Category: schema

Excluded:

- orphanedObjects
  Check for orphaned routines and events in 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- oldTemporal
  Usage of old temporal type
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- reservedKeywords
  Usage of db objects with names conflicting with new reserved keywords
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
  8.0.14, 8.0.17, 8.0.31

- utf8mb3
  Usage of utf8mb3 charset
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- mysqlSchema
  Table names in the mysql schema conflicting with new tables in the latest
  MySQL.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- nonNativePartitioning
  Partitioned tables using engines with non native partitioning
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- foreignKeyLength
  Foreign key constraint names longer than 64 characters
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- maxdbSqlModeFlags
  Usage of obsolete MAXDB sql_mode flag
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- obsoleteSqlModeFlags
  Usage of obsolete sql_mode flags
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- enumSetElementLength
  ENUM/SET column definitions containing elements longer than 255 characters
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- partitionedTablesInSharedTablespaces
  Usage of partitioned tables in shared tablespaces
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
  8.0.13

- circularDirectory
  Circular directory references in tablespace data file paths
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.17

- removedFunctions
  Usage of removed functions
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- groupbyAscSyntax
  Usage of removed GROUP BY ASC/DESC syntax
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- sysVars
  System variable check for deprecation, removal, changes in defaults values or
  invalid values.
  Category: config

- zeroDates
  Zero Date, Datetime, and Timestamp values
  Category: config
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- schemaInconsistency
  Schema inconsistencies resulting from file removal or corruption
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- ftsInTablename
  Table names containing 'FTS'
  Category: schema
  Condition: When upgrading to a version between 8.0.11 and 8.0.17 on non
  Windows platforms.

- engineMixup
  Tables recognized by InnoDB that belong to a different engine
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- oldGeometryTypes
  Spatial data columns created in MySQL 5.6
  Category: schema
  Condition: When upgrading to a version between 8.0.11 and 8.0.23

- defaultAuthenticationPlugin
  New default authentication plugin considerations
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- defaultAuthenticationPluginMds
  New default authentication plugin considerations
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- changedFunctionsInGeneratedColumns
  Indexes on functions with changed semantics
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 5.7.0,
  8.0.28

- columnsWhichCannotHaveDefaults
  Columns which cannot have default values
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.12

- invalid57Names
  Check for invalid table names and schema names used in 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- dollarSignName
  Check for deprecated usage of single dollar signs in object names
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.31

- indexTooLarge
  Check for indexes that are too large to work on higher versions of MySQL
  Server than 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- emptyDotTableSyntax
  Check for deprecated '.<table>' syntax used in routines.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- syntax
  MySQL syntax check for routine-like objects
  Category: parsing
  Condition: When the major.minor version of the source and target servers is
  different.

- invalidEngineForeignKey
  Check for columns that have foreign keys pointing to tables from a different
  database engine.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- foreignKeyReferences
  Checks for foreign keys not referencing a full unique index
  Category: schema
  Condition: When the target server is equal or above 8.4.0.

- authMethodUsage
  Check for deprecated or invalid user authentication methods.
  Category: accounts
  Condition: Target version is at least 8.0.16

- pluginUsage
  Check for deprecated or removed plugin usage.
  Category: config
  Condition: Server version is lower than 9.5.0 and the target version is at
  least 8.0.26

- deprecatedDefaultAuth
  Check for deprecated or invalid default authentication methods in system
  variables.
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
  8.1.0, 8.2.0

- deprecatedRouterAuthMethod
  Check for deprecated or invalid authentication methods in use by MySQL Router
  internal accounts.
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
  8.1.0, 8.2.0

- deprecatedTemporalDelimiter
  Check for deprecated temporal delimiters in table partitions.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.29

- columnDefinition
  Checks for errors in column definitions
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- invalidPrivileges
  Checks for user privileges that will be removed
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- partitionsWithPrefixKeys
  Checks for partitions by key using columns with prefix key indexes
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- spatialIndex
  Checks for Spatial Indexes
  Category: schema
  Condition: When the source server is between (inclusive) 8.0.3-8.0.40,
  8.1.0-8.4.3, 9.0.0-9.1.0 and the target server version is above the listed
  versions.

Included: 1
Excluded: 40


#@<OUT> WL15974-TSFR_3_3_2
Upgrade Consistency Checks

The MySQL Shell will now list checks for possible compatibility issues for
upgrade of MySQL Server...

Included:

- orphanedObjects
  Check for orphaned routines and events in 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- oldTemporal
  Usage of old temporal type
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- reservedKeywords
  Usage of db objects with names conflicting with new reserved keywords
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
  8.0.14, 8.0.17, 8.0.31

- utf8mb3
  Usage of utf8mb3 charset
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- mysqlSchema
  Table names in the mysql schema conflicting with new tables in the latest
  MySQL.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- nonNativePartitioning
  Partitioned tables using engines with non native partitioning
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- foreignKeyLength
  Foreign key constraint names longer than 64 characters
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- maxdbSqlModeFlags
  Usage of obsolete MAXDB sql_mode flag
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- obsoleteSqlModeFlags
  Usage of obsolete sql_mode flags
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- enumSetElementLength
  ENUM/SET column definitions containing elements longer than 255 characters
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- partitionedTablesInSharedTablespaces
  Usage of partitioned tables in shared tablespaces
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
  8.0.13

- circularDirectory
  Circular directory references in tablespace data file paths
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.17

- removedFunctions
  Usage of removed functions
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- groupbyAscSyntax
  Usage of removed GROUP BY ASC/DESC syntax
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- sysVars
  System variable check for deprecation, removal, changes in defaults values or
  invalid values.
  Category: config

- zeroDates
  Zero Date, Datetime, and Timestamp values
  Category: config
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- schemaInconsistency
  Schema inconsistencies resulting from file removal or corruption
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- ftsInTablename
  Table names containing 'FTS'
  Category: schema
  Condition: When upgrading to a version between 8.0.11 and 8.0.17 on non
  Windows platforms.

- engineMixup
  Tables recognized by InnoDB that belong to a different engine
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- oldGeometryTypes
  Spatial data columns created in MySQL 5.6
  Category: schema
  Condition: When upgrading to a version between 8.0.11 and 8.0.23

- checkTableCommand
  Issues reported by 'check table x for upgrade' command
  Category: schema

- defaultAuthenticationPlugin
  New default authentication plugin considerations
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- defaultAuthenticationPluginMds
  New default authentication plugin considerations
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- changedFunctionsInGeneratedColumns
  Indexes on functions with changed semantics
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 5.7.0,
  8.0.28

- columnsWhichCannotHaveDefaults
  Columns which cannot have default values
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.12

- invalid57Names
  Check for invalid table names and schema names used in 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- dollarSignName
  Check for deprecated usage of single dollar signs in object names
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.31

- indexTooLarge
  Check for indexes that are too large to work on higher versions of MySQL
  Server than 5.7
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- emptyDotTableSyntax
  Check for deprecated '.<table>' syntax used in routines.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- syntax
  MySQL syntax check for routine-like objects
  Category: parsing
  Condition: When the major.minor version of the source and target servers is
  different.

- invalidEngineForeignKey
  Check for columns that have foreign keys pointing to tables from a different
  database engine.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- foreignKeyReferences
  Checks for foreign keys not referencing a full unique index
  Category: schema
  Condition: When the target server is equal or above 8.4.0.

- authMethodUsage
  Check for deprecated or invalid user authentication methods.
  Category: accounts
  Condition: Target version is at least 8.0.16

- pluginUsage
  Check for deprecated or removed plugin usage.
  Category: config
  Condition: Server version is lower than 9.5.0 and the target version is at
  least 8.0.26

- deprecatedDefaultAuth
  Check for deprecated or invalid default authentication methods in system
  variables.
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
  8.1.0, 8.2.0

- deprecatedRouterAuthMethod
  Check for deprecated or invalid authentication methods in use by MySQL Router
  internal accounts.
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
  8.1.0, 8.2.0

- deprecatedTemporalDelimiter
  Check for deprecated temporal delimiters in table partitions.
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.0.29

- columnDefinition
  Checks for errors in column definitions
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- invalidPrivileges
  Checks for user privileges that will be removed
  Category: accounts
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- partitionsWithPrefixKeys
  Checks for partitions by key using columns with prefix key indexes
  Category: schema
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- spatialIndex
  Checks for Spatial Indexes
  Category: schema
  Condition: When the source server is between (inclusive) 8.0.3-8.0.40,
  8.1.0-8.4.3, 9.0.0-9.1.0 and the target server version is above the listed
  versions.

Excluded:
  Empty.

Included: 41


#@<OUT> warning about excluded checks
[[*]]WARNING: The following checks were excluded (per user request):
checkTableCommand: Issues reported by 'check table x for upgrade' command
dollarSignName: Check for deprecated usage of single dollar signs in object names

#@<OUT> warning about excluded checks JSON
[[*]]"excludedChecks": [
        {
            "id": "checkTableCommand",
            "title": "Issues reported by 'check table x for upgrade' command"
        },
        {
            "id": "dollarSignName",
            "title": "Check for deprecated usage of single dollar signs in object names"
        }
    ]
}

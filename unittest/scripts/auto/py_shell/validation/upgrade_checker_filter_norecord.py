#@<OUT> WL15974-TSFR_3_3_1
  Upgrade Consistency Checks

  The MySQL Shell will now list checks for possible compatibility issues for
    upgrade of MySQL Server...

Included:

- checkTableCommand
  Issues reported by 'check table x for upgrade' command

Excluded:

- oldTemporal
  Usage of old temporal type
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- routineSyntax
  MySQL syntax check for routine-like objects
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- reservedKeywords
  Usage of db objects with names conflicting with new reserved keywords
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.14, 8.0.17, 8.0.31

- utf8mb3
  Usage of utf8mb3 charset
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- mysqlSchema
  Table names in the mysql schema conflicting with new tables in the latest
    MySQL.
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- nonNativePartitioning
  Partitioned tables using engines with non native partitioning
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- foreignKeyLength
  Foreign key constraint names longer than 64 characters
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- maxdbSqlModeFlags
  Usage of obsolete MAXDB sql_mode flag
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- obsoleteSqlModeFlags
  Usage of obsolete sql_mode flags
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- enumSetElementLength
  ENUM/SET column definitions containing elements longer than 255 characters
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- partitionedTablesInSharedTablespaces
  Usage of partitioned tables in shared tablespaces
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.13

- circularDirectory
  Circular directory references in tablespace data file paths
  Condition: When the upgrade reaches any of the following versions: 8.0.17

- removedFunctions
  Usage of removed functions
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- groupbyAscSyntax
  Usage of removed GROUP BY ASC/DESC syntax
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- removedSysLogVars
  Removed system variables for error logging to the system log configuration
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- removedSysVars
  Removed system variables
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.13, 8.0.16, 8.2.0, 8.3.0, 8.4.0

- sysVarsNewDefaults
  System variables with new default values
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.4.0

- zeroDates
  Zero Date, Datetime, and Timestamp values
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- schemaInconsistency
  Schema inconsistencies resulting from file removal or corruption
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- ftsInTablename
  Table names containing 'FTS'
  Condition: When upgrading to a version between 8.0.11 and 8.0.17 on non
    Windows platforms.

- engineMixup
  Tables recognized by InnoDB that belong to a different engine
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- oldGeometryTypes
  Spatial data columns created in MySQL 5.6
  Condition: When upgrading to a version between 8.0.11 and 8.0.23

- defaultAuthenticationPlugin
  New default authentication plugin considerations
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- defaultAuthenticationPluginMds
  New default authentication plugin considerations
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- changedFunctionsInGeneratedColumns
  Indexes on functions with changed semantics
  Condition: When the upgrade reaches any of the following versions: 5.7.0,
    8.0.28

- columnsWhichCannotHaveDefaults
  Columns which cannot have default values
  Condition: When the upgrade reaches any of the following versions: 8.0.12

- invalid57Names
  Check for invalid table names and schema names used in 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- orphanedObjects
  Check for orphaned routines and events in 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- dollarSignName
  Check for deprecated usage of single dollar signs in object names
  Condition: When the upgrade reaches any of the following versions: 8.0.31

- indexTooLarge
  Check for indexes that are too large to work on higher versions of MySQL
    Server than 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- emptyDotTableSyntax
  Check for deprecated '.<table>' syntax used in routines.
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- invalidEngineForeignKey
  Check for columns that have foreign keys pointing to tables from a different
    database engine.
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- authMethodUsage
  Check for deprecated or invalid user authentication methods.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.2.0

- pluginUsage
  Check for deprecated or removed plugin usage.
  Condition: When the upgrade reaches any of the following versions: 8.0.31,
    8.0.34, 8.2.0

- deprecatedDefaultAuth
  Check for deprecated or invalid default authentication methods in system
    variables.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.1.0, 8.2.0

- deprecatedRouterAuthMethod
  Check for deprecated or invalid authentication methods in use by MySQL Router
    internal accounts.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.1.0, 8.2.0

- deprecatedTemporalDelimiter
  Check for deprecated temporal delimiters in table partitions.
  Condition: When the upgrade reaches any of the following versions: 8.0.29

- columnDefinition
  Checks for errors in column definitions
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- sysvarAllowedValues
  Check for allowed values in System Variables.
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- invalidPrivileges
  Checks for user privileges that will be removed
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- partitionsWithPrefixKeys
  Checks for partitions by key using columns with prefix key indexes
  Condition: When the upgrade reaches any of the following versions: 8.4.0

Included: 1
Excluded: 41


#@<OUT> WL15974-TSFR_3_3_2
  Upgrade Consistency Checks

  The MySQL Shell will now list checks for possible compatibility issues for
    upgrade of MySQL Server...

Included:

- oldTemporal
  Usage of old temporal type
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- routineSyntax
  MySQL syntax check for routine-like objects
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- reservedKeywords
  Usage of db objects with names conflicting with new reserved keywords
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.14, 8.0.17, 8.0.31

- utf8mb3
  Usage of utf8mb3 charset
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- mysqlSchema
  Table names in the mysql schema conflicting with new tables in the latest
    MySQL.
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- nonNativePartitioning
  Partitioned tables using engines with non native partitioning
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- foreignKeyLength
  Foreign key constraint names longer than 64 characters
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- maxdbSqlModeFlags
  Usage of obsolete MAXDB sql_mode flag
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- obsoleteSqlModeFlags
  Usage of obsolete sql_mode flags
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- enumSetElementLength
  ENUM/SET column definitions containing elements longer than 255 characters
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- partitionedTablesInSharedTablespaces
  Usage of partitioned tables in shared tablespaces
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.13

- circularDirectory
  Circular directory references in tablespace data file paths
  Condition: When the upgrade reaches any of the following versions: 8.0.17

- removedFunctions
  Usage of removed functions
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- groupbyAscSyntax
  Usage of removed GROUP BY ASC/DESC syntax
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- removedSysLogVars
  Removed system variables for error logging to the system log configuration
  Condition: When the upgrade reaches any of the following versions: 8.0.13

- removedSysVars
  Removed system variables
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.0.13, 8.0.16, 8.2.0, 8.3.0, 8.4.0

- sysVarsNewDefaults
  System variables with new default values
  Condition: When the upgrade reaches any of the following versions: 8.0.11,
    8.4.0

- zeroDates
  Zero Date, Datetime, and Timestamp values
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- schemaInconsistency
  Schema inconsistencies resulting from file removal or corruption
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- ftsInTablename
  Table names containing 'FTS'
  Condition: When upgrading to a version between 8.0.11 and 8.0.17 on non
    Windows platforms.

- engineMixup
  Tables recognized by InnoDB that belong to a different engine
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- oldGeometryTypes
  Spatial data columns created in MySQL 5.6
  Condition: When upgrading to a version between 8.0.11 and 8.0.23

- checkTableCommand
  Issues reported by 'check table x for upgrade' command

- defaultAuthenticationPlugin
  New default authentication plugin considerations
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- defaultAuthenticationPluginMds
  New default authentication plugin considerations
  Condition: When the upgrade reaches any of the following versions: 8.0.11

- changedFunctionsInGeneratedColumns
  Indexes on functions with changed semantics
  Condition: When the upgrade reaches any of the following versions: 5.7.0,
    8.0.28

- columnsWhichCannotHaveDefaults
  Columns which cannot have default values
  Condition: When the upgrade reaches any of the following versions: 8.0.12

- invalid57Names
  Check for invalid table names and schema names used in 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- orphanedObjects
  Check for orphaned routines and events in 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- dollarSignName
  Check for deprecated usage of single dollar signs in object names
  Condition: When the upgrade reaches any of the following versions: 8.0.31

- indexTooLarge
  Check for indexes that are too large to work on higher versions of MySQL
    Server than 5.7
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- emptyDotTableSyntax
  Check for deprecated '.<table>' syntax used in routines.
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- invalidEngineForeignKey
  Check for columns that have foreign keys pointing to tables from a different
    database engine.
  Condition: When the upgrade reaches any of the following versions: 8.0.0

- authMethodUsage
  Check for deprecated or invalid user authentication methods.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.2.0

- pluginUsage
  Check for deprecated or removed plugin usage.
  Condition: When the upgrade reaches any of the following versions: 8.0.31,
    8.0.34, 8.2.0

- deprecatedDefaultAuth
  Check for deprecated or invalid default authentication methods in system
    variables.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.1.0, 8.2.0

- deprecatedRouterAuthMethod
  Check for deprecated or invalid authentication methods in use by MySQL Router
    internal accounts.
  Condition: When the upgrade reaches any of the following versions: 8.0.0,
    8.1.0, 8.2.0

- deprecatedTemporalDelimiter
  Check for deprecated temporal delimiters in table partitions.
  Condition: When the upgrade reaches any of the following versions: 8.0.29

- columnDefinition
  Checks for errors in column definitions
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- sysvarAllowedValues
  Check for allowed values in System Variables.
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- invalidPrivileges
  Checks for user privileges that will be removed
  Condition: When the upgrade reaches any of the following versions: 8.4.0

- partitionsWithPrefixKeys
  Checks for partitions by key using columns with prefix key indexes
  Condition: When the upgrade reaches any of the following versions: 8.4.0

Excluded:
  Empty.

Included: 42


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

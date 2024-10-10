//@<OUT> CLI shell Global --help
The following objects provide command line operations:

   cluster
      Represents an InnoDB Cluster.

   clusterset
      Represents an InnoDB ClusterSet.

   dba
      InnoDB Cluster, ReplicaSet, and ClusterSet management functions.

   rs
      Represents an InnoDB ReplicaSet.

   shell
      Gives access to general purpose functions and properties.

   util
      Global object that groups miscellaneous tools like upgrade checker and
      JSON import.

//@<OUT> CLI shell --help
The following object provides command line operations at 'shell':

   options
      Gives access to options impacting shell behavior.

The following operations are available at 'shell':

   delete-all-credentials
      Deletes all credentials managed by the configured helper.

   delete-credential
      Deletes credential for the given URL using the configured helper.

   list-credential-helpers
      Returns a list of strings, where each string is a name of a helper
      available on the current platform.

   list-credentials
      Retrieves a list of all URLs stored by the configured helper.

   list-sql-handlers
      Lists the name and description of any registered SQL handlers.

   status
      Shows connection status info for the shell.

   store-credential
      Stores given credential using the configured helper.

//@<OUT> CLI shell options --help
The following operations are available at 'shell options':

   set-persist
      Sets value of an option and stores it in the configuration file.

   unset-persist
      Resets value of an option to default and removes it from the
      configuration file.

//@<OUT> CLI util --help
The following object provides command line operations at 'util':

   debug
      Debugging and diagnostic utilities.

The following operations are available at 'util':

   check-for-server-upgrade
      Performs series of tests on specified MySQL server to check if the
      upgrade process will succeed.

   copy-instance
      Copies a source instance to the target instance. Requires an open global
      Shell session to the source instance, if there is none, an exception is
      raised.

   copy-schemas
      Copies schemas from the source instance to the target instance. Requires
      an open global Shell session to the source instance, if there is none, an
      exception is raised.

   copy-tables
      Copies tables and views from schema in the source instance to the target
      instance. Requires an open global Shell session to the source instance,
      if there is none, an exception is raised.

   dump-binlogs
      Dumps binary logs generated since a specific point in time to the given
      local or remote directory.

   dump-instance
      Dumps the whole database to files in the output directory.

   dump-schemas
      Dumps the specified schemas to the files in the output directory.

   dump-tables
      Dumps the specified tables or views from the given schema to the files in
      the target directory.

   export-table
      Exports the specified table to the data dump file.

   import-json
      Import JSON documents from file to collection or table in MySQL Server
      using X Protocol session.

   import-table
      Import table dump stored in files to target table using LOAD DATA LOCAL
      INFILE calls in parallel connections.

   load-binlogs
      Loads binary log dumps created by MySQL Shell from a local or remote
      directory.

   load-dump
      Loads database dumps created by MySQL Shell.

//@<OUT> CLI --help Unexisting Objects
ERROR: There is no object registered under name 'test'
ERROR: There is no object registered under name 'test.wex'

//@<OUT> CLI shell delete-all-credentials --help
NAME
      delete-all-credentials - Deletes all credentials managed by the
                               configured helper.

SYNTAX
      shell delete-all-credentials

//@<OUT> CLI shell delete-credential --help
NAME
      delete-credential - Deletes credential for the given URL using the
                          configured helper.

SYNTAX
      shell delete-credential <url>

WHERE
      url: URL of the server to delete.

//@<OUT> CLI shell list-credential-helpers --help
NAME
      list-credential-helpers - Returns a list of strings, where each string is
                                a name of a helper available on the current
                                platform.

SYNTAX
      shell list-credential-helpers

RETURNS
      A list of string with names of available credential helpers.

//@<OUT> CLI shell list-credentials --help
NAME
      list-credentials - Retrieves a list of all URLs stored by the configured
                         helper.

SYNTAX
      shell list-credentials

RETURNS
      A list of URLs stored by the configured credential helper.

//@<OUT> CLI shell status --help
NAME
      status - Shows connection status info for the shell.

SYNTAX
      shell status

//@<OUT> CLI shell store-credential --help
NAME
      store-credential - Stores given credential using the configured helper.

SYNTAX
      shell store-credential <url> [<password>]

WHERE
      url: URL of the server for the password to be stored.
      password: Password for the given URL.

//@<OUT> CLI shell options set-persist --help
NAME
      set-persist - Sets value of an option and stores it in the configuration
                    file.

SYNTAX
      shell options set-persist <option_name> <value>

WHERE
      optionName: name of the option to set.
      value: new value for the option.

//@<OUT> CLI shell options unset-persist --help
NAME
      unset-persist - Resets value of an option to default and removes it from
                      the configuration file.

SYNTAX
      shell options unset-persist <option_name>

WHERE
      optionName: name of the option to reset.

//@<OUT> CLI util check-for-server-upgrade --help
NAME
      check-for-server-upgrade - Performs series of tests on specified MySQL
                                 server to check if the upgrade process will
                                 succeed.

SYNTAX
      util check-for-server-upgrade [<connectionData>] [<options>]

WHERE
      connectionData: The connection data to server to be checked

OPTIONS
--configPath=<str>
            Full path to MySQL server configuration file.

--exclude=<str list>
            Comma separated list containing the check identifiers to be
            excluded from the operation.

--include=<str list>
            Comma separated list containing the check identifiers to be
            included in the operation.

--list=<bool>
            Bool value to indicate the operation should only list the checks.

--outputFormat=<str>
            Value can be either TEXT (default) or JSON.

--targetVersion=<str>
            Version to which upgrade will be checked.

//@<OUT> CLI util copy-instance --help
NAME
      copy-instance - Copies a source instance to the target instance. Requires
                      an open global Shell session to the source instance, if
                      there is none, an exception is raised.

SYNTAX
      util copy-instance <connectionData> [<options>]

WHERE
      connectionData: Specifies the connection information required to
                      establish a connection to the target instance.

OPTIONS
--analyzeTables=<str>
            "off", "on", "histogram" (default: off) - If 'on', executes ANALYZE
            TABLE for all tables, once copied. If set to 'histogram', only
            tables that have histogram information stored in the copy will be
            analyzed.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be copied in each chunk,
            enables chunking. Default: "64M".

--checksum=<bool>
            Compute checksums of the data and verify tables in the target
            instance against these checksums. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            copying the DDL. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--consistent=<bool>
            Enable or disable consistent data copies. When enabled, produces a
            transactionally consistent copy at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only copy data from the database. Default: false.

--ddlOnly=<bool>
            Only copy Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the copy. Default: "utf8mb4".

--deferTableIndexes=<str>
            "off", "fulltext", "all" (default: fulltext) - If "all", creation
            of "all" indexes except PRIMARY is deferred until after table data
            is copied, which in many cases can reduce load times. If
            "fulltext", only full-text indexes will be deferred.

--dropExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Drops existing user accounts and objects
            (excluding schemas) before copying them. Mutually exclusive with
            ignoreExistingObjects. Default: false.

--dryRun=<bool>
            Simulates a copy and prints everything that would be performed,
            without actually doing so. If target is MySQL HeatWave Service,
            also checks for compatibility issues. Default: false.

--events=<bool>
            Include events from each copied schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the copy in the format of
            schema.event. Default: empty.

--excludeRoutines=<str list>
            List of routines to be excluded from the copy in the format of
            schema.routine. Default: empty.

--excludeSchemas=<str list>
            List of schemas to be excluded from the copy. Default: empty.

--excludeTables=<str list>
            List of tables or views to be excluded from the copy in the format
            of schema.table. Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--excludeUsers=<str list>
            Skip copying the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are excluded. Default: not set.

--handleGrantErrors=<str>
            "abort", "drop_account", "ignore" (default: abort) - Specifies
            action to be performed in case of errors related to the
            GRANT/REVOKE statements, "abort": throws an error and aborts the
            copy, "drop_account": deletes the problematic account and
            continues, "ignore": ignores the error and continues copying the
            account.

--ignoreExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Ignores existing user accounts and objects.
            Mutually exclusive with dropExistingObjects. Default: false.

--ignoreVersion=<bool>
            Perform the copy even if the major version number of the server
            where it was created is different from where it will be loaded.
            Default: false.

--includeEvents=<str list>
            List of events to be included in the copy in the format of
            schema.event. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the copy in the format of
            schema.routine. Default: empty.

--includeSchemas=<str list>
            List of schemas to be included in the copy. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the copy in the format of
            schema.table. Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--includeUsers=<str list>
            Copy only the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are included. By default, all
            users are included. Default: not set.

--loadIndexes=<bool>
            Use together with deferTableIndexes to control whether secondary
            indexes should be recreated at the end of the copy. Default: true.

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be copied per single
            LOAD DATA statement. Supports unit suffixes: k (kilobytes), M
            (Megabytes), G (Gigabytes). Minimum value: 4096. Default: the value
            of bytesPerChunk.

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data copy to just
            the specified partitions. Default: not set.

--routines=<bool>
            Include functions and stored procedures for each copied schema.
            Default: true.

--schema=<str>
            Copy the data into the given schema. This option can only be used
            when copying just one schema. Default: not set.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            copy data. Default: [].

--showProgress=<bool>
            Enable or disable copy progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipBinlog=<bool>
            Disables the binary log for the MySQL sessions used by the loader
            (set sql_log_bin=0). Default: false.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent copies and i.e. backup lock cannot not be acquired.
            Default: false.

--threads=<uint>
            Use N threads to read the data from the source server and
            additional N threads to write the data to the target server.
            Default: 4.

--triggers=<bool>
            Include triggers for each copied table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--updateGtidSet=<str>
            "off", "replace", "append" (default: off) - if set to a value other
            than 'off' updates GTID_PURGED by either replacing its contents or
            appending to it the gtid set present in the copy.

--users=<bool>
            Include users, roles and grants in the copy. Default: true.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            copied. Default: not set.

//@<OUT> CLI util copy-schemas --help
NAME
      copy-schemas - Copies schemas from the source instance to the target
                     instance. Requires an open global Shell session to the
                     source instance, if there is none, an exception is raised.

SYNTAX
      util copy-schemas <schemas> <connectionData> [<options>]

WHERE
      schemas: List of strings with names of schemas to be copied.
      connectionData: Specifies the connection information required to
                      establish a connection to the target instance.

OPTIONS
--analyzeTables=<str>
            "off", "on", "histogram" (default: off) - If 'on', executes ANALYZE
            TABLE for all tables, once copied. If set to 'histogram', only
            tables that have histogram information stored in the copy will be
            analyzed.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be copied in each chunk,
            enables chunking. Default: "64M".

--checksum=<bool>
            Compute checksums of the data and verify tables in the target
            instance against these checksums. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            copying the DDL. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--consistent=<bool>
            Enable or disable consistent data copies. When enabled, produces a
            transactionally consistent copy at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only copy data from the database. Default: false.

--ddlOnly=<bool>
            Only copy Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the copy. Default: "utf8mb4".

--deferTableIndexes=<str>
            "off", "fulltext", "all" (default: fulltext) - If "all", creation
            of "all" indexes except PRIMARY is deferred until after table data
            is copied, which in many cases can reduce load times. If
            "fulltext", only full-text indexes will be deferred.

--dropExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Drops existing user accounts and objects
            (excluding schemas) before copying them. Mutually exclusive with
            ignoreExistingObjects. Default: false.

--dryRun=<bool>
            Simulates a copy and prints everything that would be performed,
            without actually doing so. If target is MySQL HeatWave Service,
            also checks for compatibility issues. Default: false.

--events=<bool>
            Include events from each copied schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the copy in the format of
            schema.event. Default: empty.

--excludeRoutines=<str list>
            List of routines to be excluded from the copy in the format of
            schema.routine. Default: empty.

--excludeTables=<str list>
            List of tables or views to be excluded from the copy in the format
            of schema.table. Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--handleGrantErrors=<str>
            "abort", "drop_account", "ignore" (default: abort) - Specifies
            action to be performed in case of errors related to the
            GRANT/REVOKE statements, "abort": throws an error and aborts the
            copy, "drop_account": deletes the problematic account and
            continues, "ignore": ignores the error and continues copying the
            account.

--ignoreExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Ignores existing user accounts and objects.
            Mutually exclusive with dropExistingObjects. Default: false.

--ignoreVersion=<bool>
            Perform the copy even if the major version number of the server
            where it was created is different from where it will be loaded.
            Default: false.

--includeEvents=<str list>
            List of events to be included in the copy in the format of
            schema.event. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the copy in the format of
            schema.routine. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the copy in the format of
            schema.table. Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--loadIndexes=<bool>
            Use together with deferTableIndexes to control whether secondary
            indexes should be recreated at the end of the copy. Default: true.

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be copied per single
            LOAD DATA statement. Supports unit suffixes: k (kilobytes), M
            (Megabytes), G (Gigabytes). Minimum value: 4096. Default: the value
            of bytesPerChunk.

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data copy to just
            the specified partitions. Default: not set.

--routines=<bool>
            Include functions and stored procedures for each copied schema.
            Default: true.

--schema=<str>
            Copy the data into the given schema. This option can only be used
            when copying just one schema. Default: not set.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            copy data. Default: [].

--showProgress=<bool>
            Enable or disable copy progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipBinlog=<bool>
            Disables the binary log for the MySQL sessions used by the loader
            (set sql_log_bin=0). Default: false.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent copies and i.e. backup lock cannot not be acquired.
            Default: false.

--threads=<uint>
            Use N threads to read the data from the source server and
            additional N threads to write the data to the target server.
            Default: 4.

--triggers=<bool>
            Include triggers for each copied table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--updateGtidSet=<str>
            "off", "replace", "append" (default: off) - if set to a value other
            than 'off' updates GTID_PURGED by either replacing its contents or
            appending to it the gtid set present in the copy.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            copied. Default: not set.

//@<OUT> CLI util copy-tables --help
NAME
      copy-tables - Copies tables and views from schema in the source instance
                    to the target instance. Requires an open global Shell
                    session to the source instance, if there is none, an
                    exception is raised.

SYNTAX
      util copy-tables <schema> <tables> <connectionData> [<options>]

WHERE
      schema: Name of the schema that contains tables and views to be copied.
      tables: List of strings with names of tables and views to be copied.
      connectionData: Specifies the connection information required to
                      establish a connection to the target instance.

OPTIONS
--all=<bool>
            Copy all views and tables from the specified schema, requires the
            tables argument to be an empty list. Default: false.

--analyzeTables=<str>
            "off", "on", "histogram" (default: off) - If 'on', executes ANALYZE
            TABLE for all tables, once copied. If set to 'histogram', only
            tables that have histogram information stored in the copy will be
            analyzed.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be copied in each chunk,
            enables chunking. Default: "64M".

--checksum=<bool>
            Compute checksums of the data and verify tables in the target
            instance against these checksums. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            copying the DDL. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--consistent=<bool>
            Enable or disable consistent data copies. When enabled, produces a
            transactionally consistent copy at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only copy data from the database. Default: false.

--ddlOnly=<bool>
            Only copy Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the copy. Default: "utf8mb4".

--deferTableIndexes=<str>
            "off", "fulltext", "all" (default: fulltext) - If "all", creation
            of "all" indexes except PRIMARY is deferred until after table data
            is copied, which in many cases can reduce load times. If
            "fulltext", only full-text indexes will be deferred.

--dropExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Drops existing user accounts and objects
            (excluding schemas) before copying them. Mutually exclusive with
            ignoreExistingObjects. Default: false.

--dryRun=<bool>
            Simulates a copy and prints everything that would be performed,
            without actually doing so. If target is MySQL HeatWave Service,
            also checks for compatibility issues. Default: false.

--excludeTriggers=<str list>
            List of triggers to be excluded from the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--handleGrantErrors=<str>
            "abort", "drop_account", "ignore" (default: abort) - Specifies
            action to be performed in case of errors related to the
            GRANT/REVOKE statements, "abort": throws an error and aborts the
            copy, "drop_account": deletes the problematic account and
            continues, "ignore": ignores the error and continues copying the
            account.

--ignoreExistingObjects=<bool>
            Perform the copy even if it contains objects that already exist in
            the target database. Ignores existing user accounts and objects.
            Mutually exclusive with dropExistingObjects. Default: false.

--ignoreVersion=<bool>
            Perform the copy even if the major version number of the server
            where it was created is different from where it will be loaded.
            Default: false.

--includeTriggers=<str list>
            List of triggers to be included in the copy in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--loadIndexes=<bool>
            Use together with deferTableIndexes to control whether secondary
            indexes should be recreated at the end of the copy. Default: true.

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be copied per single
            LOAD DATA statement. Supports unit suffixes: k (kilobytes), M
            (Megabytes), G (Gigabytes). Minimum value: 4096. Default: the value
            of bytesPerChunk.

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data copy to just
            the specified partitions. Default: not set.

--schema=<str>
            Copy the data into the given schema. This option can only be used
            when copying just one schema. Default: not set.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            copy data. Default: [].

--showProgress=<bool>
            Enable or disable copy progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipBinlog=<bool>
            Disables the binary log for the MySQL sessions used by the loader
            (set sql_log_bin=0). Default: false.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent copies and i.e. backup lock cannot not be acquired.
            Default: false.

--threads=<uint>
            Use N threads to read the data from the source server and
            additional N threads to write the data to the target server.
            Default: 4.

--triggers=<bool>
            Include triggers for each copied table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--updateGtidSet=<str>
            "off", "replace", "append" (default: off) - if set to a value other
            than 'off' updates GTID_PURGED by either replacing its contents or
            appending to it the gtid set present in the copy.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            copied. Default: not set.

//@<OUT> CLI util dump-binlogs --help
NAME
      dump-binlogs - Dumps binary logs generated since a specific point in time
                     to the given local or remote directory.

SYNTAX
      util dump-binlogs <outputUrl> [<options>]

WHERE
      outputUrl: Target directory to store the dump files.

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--compression=<str>
            Compression used when writing the binary log dump files, one of:
            "none", "gzip", "zstd". Compression level may be specified as
            "gzip;level=8" or "zstd;level=8". Default: "zstd;level=1".

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. Default: false.

--ignoreDdlChanges=<bool>
            Proceeds with the dump even if the since option points to a dump
            created by util.dumpInstance() which contains modified DDL.
            Default: false.

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--since=<str>
            URL to a local or remote directory containing dump created either
            by util.dumpInstance() or util.dumpBinlogs(). Binary logs since the
            specified dump are going to be dumped. Default: not set.

--startFrom=<str>
            Specifies the name of the binary log file and its position to start
            the dump from. Accepted format:
            <binary-log-file>[:<binary-log-file-position>]. Default: not set.

--threads=<uint>
            unsigned int. Use N threads to dump the data from the instance.
            Default: 4.

//@<OUT> CLI util dump-instance --help
NAME
      dump-instance - Dumps the whole database to files in the output
                      directory.

SYNTAX
      util dump-instance <outputUrl> [<options>]

WHERE
      outputUrl: Target directory to store the dump files.

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--checksum=<bool>
            Compute and include checksum of the dumped data. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Compression level may be specified as
            "gzip;level=8" or "zstd;level=8". Default: "zstd;level=1".

--consistent=<bool>
            Enable or disable consistent data dumps. When enabled, produces a
            transactionally consistent dump at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsEscapedBy,
            fieldsOptionallyEnclosed and linesTerminatedBy options. Must be one
            of the following values: default, csv, tsv or csv-unix. Default:
            "default".

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. If ocimds is enabled, also checks for compatibility
            issues with MySQL HeatWave Service. Default: false.

--events=<bool>
            Include events from each dumped schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the dump in the format of
            schema.event. Default: empty.

--excludeRoutines=<str list>
            List of routines to be excluded from the dump in the format of
            schema.routine. Default: empty.

--excludeSchemas=<str list>
            List of schemas to be excluded from the dump. Default: empty.

--excludeTables=<str list>
            List of tables or views to be excluded from the dump in the format
            of schema.table. Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--excludeUsers=<str list>
            Skip dumping the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are excluded. Default: not set.

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: ''.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: '\'.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: "\t".

--includeEvents=<str list>
            List of events to be included in the dump in the format of
            schema.event. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the dump in the format of
            schema.routine. Default: empty.

--includeSchemas=<str list>
            List of schemas to be included in the dump. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the dump in the format of
            schema.table. Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--includeUsers=<str list>
            Dump only the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are included. By default, all
            users are included. Default: not set.

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. See Section 13.2.10.1, "SELECT ... INTO
            Statement". Default: "\n".

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ocimds=<bool>
            Enable checks for compatibility with MySQL HeatWave Service.
            Default: false.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data export to
            just the specified partitions. Default: not set.

--routines=<bool>
            Include functions and stored procedures for each dumped schema.
            Default: true.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent dumps and i.e. backup lock cannot not be acquired.
            Default: false.

--skipUpgradeChecks=<bool>
            Do not execute the upgrade check utility. Compatibility issues
            related to MySQL version upgrades will not be checked. Use this
            option only when executing the Upgrade Checker separately. Default:
            false.

--targetVersion=<str>
            Specifies version of the destination MySQL server. Default: current
            version of Shell.

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--users=<bool>
            Include users, roles and grants in the dump file. Default: true.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            exported. Default: not set.

//@<OUT> CLI util dump-schemas --help
NAME
      dump-schemas - Dumps the specified schemas to the files in the output
                     directory.

SYNTAX
      util dump-schemas <schemas> --outputUrl=<str> [<options>]

WHERE
      schemas: List of schemas to be dumped.

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--checksum=<bool>
            Compute and include checksum of the dumped data. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Compression level may be specified as
            "gzip;level=8" or "zstd;level=8". Default: "zstd;level=1".

--consistent=<bool>
            Enable or disable consistent data dumps. When enabled, produces a
            transactionally consistent dump at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsEscapedBy,
            fieldsOptionallyEnclosed and linesTerminatedBy options. Must be one
            of the following values: default, csv, tsv or csv-unix. Default:
            "default".

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. If ocimds is enabled, also checks for compatibility
            issues with MySQL HeatWave Service. Default: false.

--events=<bool>
            Include events from each dumped schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the dump in the format of
            schema.event. Default: empty.

--excludeRoutines=<str list>
            List of routines to be excluded from the dump in the format of
            schema.routine. Default: empty.

--excludeTables=<str list>
            List of tables or views to be excluded from the dump in the format
            of schema.table. Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: ''.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: '\'.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: "\t".

--includeEvents=<str list>
            List of events to be included in the dump in the format of
            schema.event. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the dump in the format of
            schema.routine. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the dump in the format of
            schema.table. Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. See Section 13.2.10.1, "SELECT ... INTO
            Statement". Default: "\n".

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ocimds=<bool>
            Enable checks for compatibility with MySQL HeatWave Service.
            Default: false.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--outputUrl=<str>
            Target directory to store the dump files.

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data export to
            just the specified partitions. Default: not set.

--routines=<bool>
            Include functions and stored procedures for each dumped schema.
            Default: true.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent dumps and i.e. backup lock cannot not be acquired.
            Default: false.

--skipUpgradeChecks=<bool>
            Do not execute the upgrade check utility. Compatibility issues
            related to MySQL version upgrades will not be checked. Use this
            option only when executing the Upgrade Checker separately. Default:
            false.

--targetVersion=<str>
            Specifies version of the destination MySQL server. Default: current
            version of Shell.

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            exported. Default: not set.

//@<OUT> CLI util dump-tables --help
NAME
      dump-tables - Dumps the specified tables or views from the given schema
                    to the files in the target directory.

SYNTAX
      util dump-tables <schema> <tables> --outputUrl=<str> [<options>]

WHERE
      schema: Name of the schema that contains tables/views to be dumped.
      tables: List of tables/views to be dumped.

OPTIONS
--all=<bool>
            Dump all views and tables from the specified schema. Default:
            false.

--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--checksum=<bool>
            Compute and include checksum of the dumped data. Default: false.

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--compatibility=<str list>
            Apply MySQL HeatWave Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "force_non_standard_fks", "ignore_missing_pks",
            "ignore_wildcard_grants", "skip_invalid_accounts",
            "strip_definers", "strip_invalid_grants",
            "strip_restricted_grants", "strip_tablespaces",
            "unescape_wildcard_grants". Default: empty.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Compression level may be specified as
            "gzip;level=8" or "zstd;level=8". Default: "zstd;level=1".

--consistent=<bool>
            Enable or disable consistent data dumps. When enabled, produces a
            transactionally consistent dump at a specific point in time.
            Default: true.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsEscapedBy,
            fieldsOptionallyEnclosed and linesTerminatedBy options. Must be one
            of the following values: default, csv, tsv or csv-unix. Default:
            "default".

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. If ocimds is enabled, also checks for compatibility
            issues with MySQL HeatWave Service. Default: false.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: ''.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: '\'.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: "\t".

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. See Section 13.2.10.1, "SELECT ... INTO
            Statement". Default: "\n".

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ocimds=<bool>
            Enable checks for compatibility with MySQL HeatWave Service.
            Default: false.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--outputUrl=<str>
            Target directory to store the dump files.

--partitions=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a list of valid partition names used to limit the data export to
            just the specified partitions. Default: not set.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--skipConsistencyChecks=<bool>
            Skips additional consistency checks which are executed when running
            consistent dumps and i.e. backup lock cannot not be acquired.
            Default: false.

--skipUpgradeChecks=<bool>
            Do not execute the upgrade check utility. Compatibility issues
            related to MySQL version upgrades will not be checked. Use this
            option only when executing the Upgrade Checker separately. Default:
            false.

--targetVersion=<str>
            Specifies version of the destination MySQL server. Default: current
            version of Shell.

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--where=<key>[:<type>]=<value>
            A key-value pair of a table name in the format of schema.table and
            a valid SQL condition expression used to filter the data being
            exported. Default: not set.

//@<OUT> CLI util export-table --help
NAME
      export-table - Exports the specified table to the data dump file.

SYNTAX
      util export-table <table> <outputUrl> [<options>]

WHERE
      table: Name of the table to be exported.
      outputUrl: Target file to store the data.

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Compression level may be specified as
            "gzip;level=8" or "zstd;level=8". Default: "none".

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsEscapedBy,
            fieldsOptionallyEnclosed and linesTerminatedBy options. Must be one
            of the following values: default, csv, tsv or csv-unix. Default:
            "default".

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: ''.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: '\'.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. Default: "\t".

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            SELECT ... INTO OUTFILE. See Section 13.2.10.1, "SELECT ... INTO
            Statement". Default: "\n".

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--partitions=<str list>
            A list of valid partition names used to limit the data export to
            just the specified partitions. Default: not set.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--where=<str>
            A valid SQL condition expression used to filter the data being
            exported. Default: not set.

//@<OUT> CLI util import-json --help
NAME
      import-json - Import JSON documents from file to collection or table in
                    MySQL Server using X Protocol session.

SYNTAX
      util import-json <path> [<options>]

WHERE
      file: Path to JSON documents file

OPTIONS
--collection=<str>
            Name of collection where the data will be imported.

--convertBsonOid=<bool>
            Enables conversion of the BSON ObjectId values. Default: the value
            of convertBsonTypes.

--convertBsonTypes=<bool>
            Enables the BSON data type conversion. Default: false.

--decimalAsDouble=<bool>
            Causes BSON Decimal values to be imported as double values.

--extractOidTime=<str>
            Creates a new field based on the ObjectID timestamp. Only valid if
            convertBsonOid is enabled. Default: empty.

--ignoreBinary=<bool>
            Disables conversion of BSON BinData values.

--ignoreDate=<bool>
            Disables conversion of BSON Date values.

--ignoreRegex=<bool>
            Disables conversion of BSON Regex values.

--ignoreRegexOptions=<bool>
            Causes regex options to be ignored when processing a Regex BSON
            value. This option is only valid if ignoreRegex is disabled.

--ignoreTimestamp=<bool>
            Disables conversion of BSON Timestamp values.

--schema=<str>
            Name of target schema.

--table=<str>
            Name of table where the data will be imported.

--tableColumn=<str>
            Name of column in target table where the imported JSON documents
            will be stored. Default: "doc".

//@<OUT> CLI util import-table --help
NAME
      import-table - Import table dump stored in files to target table using
                     LOAD DATA LOCAL INFILE calls in parallel connections.

SYNTAX
      util import-table <files> [<options>]

WHERE
      urls: URL or list of URLs to files with user data. URL can contain a glob
            pattern with wildcard '*' and/or '?'. All selected files must be
            chunks of the same target table.

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--bytesPerChunk=<str>
            Send bytesPerChunk (+ bytes to end of the row) in single LOAD DATA
            call. Unit suffixes, k - for Kilobytes (n * 1'000 bytes), M - for
            Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n *
            1'000'000'000 bytes), bytesPerChunk="2k" - ~2 kilobyte data chunk
            will send to the MySQL Server. Not available for multiple files
            import. Default: minimum: "131072", default: "50M".

--characterSet=<str>
            Interpret the information in the input file using this character
            set encoding. characterSet set to "binary" specifies "no
            conversion". If not set, the server will use the character set
            indicated by the character_set_database system variable to
            interpret the information in the file. Default: not set.

--columns[:<type>]=<value>
            Array of strings and/or integers (default: empty array) - This
            option takes an array of column names as its value. The order of
            the column names indicates how to match data file columns with
            table columns. Use non-negative integer `i` to capture column value
            into user variable @i. With user variables, the decodeColumns
            option enables you to perform preprocessing transformations on
            their values before assigning the result to columns.

--decodeColumns=<key>[:<type>]=<value>
            A map between columns names and SQL expressions to be applied on
            the loaded data. Column value captured in 'columns' by integer is
            available as user variable '@i', where `i` is that integer.
            Requires 'columns' to be set. Default: not set.

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
            fieldsEscapedBy and linesTerminatedBy options. Must be one of the
            following values: default, csv, tsv, json or csv-unix. Default:
            "default".

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: ''.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: '\'.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: "\t".

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. For example, to import Windows files that have
            lines terminated with carriage return/linefeed pairs, use
            --lines-terminated-by="\r\n". (You might have to double the
            backslashes, depending on the escaping conventions of your command
            interpreter.) See Section 13.2.7, "LOAD DATA INFILE Syntax".
            Default: "\n".

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be loaded from a
            dump data file per single LOAD DATA statement. If a content size of
            data file is bigger than this option value, then multiple LOAD DATA
            statements will be executed per single file. If this option is not
            specified explicitly, dump data file sub-chunking will be disabled.
            Use this option with value less or equal to global variable
            'max_binlog_cache_size' to mitigate "MySQL Error 1197 (HY000):
            Multi-statement transaction required more than
            'max_binlog_cache_size' bytes of storage". Unit suffixes: k
            (Kilobytes), M (Megabytes), G (Gigabytes). Minimum value: 4096.
            Default: empty.

--maxRate=<str>
            Limit data send throughput to maxRate in bytes per second per
            thread. maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n
            * 1'000 bytes), M - for Megabytes (n * 1'000'000 bytes), G - for
            Gigabytes (n * 1'000'000'000 bytes), maxRate="2k" - limit to 2
            kilobytes per second. Default: "0".

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--replaceDuplicates=<bool>
            If true, input rows that have the same value for a primary key or
            unique index as an existing row will be replaced, otherwise input
            rows will be skipped. Default: false.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--schema=<str>
            Name of target schema. Default: current Shell active schema.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            load data. Default: [].

--showProgress=<bool>
            Enable or disable import progress information. Default: true if
            stdout is a tty, false otherwise.

--skipRows=<uint>
            Skip first N physical lines from each of the imported files. You
            can use this option to skip an initial header line containing
            column names. Default: 0.

--table=<str>
            Name of target table. Default: filename without extension.

--threads=<int>
            Use N threads to sent file chunks to the server. Default: 8.

//@<OUT> CLI util load-binlogs --help
NAME
      load-binlogs - Loads binary log dumps created by MySQL Shell from a local
                     or remote directory.

SYNTAX
      util load-binlogs <urls> [<options>]

WHERE
      url: URL to a local or remote directory containing dump created by
           util.dumpBinlogs().

OPTIONS
--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--dryRun=<bool>
            Print information about what would be loaded, but do not load
            anything. Default: false.

--ignoreGtidGap=<bool>
            Load the dumps even if the current value of gtid_executed in the
            target instance does not fully contain the starting value of
            gtid_executed of the first binary log file to be loaded. Default:
            false.

--ignoreVersion=<bool>
            Load the dumps even if version of the target instance is
            incompatible with version of the source instance where the binary
            logs were dumped from. Default: false.

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--stopBefore=<str>
            Stops the load right before the specified binary log event is
            applied. Accepted format:
            <binary-log-file>:<binary-log-file-position> or GTID:
            <UUID>[:tag]:<transaction-id>. Default: not set.

//@<OUT> CLI util load-dump --help
NAME
      load-dump - Loads database dumps created by MySQL Shell.

SYNTAX
      util load-dump <url> [<options>]

WHERE
      url: defines the location of the dump to be loaded

OPTIONS
--analyzeTables=<str>
            "off", "on", "histogram" (default: off) - If 'on', executes ANALYZE
            TABLE for all tables, once loaded. If set to 'histogram', only
            tables that have histogram information stored in the dump will be
            analyzed. This option can be used even if all 'load' options are
            disabled.

--azureConfigFile=<str>
            Use the specified Azure configuration file instead of the one at
            the default location. Default: not set.

--azureContainerName=<str>
            Name of the Azure container to use. The container must already
            exist. Default: not set.

--azureStorageAccount=<str>
            The account to be used for the operation. Default: not set.

--azureStorageSasToken=<str>
            Azure Shared Access Signature (SAS) token, to be used for the
            authentication of the operation, instead of a key. Default: not
            set.

--backgroundThreads=<uint>
            Number of additional threads to use to fetch contents of metadata
            and DDL files. If not set, loader will use the value of the threads
            option in case of a local dump, or four times that value in case on
            a non-local dump. Default: not set.

--characterSet=<str>
            Overrides the character set to be used for loading dump data. By
            default, the same character set used for dumping will be used
            (utf8mb4 if not set on dump). Default: taken from dump.

--checksum=<bool>
            Verify tables against checksums that were computed during dump.
            Default: false.

--createInvisiblePKs=<bool>
            Automatically create an invisible Primary Key for each table which
            does not have one. By default, set to true if dump was created with
            create_invisible_pks compatibility option, false otherwise.
            Requires server 8.0.24 or newer. Default: taken from dump.

--deferTableIndexes=<str>
            "off", "fulltext", "all" (default: fulltext) - If "all", creation
            of "all" indexes except PRIMARY is deferred until after table data
            is loaded, which in many cases can reduce load times. If
            "fulltext", only full-text indexes will be deferred.

--disableBulkLoad=<bool>
            Do not use BULK LOAD feature to load the data, even when available.
            Default: false.

--dropExistingObjects=<bool>
            Load the dump even if it contains user accounts or DDL objects that
            already exist in the target database. If this option is set to
            false, any existing object results in an error. Setting it to true
            drops existing user accounts and objects before creating them.
            Schemas are not dropped. Mutually exclusive with
            ignoreExistingObjects. Default: false.

--dryRun=<bool>
            Scans the dump and prints everything that would be performed,
            without actually doing so. Default: false.

--excludeEvents=<str list>
            Skip loading specified events from the dump. Strings are in format
            schema.event, quoted using backtick characters when required.
            Default: not set.

--excludeRoutines=<str list>
            Skip loading specified routines from the dump. Strings are in
            format schema.routine, quoted using backtick characters when
            required. Default: not set.

--excludeSchemas=<str list>
            Skip loading specified schemas from the dump. Default: not set.

--excludeTables=<str list>
            Skip loading specified tables from the dump. Strings are in format
            schema.table, quoted using backtick characters when required.
            Default: not set.

--excludeTriggers=<str list>
            Skip loading specified triggers from the dump. Strings are in
            format schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger), quoted using
            backtick characters when required. Default: not set.

--excludeUsers=<str list>
            Skip loading specified users from the dump. Each user is in the
            format of 'user_name'[@'host']. If the host is not specified, all
            the accounts with the given user name are excluded. Default: not
            set.

--handleGrantErrors=<str>
            "abort", "drop_account", "ignore" (default: abort) - Specifies
            action to be performed in case of errors related to the
            GRANT/REVOKE statements, "abort": throws an error and aborts the
            load, "drop_account": deletes the problematic account and
            continues, "ignore": ignores the error and continues loading the
            account.

--ignoreExistingObjects=<bool>
            Load the dump even if it contains user accounts or DDL objects that
            already exist in the target database. If this option is set to
            false, any existing object results in an error. Setting it to true
            ignores existing objects, but the CREATE statements are still going
            to be executed, except for the tables and views. Mutually exclusive
            with dropExistingObjects. Default: false.

--ignoreVersion=<bool>
            Load the dump even if the major version number of the server where
            it was created is different from where it will be loaded. Default:
            false.

--includeEvents=<str list>
            Loads only the specified events from the dump. Strings are in
            format schema.event, quoted using backtick characters when
            required. By default, all events are included. Default: not set.

--includeRoutines=<str list>
            Loads only the specified routines from the dump. Strings are in
            format schema.routine, quoted using backtick characters when
            required. By default, all routines are included. Default: not set.

--includeSchemas=<str list>
            Loads only the specified schemas from the dump. By default, all
            schemas are included. Default: not set.

--includeTables=<str list>
            Loads only the specified tables from the dump. Strings are in
            format schema.table, quoted using backtick characters when
            required. By default, all tables from all schemas are included.
            Default: not set.

--includeTriggers=<str list>
            Loads only the specified triggers from the dump. Strings are in
            format schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger), quoted using
            backtick characters when required. By default, all triggers are
            included. Default: not set.

--includeUsers=<str list>
            Load only the specified users from the dump. Each user is in the
            format of 'user_name'[@'host']. If the host is not specified, all
            the accounts with the given user name are included. By default, all
            users are included. Default: not set.

--loadData=<bool>
            Loads table data from the dump. Default: true.

--loadDdl=<bool>
            Executes DDL/SQL scripts in the dump. Default: true.

--loadIndexes=<bool>
            Use together with deferTableIndexes to control whether secondary
            indexes should be recreated at the end of the load. Useful when
            loading DDL and data separately. Default: true.

--loadUsers=<bool>
            Executes SQL scripts for user accounts, roles and grants contained
            in the dump. Note: statements for the current user will be skipped.
            Default: false.

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be loaded from a
            dump data file per single LOAD DATA statement. Supports unit
            suffixes: k (kilobytes), M (Megabytes), G (Gigabytes). Minimum
            value: 4096. If this option is not specified explicitly, the value
            of the bytesPerChunk dump option is used, but only in case of the
            files with data size greater than 1.5 * bytesPerChunk. Not used if
            table is BULK LOADED. Default: taken from dump.

--ociAuth=<str>
            Use the specified authentication method when connecting to the OCI.
            Allowed values: api_key (used when not explicitly set),
            instance_principal, resource_principal, security_token. Default:
            not set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one at the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy ID on the OCI configuration.
            Default: not set.

--progressFile=<str>
            Stores load progress information in the given local file path.
            Default: load-progress.<server_uuid>.progress.

--resetProgress=<bool>
            Discards progress information of previous load attempts to the
            destination server and loads the whole dump again. Default: false.

--s3BucketName=<str>
            Name of the AWS S3 bucket to use. The bucket must already exist.
            Default: not set.

--s3ConfigFile=<str>
            Use the specified AWS config file. Default: not set.

--s3CredentialsFile=<str>
            Use the specified AWS credentials file. Default: not set.

--s3EndpointOverride=<str>
            Use the specified AWS S3 API endpoint instead of the default one.
            Default: not set.

--s3Profile=<str>
            Use the specified AWS profile. Default: not set.

--s3Region=<str>
            Use the specified AWS region. Default: not set.

--schema=<str>
            Load the dump into the given schema. This option can only be used
            when loading just one schema, (either only one schema was dumped,
            or schema filters result in only one schema). Default: not set.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            load data. Default: [].

--showMetadata=<bool>
            Displays the metadata information stored in the dump files, i.e.
            binary log file name and position. Default: false.

--showProgress=<bool>
            Enable or disable import progress information. Default: true if
            stdout is a tty, false otherwise.

--skipBinlog=<bool>
            Disables the binary log for the MySQL sessions used by the loader
            (set sql_log_bin=0). Default: false.

--threads=<uint>
            Number of threads to use to import table data. Default: 4.

--updateGtidSet=<str>
            "off", "replace", "append" (default: off) - if set to a value other
            than 'off' updates GTID_PURGED by either replacing its contents or
            appending to it the gtid set present in the dump.

--waitDumpTimeout=<float>
            Loads a dump while it's still being created. Once all uploaded
            tables are processed the command will either wait for more data,
            the dump is marked as completed or the given timeout (in seconds)
            passes. <= 0 disables waiting. Default: 0.


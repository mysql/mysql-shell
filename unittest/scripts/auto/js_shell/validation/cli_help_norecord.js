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
The following operations are available at 'util':

   check-for-server-upgrade
      Performs series of tests on specified MySQL server to check if the
      upgrade process will succeed.

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
--outputFormat=<str>
            Value can be either TEXT (default) or JSON.

--targetVersion=<str>
            Version to which upgrade will be checked (default=<<<__mysh_version>>>)

--configPath=<str>
            Full path to MySQL server configuration file.

//@<OUT> CLI util dump-instance --help
NAME
      dump-instance - Dumps the whole database to files in the output
                      directory.

SYNTAX
      util dump-instance <outputUrl> [<options>]

WHERE
      outputUrl: Target directory to store the dump files.

OPTIONS
--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Default: "zstd".

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. Default: false.

--consistent=<bool>
            Enable or disable consistent data dumps. Default: true.

--ocimds=<bool>
            Enable checks for compatibility with MySQL Database Service (MDS)
            Default: false.

--compatibility=<str list>
            Apply MySQL Database Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "ignore_missing_pks", "skip_invalid_accounts",
            "strip_definers", "strip_restricted_grants", "strip_tablespaces".
            Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Use specified OCI bucket for the location of the dump. Default: not
            set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one in the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ociParExpireTime=<str>
            Allows defining the expiration time for the PARs generated when
            ociParManifest is enabled. Default: not set.

--ociParManifest=<bool>
            Enables the generation of the PAR manifest while the dump operation
            is being executed. Default: not set.

--excludeTables=<str list>
            List of tables or views to be excluded from the dump in the format
            of schema.table. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the dump in the format of
            schema.table. Default: empty.

--events=<bool>
            Include events from each dumped schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the dump in the format of
            schema.event. Default: empty.

--includeEvents=<str list>
            List of events to be included in the dump in the format of
            schema.event. Default: empty.

--routines=<bool>
            Include functions and stored procedures for each dumped schema.
            Default: true.

--excludeRoutines=<str list>
            List of routines to be excluded from the dump in the format of
            schema.routine. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the dump in the format of
            schema.routine. Default: empty.

--excludeSchemas=<str list>
            List of schemas to be excluded from the dump. Default: empty.

--includeSchemas=<str list>
            List of schemas to be included in the dump. Default: empty.

--users=<bool>
            Include users, roles and grants in the dump file. Default: true.

--excludeUsers=<str list>
            Skip dumping the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are excluded. Default: not set.

--includeUsers=<str list>
            Dump only the specified users. Each user is in the format of
            'user_name'[@'host']. If the host is not specified, all the
            accounts with the given user name are included. By default, all
            users are included. Default: not set.

//@<OUT> CLI util dump-schemas --help
NAME
      dump-schemas - Dumps the specified schemas to the files in the output
                     directory.

SYNTAX
      util dump-schemas <schemas> --outputUrl=<str> [<options>]

WHERE
      schemas: List of schemas to be dumped.

OPTIONS
--outputUrl=<str>
            Target directory to store the dump files.

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Default: "zstd".

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. Default: false.

--consistent=<bool>
            Enable or disable consistent data dumps. Default: true.

--ocimds=<bool>
            Enable checks for compatibility with MySQL Database Service (MDS)
            Default: false.

--compatibility=<str list>
            Apply MySQL Database Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "ignore_missing_pks", "skip_invalid_accounts",
            "strip_definers", "strip_restricted_grants", "strip_tablespaces".
            Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Use specified OCI bucket for the location of the dump. Default: not
            set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one in the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ociParExpireTime=<str>
            Allows defining the expiration time for the PARs generated when
            ociParManifest is enabled. Default: not set.

--ociParManifest=<bool>
            Enables the generation of the PAR manifest while the dump operation
            is being executed. Default: not set.

--excludeTables=<str list>
            List of tables or views to be excluded from the dump in the format
            of schema.table. Default: empty.

--includeTables=<str list>
            List of tables or views to be included in the dump in the format of
            schema.table. Default: empty.

--events=<bool>
            Include events from each dumped schema. Default: true.

--excludeEvents=<str list>
            List of events to be excluded from the dump in the format of
            schema.event. Default: empty.

--includeEvents=<str list>
            List of events to be included in the dump in the format of
            schema.event. Default: empty.

--routines=<bool>
            Include functions and stored procedures for each dumped schema.
            Default: true.

--excludeRoutines=<str list>
            List of routines to be excluded from the dump in the format of
            schema.routine. Default: empty.

--includeRoutines=<str list>
            List of routines to be included in the dump in the format of
            schema.routine. Default: empty.

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
--outputUrl=<str>
            Target directory to store the dump files.

--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Default: "zstd".

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--chunking=<bool>
            Enable chunking of the tables. Default: true.

--bytesPerChunk=<str>
            Sets average estimated number of bytes to be written to each chunk
            file, enables chunking. Default: "64M".

--threads=<uint>
            Use N threads to dump data chunks from the server. Default: 4.

--triggers=<bool>
            Include triggers for each dumped table. Default: true.

--tzUtc=<bool>
            Convert TIMESTAMP data to UTC. Default: true.

--ddlOnly=<bool>
            Only dump Data Definition Language (DDL) from the database.
            Default: false.

--dataOnly=<bool>
            Only dump data from the database. Default: false.

--dryRun=<bool>
            Print information about what would be dumped, but do not dump
            anything. Default: false.

--consistent=<bool>
            Enable or disable consistent data dumps. Default: true.

--ocimds=<bool>
            Enable checks for compatibility with MySQL Database Service (MDS)
            Default: false.

--compatibility=<str list>
            Apply MySQL Database Service compatibility modifications when
            writing dump files. Supported values: "create_invisible_pks",
            "force_innodb", "ignore_missing_pks", "skip_invalid_accounts",
            "strip_definers", "strip_restricted_grants", "strip_tablespaces".
            Default: empty.

--excludeTriggers=<str list>
            List of triggers to be excluded from the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--includeTriggers=<str list>
            List of triggers to be included in the dump in the format of
            schema.table (all triggers from the specified table) or
            schema.table.trigger (the individual trigger). Default: empty.

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Use specified OCI bucket for the location of the dump. Default: not
            set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one in the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

--ociParExpireTime=<str>
            Allows defining the expiration time for the PARs generated when
            ociParManifest is enabled. Default: not set.

--ociParManifest=<bool>
            Enables the generation of the PAR manifest while the dump operation
            is being executed. Default: not set.

--all=<bool>
            Dump all views and tables from the specified schema. Default:
            false.

//@<OUT> CLI util export-table --help
NAME
      export-table - Exports the specified table to the data dump file.

SYNTAX
      util export-table <table> <outputUrl> [<options>]

WHERE
      table: Name of the table to be exported.
      outputUrl: Target file to store the data.

OPTIONS
--maxRate=<str>
            Limit data read throughput to maximum rate, measured in bytes per
            second per thread. Use maxRate="0" to set no limit. Default: "0".

--showProgress=<bool>
            Enable or disable dump progress information. Default: true if
            stdout is a TTY device, false otherwise.

--compression=<str>
            Compression used when writing the data dump files, one of: "none",
            "gzip", "zstd". Default: "none".

--defaultCharacterSet=<str>
            Character set used for the dump. Default: "utf8mb4".

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
            fieldsEscapedBy and linesTerminatedBy options. Must be one of the
            following values: default, csv, tsv or csv-unix. Default:
            "default".

--fieldsTerminatedBy=<str>
            These options have the same meaning as the corresponding clauses
            for SELECT ... INTO OUTFILE. For more information use \? SQL
            Syntax/SELECT, (a session is required). Default: "\t"),
            fieldsEnclosedBy: char (default: ''), fieldsEscapedBy: char
            (default: '\'), linesTerminatedBy: string (default: "\n".

--fieldsEnclosedBy=<str>

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsEscapedBy=<str>

--linesTerminatedBy=<str>

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Use specified OCI bucket for the location of the dump. Default: not
            set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one in the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

//@<OUT> CLI util import-json --help
NAME
      import-json - Import JSON documents from file to collection or table in
                    MySQL Server using X Protocol session.

SYNTAX
      util import-json <path> [<options>]

WHERE
      file: Path to JSON documents file

OPTIONS
--schema=<str>
            Name of target schema.

--collection=<str>
            Name of collection where the data will be imported.

--table=<str>
            Name of table where the data will be imported.

--tableColumn=<str>
            Name of column in target table where the imported JSON documents
            will be stored. Default: "doc".

--convertBsonTypes=<bool>
            Enables the BSON data type conversion. Default: false.

--convertBsonOid=<bool>
            Enables conversion of the BSON ObjectId values. Default: the value
            of convertBsonTypes.

--extractOidTime=<str>
            Creates a new field based on the ObjectID timestamp. Only valid if
            convertBsonOid is enabled. Default: empty.

--ignoreDate=<bool>
            Disables conversion of BSON Date values

--ignoreTimestamp=<bool>
            Disables conversion of BSON Timestamp values

--ignoreBinary=<bool>
            Disables conversion of BSON BinData values.

--ignoreRegex=<bool>
            Disables conversion of BSON Regex values.

--ignoreRegexOptions=<bool>
            Causes regex options to be ignored when processing a Regex BSON
            value. This option is only valid if ignoreRegex is disabled.

--decimalAsDouble=<bool>
            Causes BSON Decimal values to be imported as double values.

//@<OUT> CLI util import-table --help
NAME
      import-table - Import table dump stored in files to target table using
                     LOAD DATA LOCAL INFILE calls in parallel connections.

SYNTAX
      util import-table <files> [<options>]

WHERE
      files: Path or list of paths to files with user data. Path name can
             contain a glob pattern with wildcard '*' and/or '?'. All selected
             files must be chunks of the same target table.

OPTIONS
--table=<str>
            Name of target table Default: filename without extension.

--schema=<str>
            Name of target schema Default: current shell active schema.

--threads=<int>
            Use N threads to sent file chunks to the server. Default: 8.

--bytesPerChunk=<str>
            Send bytesPerChunk (+ bytes to end of the row) in single LOAD DATA
            call. Unit suffixes, k - for Kilobytes (n * 1'000 bytes), M - for
            Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n *
            1'000'000'000 bytes), bytesPerChunk="2k" - ~2 kilobyte data chunk
            will send to the MySQL Server. Not available for multiple files
            import. Default: minimum: "131072", default: "50M".

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

--columns[:<type>]=<value>
            Array of strings and/or integers (default: empty array) - This
            option takes an array of column names as its value. The order of
            the column names indicates how to match data file columns with
            table columns. Use non-negative integer `i` to capture column value
            into user variable @i. With user variables, the decodeColumns
            option enables you to perform preprocessing transformations on
            their values before assigning the result to columns.

--replaceDuplicates=<bool>
            If true, input rows that have the same value for a primary key or
            unique index as an existing row will be replaced, otherwise input
            rows will be skipped. Default: false.

--maxRate=<str>
            Limit data send throughput to maxRate in bytes per second per
            thread. maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n
            * 1'000 bytes), M - for Megabytes (n * 1'000'000 bytes), G - for
            Gigabytes (n * 1'000'000'000 bytes), maxRate="2k" - limit to 2
            kilobytes per second. Default: "0".

--showProgress=<bool>
            Enable or disable import progress information. Default: true if
            stdout is a tty, false otherwise.

--skipRows=<uint>
            Skip first n rows of the data in the file. You can use this option
            to skip an initial header line containing column names. Default: 0.

--decodeColumns=<key>[:<type>]=<value>
            A map between columns names and SQL expressions to be applied on
            the loaded data. Column value captured in 'columns' by integer is
            available as user variable '@i', where `i` is that integer.
            Requires 'columns' to be set. Default: not set.

--characterSet=<str>
            Interpret the information in the input file using this character
            set encoding. characterSet set to "binary" specifies "no
            conversion". If not set, the server will use the character set
            indicated by the character_set_database system variable to
            interpret the information in the file. Default: not set.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            load data. Default: [].

--dialect=<str>
            Setup fields and lines options that matches specific data file
            format. Can be used as base dialect and customized with
            fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
            fieldsEscapedBy and linesTerminatedBy options. Must be one of the
            following values: default, csv, tsv, json or csv-unix. Default:
            "default".

--fieldsTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: "\t".

--fieldsEnclosedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: ''.

--fieldsOptionallyEnclosed=<bool>
            Set to true if the input values are not necessarily enclosed within
            quotation marks specified by fieldsEnclosedBy option. Set to false
            if all fields are quoted by character specified by fieldsEnclosedBy
            option. Default: false.

--fieldsEscapedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. Default: '\'.

--linesTerminatedBy=<str>
            This option has the same meaning as the corresponding clause for
            LOAD DATA INFILE. For example, to import Windows files that have
            lines terminated with carriage return/linefeed pairs, use
            --lines-terminated-by="\r\n". (You might have to double the
            backslashes, depending on the escaping conventions of your command
            interpreter.) See Section 13.2.7, "LOAD DATA INFILE Syntax".
            Default: "\n".

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Name of the OCI Object Storage bucket to use. The bucket must
            already exist. Default: not set.

--ociConfigFile=<str>
            Override oci.configFile shell option, to specify the path to the
            OCI configuration file. Default: not set.

--ociProfile=<str>
            Override oci.profile shell option, to specify the name of the OCI
            profile to use. Default: not set.

//@<OUT> CLI util load-dump --help
NAME
      load-dump - Loads database dumps created by MySQL Shell.

SYNTAX
      util load-dump <url> [<options>]

WHERE
      url: defines the location of the dump to be loaded

OPTIONS
--threads=<uint>
            Number of threads to use to import table data. Default: 4.

--backgroundThreads=<uint>
            Number of additional threads to use to fetch contents of metadata
            and DDL files. If not set, loader will use the value of the threads
            option in case of a local dump, or four times that value in case on
            a non-local dump. Default: not set.

--showProgress=<bool>
            Enable or disable import progress information. Default: true if
            stdout is a tty, false otherwise.

--waitDumpTimeout=<float>
            Loads a dump while it's still being created. Once all uploaded
            tables are processed the command will either wait for more data,
            the dump is marked as completed or the given timeout (in seconds)
            passes. <= 0 disables waiting. Default: 0.

--loadData=<bool>
            Loads table data from the dump. Default: true.

--loadDdl=<bool>
            Executes DDL/SQL scripts in the dump. Default: true.

--loadUsers=<bool>
            Executes SQL scripts for user accounts, roles and grants contained
            in the dump. Note: statements for the current user will be skipped.
            Default: false.

--dryRun=<bool>
            Scans the dump and prints everything that would be performed,
            without actually doing so. Default: false.

--resetProgress=<bool>
            Discards progress information of previous load attempts to the
            destination server and loads the whole dump again. Default: false.

--progressFile=<str>
            Stores load progress information in the given local file path.
            Default: load-progress.<server_uuid>.progress.

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

--characterSet=<str>
            Overrides the character set to be used for loading dump data. By
            default, the same character set used for dumping will be used
            (utf8mb4 if not set on dump). Default: taken from dump.

--skipBinlog=<bool>
            Disables the binary log for the MySQL sessions used by the loader
            (set sql_log_bin=0). Default: false.

--ignoreExistingObjects=<bool>
            Load the dump even if it contains objects that already exist in the
            target database. Default: false.

--ignoreVersion=<bool>
            Load the dump even if the major version number of the server where
            it was created is different from where it will be loaded. Default:
            false.

--analyzeTables=<str>
            "off", "on", "histogram" (default: off) - If 'on', executes ANALYZE
            TABLE for all tables, once loaded. If set to 'histogram', only
            tables that have histogram information stored in the dump will be
            analyzed. This option can be used even if all 'load' options are
            disabled.

--deferTableIndexes=<str>
            "off", "fulltext", "all" (default: fulltext) - If "all", creation
            of "all" indexes except PRIMARY is deferred until after table data
            is loaded, which in many cases can reduce load times. If
            "fulltext", only full-text indexes will be deferred.

--loadIndexes=<bool>
            Use together with ‘deferTableIndexes’ to control whether
            secondary indexes should be recreated at the end of the load.
            Useful when loading DDL and data separately. Default: true.

--schema=<str>
            Load the dump into the given schema. This option can only be used
            when loading dumps created by the util.dumpTables() function.
            Default: not set.

--excludeUsers=<str list>
            Skip loading specified users from the dump. Each user is in the
            format of 'user_name'[@'host']. If the host is not specified, all
            the accounts with the given user name are excluded. Default: not
            set.

--includeUsers=<str list>
            Load only the specified users from the dump. Each user is in the
            format of 'user_name'[@'host']. If the host is not specified, all
            the accounts with the given user name are included. By default, all
            users are included. Default: not set.

--updateGtidSet=<str>
            "off", "replace", "append" (default: off) - if set to a value other
            than 'off' updates GTID_PURGED by either replacing its contents or
            appending to it the gtid set present in the dump.

--showMetadata=<bool>
            Displays the metadata information stored in the dump files, i.e.
            binary log file name and position. Default: false.

--createInvisiblePKs=<bool>
            Automatically create an invisible Primary Key for each table which
            does not have one. By default, set to true if dump was created with
            create_invisible_pks compatibility option, false otherwise.
            Requires server 8.0.24 or newer. Default: taken from dump.

--maxBytesPerTransaction=<str>
            Specifies the maximum number of bytes that can be loaded from a
            dump data file per single LOAD DATA statement. Supports unit
            suffixes: k (kilobytes), M (Megabytes), G (Gigabytes). Minimum
            value: 4096. If this option is not specified explicitly, the value
            of the bytesPerChunk dump option is used, but only in case of the
            files with data size greater than 1.5 * bytesPerChunk. Default:
            taken from dump.

--sessionInitSql=<str list>
            Execute the given list of SQL statements in each session about to
            load data. Default: [].

--osNamespace=<str>
            Specifies the namespace where the bucket is located, if not given
            it will be obtained using the tenancy id on the OCI configuration.
            Default: not set.

--osBucketName=<str>
            Use specified OCI bucket for the location of the dump. Default: not
            set.

--ociConfigFile=<str>
            Use the specified OCI configuration file instead of the one in the
            default location. Default: not set.

--ociProfile=<str>
            Use the specified OCI profile instead of the default one. Default:
            not set.

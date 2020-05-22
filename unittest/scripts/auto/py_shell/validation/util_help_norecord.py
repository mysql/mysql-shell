#@<OUT> util help
NAME
      util - Global object that groups miscellaneous tools like upgrade checker
             and JSON import.

DESCRIPTION
      Global object that groups miscellaneous tools like upgrade checker and
      JSON import.

FUNCTIONS
      check_for_server_upgrade([connectionData][, options])
            Performs series of tests on specified MySQL server to check if the
            upgrade process will succeed.

      configure_oci([profile])
            Wizard to create a valid configuration for the OCI SDK.

      dump_instance(outputUrl[, options])
            Dumps the whole database to files in the output directory.

      dump_schemas(schemas, outputUrl[, options])
            Dumps the specified schemas to the files in the output directory.

      help([member])
            Provides help about this object and it's members

      import_json(file[, options])
            Import JSON documents from file to collection or table in MySQL
            Server using X Protocol session.

      import_table(filename[, options])
            Import table dump stored in filename to target table using LOAD
            DATA LOCAL INFILE calls in parallel connections.

      load_dump(url[, options])
            Loads database dumps created by MySQL Shell.

#@<OUT> util check_for_server_upgrade help
NAME
      check_for_server_upgrade - Performs series of tests on specified MySQL
                                 server to check if the upgrade process will
                                 succeed.

SYNTAX
      util.check_for_server_upgrade([connectionData][, options])

WHERE
      connectionData: The connection data to server to be checked
      options: Dictionary of options to modify tool behaviour.

DESCRIPTION
      If no connectionData is specified tool will try to establish connection
      using data from current session.

      Tool behaviour can be modified with following options:

      - configPath - full path to MySQL server configuration file.
      - outputFormat - value can be either TEXT (default) or JSON.
      - targetVersion - version to which upgrade will be checked
        (default=<<<__mysh_version>>>)
      - password - password for connection.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: The SSL mode to be used in the connection.
      - ssl-ca: The path to the X509 certificate authority file in PEM format.
      - ssl-capath: The path to the directory that contains the X509
        certificate authority files in PEM format.
      - ssl-cert: The path to the SSL public key certificate file in PEM
        format.
      - ssl-key: The path to the SSL private key file in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: The list of permissible encryption ciphers for connections
        that use TLS protocols up through TLSv1.2.
      - tls-version: List of protocols permitted for secure connections.
      - tls-ciphers: List of TLS v1.3 ciphers to use.
      - auth-method: Authentication method.
      - get-server-public-key: Request public key from the server required for
        RSA key pair-based password exchange. Use when connecting to MySQL 8.0
        servers with classic MySQL sessions with SSL mode DISABLED.
      - server-public-key-path: The path name to a file containing a
        client-side copy of the public key required by the server for RSA key
        pair-based password exchange. Use when connecting to MySQL 8.0 servers
        with classic MySQL sessions with SSL mode DISABLED.
      - connect-timeout: The connection timeout in milliseconds. If not
        provided a default timeout of 10 seconds will be used. Specifying a
        value of 0 disables the connection timeout.
      - compression: Enable compression in client/server protocol.
      - compression-algorithms: Use compression algorithm in server/client
        protocol.
      - compression-level: Use this compression level in the client/server
        protocol.
      - connection-attributes: List of connection attributes to be registered
        at the PERFORMANCE_SCHEMA connection attributes tables.
      - local-infile: Enable/disable LOAD DATA LOCAL INFILE.
      - net-buffer-length: The buffer size for TCP/IP and socket communication.

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on the connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.

#@<OUT> util dump_instance help
NAME
      dump_instance - Dumps the whole database to files in the output
                      directory.

SYNTAX
      util.dump_instance(outputUrl[, options])

WHERE
      outputUrl: Target directory to store the dump files.
      options: Dictionary with the dump options.

DESCRIPTION
      The outputUrl specifies where the dump is going to be stored.

      By default, a local directory is used, and in this case outputUrl can be
      prefixed with file:// scheme. If a relative path is given, the absolute
      path is computed as relative to the current working directory. If the
      output directory does not exist but its parent does, it is created. If
      the output directory exists, it must be empty. All directories created
      during the dump will have the following access rights (on operating
      systems which support them): rwxr-x---. All files created during the dump
      will have the following access rights (on operating systems which support
      them): rw-r-----.

      The following options are supported:

      - excludeSchemas: list of strings (default: empty) - list of schemas to
        be excluded from the dump.
      - excludeTables: list of strings (default: empty) - List of tables to be
        excluded from the dump in the format of schema.table.
      - users: bool (default: true) - Include users, roles and grants in the
        dump file.
      - events: bool (default: true) - Include events from each dumped schema.
      - routines: bool (default: true) - Include functions and stored
        procedures for each dumped schema.
      - triggers: bool (default: true) - Include triggers for each dumped
        table.
      - tzUtc: bool (default: true) - Convert TMESTAMP data to UTC.
      - consistent: bool (default: true) - Enable or disable consistent data
        dumps.
      - ddlOnly: bool (default: false) - Only dump Data Definition Language
        (DDL) from the database.
      - dataOnly: bool (default: false) - Only dump data from the database.
      - dryRun: bool (default: false) - Print information about what would be
        dumped, but do not dump anything.
      - ocimds: bool (default: false) - Enable checks for compatibility with
        MySQL Database Service (MDS)
      - compatibility: list of strings (default: empty) - Apply MySQL Database
        Service compatibility modifications when writing dump files. Supported
        values: "force_innodb", "strip_definers", "strip_restricted_grants",
        "strip_role_admin", "strip_tablespaces".
      - chunking: bool (default: true) - Enable chunking of the tables.
      - bytesPerChunk: string (default: "32M") - Sets average estimated number
        of bytes to be written to each chunk file, enables chunking.
      - threads: int (default: 4) - Use N threads to dump data chunks from the
        server.
      - maxRate: string (default: "0") - Limit data read throughput to maximum
        rate, measured in bytes per second per thread. Use maxRate="0" to set
        no limit.
      - showProgress: bool (default: true if stdout is a TTY device, false
        otherwise) - Enable or disable dump progress information.
      - compression: string (default: "zstd") - Compression used when writing
        the data dump files, one of: "none", "gzip", "zstd".
      - defaultCharacterSet: string (default: "utf8mb4") - Character set used
        for the dump.
      - osBucketName: string (default: not set) - Use specified OCI bucket for
        the location of the dump.
      - osNamespace: string (default: not set) - Specify the OCI namespace
        (tenancy name) where the OCI bucket is located.
      - ociConfigFile: string (default: not set) - Use the specified OCI
        configuration file instead of the one in the default location.
      - ociProfile: string (default: not set) - Use the specified OCI profile
        instead of the default one.

      Requirements

      - MySQL Server 5.7 or newer is required.
      - Schema object names must use latin1 or utf8 character set.
      - Only tables which use the InnoDB storage engine are guaranteed to be
        dumped with consistent data.
      - File size limit for files uploaded to the OCI bucket is 1.2 TiB.

      Details

      This operation writes SQL files per each schema, table and view dumped,
      along with some global SQL files.

      Table data dumps are written to TSV files, optionally splitting them into
      multiple chunk files.

      Requires an open, global Shell session, and uses its connection options,
      such as compression, ssl-mode, etc., to establish additional connections.

      Data dumps cannot be created for the following tables:

      - mysql.apply_status
      - mysql.general_log
      - mysql.schema
      - mysql.slow_log

      Dumps cannot be created for the following schemas:

      - information_schema,
      - mysql,
      - ndbinfo,
      - performance_schema,
      - sys.

      Options

      If the excludeSchemas option contains a schema which is not included in
      the dump or does not exist, it is ignored.

      The names given in the excludeTables option should be valid MySQL
      identifiers, quoted using backtick characters when required.

      If the excludeTables option contains a table which does not exist, or a
      table which belongs to a schema which is not included in the dump or does
      not exist, it is ignored.

      The tzUtc option allows dumping TIMESTAMP data when a server has data in
      different time zones or data is being moved between servers with
      different time zones.

      If the consistent option is set to true, a global read lock is set using
      the FLUSH TABLES WITH READ LOCK statement, all threads establish
      connections with the server and start transactions using:

      - SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ
      - START TRANSACTION WITH CONSISTENT SNAPSHOT

      Once all the threads start transactions, the instance is locked for
      backup and the global read lock is released.

      The ddlOnly and dataOnly options cannot both be set to true at the same
      time.

      The chunking option causes the the data from each table to be split and
      written to multiple chunk files. If this option is set to false, table
      data is written to a single file.

      If the chunking option is set to true, but a table to be dumped cannot be
      chunked (for example if it does not contain a primary key or a unique
      index), a warning is displayed and chunking is disabled for this table.

      The value of the threads option must be a positive number.

      Both the bytesPerChunk and maxRate options support unit suffixes:

      - k - for Kilobytes (n * 1'000 bytes),
      - M - for Megabytes (n * 1'000'000 bytes),
      - G - for Gigabytes (n * 1'000'000'000 bytes),

      i.e. maxRate="2k" - limit throughput to 2 kilobytes per second.

      The value of the bytesPerChunk option cannot be smaller than "128k".

      MySQL Database Service Compatibility

      The MySQL Database Service has a few security related restrictions that
      are not present in a regular, on-premise instance of MySQL. In order to
      make it easier to load existing databases into the Service, the dump
      commands in the MySQL Shell has options to detect potential issues and in
      some cases, to automatically adjust your schema definition to be
      compliant.

      The ocimds option, when set to true, will perform schema checks for most
      of these issues and abort the dump if any are found. The load_dump
      command will also only allow loading dumps that have been created with
      the "ocimds" option enabled.

      Some issues found by the ocimds option may require you to manually make
      changes to your database schema before it can be loaded into the MySQL
      Database Service. However, the compatibility option can be used to
      automatically modify the dumped schema SQL scripts, resolving some of
      these compatibility issues. You may pass one or more of the following
      options to "compatibility", separated by a comma (,).

      force_innodb - The MySQL Database Service requires use of the InnoDB
      storage engine. This option will modify the ENGINE= clause of CREATE
      TABLE statements that use incompatible storage engines and replace them
      with InnoDB.

      strip_definers - strips the "DEFINER=account" clause from views,
      routines, events and triggers. The MySQL Database Service requires
      special privileges to create these objects with a definer other than the
      user loading the schema. By stripping the DEFINER clause, these objects
      will be created with that default definer. Views and Routines will
      additionally have their SQL SECURITY clause changed from DEFINER to
      INVOKER. This ensures that the access permissions of the account querying
      or calling these are applied, instead of the user that created them. This
      should be sufficient for most users, but if your database security model
      requires that views and routines have more privileges than their invoker,
      you will need to manually modify the schema before loading it.

      Please refer to the MySQL manual for details about DEFINER and SQL
      SECURITY.

      strip_restricted_grants - Certain privileges are restricted in the MySQL
      Database Service. Attempting to create users granting these privileges
      would fail, so this option allows dumped GRANT statements to be stripped
      of these privileges.

      strip_role_admin - ROLE_ADMIN privilege can be restricted in the MySQL
      Database Service, so attempting to create users granting it would fail.
      This option allows dumped GRANT statements to be stripped of this
      privilege.

      strip_tablespaces - Tablespaces have some restrictions in the MySQL
      Database Service. If you'd like to have tables created in their default
      tablespaces, this option will strip the TABLESPACE= option from CREATE
      TABLE statements.

      Additionally, the following changes will always be made to DDL scripts
      when the ocimds option is enabled:

      - DATA DIRECTORY, INDEX DIRECTORY and ENCRYPTION options in CREATE TABLE
        statements will be commented out.

      Please refer to the MySQL Database Service documentation for more
      information about restrictions and compatibility.

      Dumping to a Bucket in the OCI Object Storage

      If the osBucketName option is used, the dump is stored in the specified
      OCI bucket, connection is established using the local OCI profile. The
      directory structure is simulated within the object name.

      The osNamespace, ociConfigFile and ociProfile options cannot be used if
      option osBucketName is set to an empty string.

      The osNamespace option overrides the OCI namespace obtained based on the
      tenancy ID from the local OCI profile.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If any of the input arguments contains an invalid value.

      RuntimeError in the following scenarios:

      - If there is no open global session.
      - If creating the output directory fails.
      - If creating or writing to the output file fails.

#@<OUT> util dump_schemas help
NAME
      dump_schemas - Dumps the specified schemas to the files in the output
                     directory.

SYNTAX
      util.dump_schemas(schemas, outputUrl[, options])

WHERE
      schemas: List of schemas to be dumped.
      outputUrl: Target directory to store the dump files.
      options: Dictionary with the dump options.

DESCRIPTION
      The schemas parameter cannot be an empty list.

      The outputUrl specifies where the dump is going to be stored.

      By default, a local directory is used, and in this case outputUrl can be
      prefixed with file:// scheme. If a relative path is given, the absolute
      path is computed as relative to the current working directory. If the
      output directory does not exist but its parent does, it is created. If
      the output directory exists, it must be empty. All directories created
      during the dump will have the following access rights (on operating
      systems which support them): rwxr-x---. All files created during the dump
      will have the following access rights (on operating systems which support
      them): rw-r-----.

      The following options are supported:

      - excludeTables: list of strings (default: empty) - List of tables to be
        excluded from the dump in the format of schema.table.
      - users: bool (default: false) - Include users, roles and grants in the
        dump file.
      - events: bool (default: true) - Include events from each dumped schema.
      - routines: bool (default: true) - Include functions and stored
        procedures for each dumped schema.
      - triggers: bool (default: true) - Include triggers for each dumped
        table.
      - tzUtc: bool (default: true) - Convert TMESTAMP data to UTC.
      - consistent: bool (default: true) - Enable or disable consistent data
        dumps.
      - ddlOnly: bool (default: false) - Only dump Data Definition Language
        (DDL) from the database.
      - dataOnly: bool (default: false) - Only dump data from the database.
      - dryRun: bool (default: false) - Print information about what would be
        dumped, but do not dump anything.
      - ocimds: bool (default: false) - Enable checks for compatibility with
        MySQL Database Service (MDS)
      - compatibility: list of strings (default: empty) - Apply MySQL Database
        Service compatibility modifications when writing dump files. Supported
        values: "force_innodb", "strip_definers", "strip_restricted_grants",
        "strip_role_admin", "strip_tablespaces".
      - chunking: bool (default: true) - Enable chunking of the tables.
      - bytesPerChunk: string (default: "32M") - Sets average estimated number
        of bytes to be written to each chunk file, enables chunking.
      - threads: int (default: 4) - Use N threads to dump data chunks from the
        server.
      - maxRate: string (default: "0") - Limit data read throughput to maximum
        rate, measured in bytes per second per thread. Use maxRate="0" to set
        no limit.
      - showProgress: bool (default: true if stdout is a TTY device, false
        otherwise) - Enable or disable dump progress information.
      - compression: string (default: "zstd") - Compression used when writing
        the data dump files, one of: "none", "gzip", "zstd".
      - defaultCharacterSet: string (default: "utf8mb4") - Character set used
        for the dump.
      - osBucketName: string (default: not set) - Use specified OCI bucket for
        the location of the dump.
      - osNamespace: string (default: not set) - Specify the OCI namespace
        (tenancy name) where the OCI bucket is located.
      - ociConfigFile: string (default: not set) - Use the specified OCI
        configuration file instead of the one in the default location.
      - ociProfile: string (default: not set) - Use the specified OCI profile
        instead of the default one.

      Requirements

      - MySQL Server 5.7 or newer is required.
      - Schema object names must use latin1 or utf8 character set.
      - Only tables which use the InnoDB storage engine are guaranteed to be
        dumped with consistent data.
      - File size limit for files uploaded to the OCI bucket is 1.2 TiB.

      Details

      This operation writes SQL files per each schema, table and view dumped,
      along with some global SQL files.

      Table data dumps are written to TSV files, optionally splitting them into
      multiple chunk files.

      Requires an open, global Shell session, and uses its connection options,
      such as compression, ssl-mode, etc., to establish additional connections.

      Data dumps cannot be created for the following tables:

      - mysql.apply_status
      - mysql.general_log
      - mysql.schema
      - mysql.slow_log

      Options

      The names given in the excludeTables option should be valid MySQL
      identifiers, quoted using backtick characters when required.

      If the excludeTables option contains a table which does not exist, or a
      table which belongs to a schema which is not included in the dump or does
      not exist, it is ignored.

      The tzUtc option allows dumping TIMESTAMP data when a server has data in
      different time zones or data is being moved between servers with
      different time zones.

      If the consistent option is set to true, a global read lock is set using
      the FLUSH TABLES WITH READ LOCK statement, all threads establish
      connections with the server and start transactions using:

      - SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ
      - START TRANSACTION WITH CONSISTENT SNAPSHOT

      Once all the threads start transactions, the instance is locked for
      backup and the global read lock is released.

      The ddlOnly and dataOnly options cannot both be set to true at the same
      time.

      The chunking option causes the the data from each table to be split and
      written to multiple chunk files. If this option is set to false, table
      data is written to a single file.

      If the chunking option is set to true, but a table to be dumped cannot be
      chunked (for example if it does not contain a primary key or a unique
      index), a warning is displayed and chunking is disabled for this table.

      The value of the threads option must be a positive number.

      Both the bytesPerChunk and maxRate options support unit suffixes:

      - k - for Kilobytes (n * 1'000 bytes),
      - M - for Megabytes (n * 1'000'000 bytes),
      - G - for Gigabytes (n * 1'000'000'000 bytes),

      i.e. maxRate="2k" - limit throughput to 2 kilobytes per second.

      The value of the bytesPerChunk option cannot be smaller than "128k".

      MySQL Database Service Compatibility

      The MySQL Database Service has a few security related restrictions that
      are not present in a regular, on-premise instance of MySQL. In order to
      make it easier to load existing databases into the Service, the dump
      commands in the MySQL Shell has options to detect potential issues and in
      some cases, to automatically adjust your schema definition to be
      compliant.

      The ocimds option, when set to true, will perform schema checks for most
      of these issues and abort the dump if any are found. The load_dump
      command will also only allow loading dumps that have been created with
      the "ocimds" option enabled.

      Some issues found by the ocimds option may require you to manually make
      changes to your database schema before it can be loaded into the MySQL
      Database Service. However, the compatibility option can be used to
      automatically modify the dumped schema SQL scripts, resolving some of
      these compatibility issues. You may pass one or more of the following
      options to "compatibility", separated by a comma (,).

      force_innodb - The MySQL Database Service requires use of the InnoDB
      storage engine. This option will modify the ENGINE= clause of CREATE
      TABLE statements that use incompatible storage engines and replace them
      with InnoDB.

      strip_definers - strips the "DEFINER=account" clause from views,
      routines, events and triggers. The MySQL Database Service requires
      special privileges to create these objects with a definer other than the
      user loading the schema. By stripping the DEFINER clause, these objects
      will be created with that default definer. Views and Routines will
      additionally have their SQL SECURITY clause changed from DEFINER to
      INVOKER. This ensures that the access permissions of the account querying
      or calling these are applied, instead of the user that created them. This
      should be sufficient for most users, but if your database security model
      requires that views and routines have more privileges than their invoker,
      you will need to manually modify the schema before loading it.

      Please refer to the MySQL manual for details about DEFINER and SQL
      SECURITY.

      strip_restricted_grants - Certain privileges are restricted in the MySQL
      Database Service. Attempting to create users granting these privileges
      would fail, so this option allows dumped GRANT statements to be stripped
      of these privileges.

      strip_role_admin - ROLE_ADMIN privilege can be restricted in the MySQL
      Database Service, so attempting to create users granting it would fail.
      This option allows dumped GRANT statements to be stripped of this
      privilege.

      strip_tablespaces - Tablespaces have some restrictions in the MySQL
      Database Service. If you'd like to have tables created in their default
      tablespaces, this option will strip the TABLESPACE= option from CREATE
      TABLE statements.

      Additionally, the following changes will always be made to DDL scripts
      when the ocimds option is enabled:

      - DATA DIRECTORY, INDEX DIRECTORY and ENCRYPTION options in CREATE TABLE
        statements will be commented out.

      Please refer to the MySQL Database Service documentation for more
      information about restrictions and compatibility.

      Dumping to a Bucket in the OCI Object Storage

      If the osBucketName option is used, the dump is stored in the specified
      OCI bucket, connection is established using the local OCI profile. The
      directory structure is simulated within the object name.

      The osNamespace, ociConfigFile and ociProfile options cannot be used if
      option osBucketName is set to an empty string.

      The osNamespace option overrides the OCI namespace obtained based on the
      tenancy ID from the local OCI profile.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If any of the input arguments contains an invalid value.

      RuntimeError in the following scenarios:

      - If there is no open global session.
      - If creating the output directory fails.
      - If creating or writing to the output file fails.


#@<OUT> util import_json help
NAME
      import_json - Import JSON documents from file to collection or table in
                    MySQL Server using X Protocol session.

SYNTAX
      util.import_json(file[, options])

WHERE
      file: Path to JSON documents file
      options: Dictionary with import options

DESCRIPTION
      This function reads standard JSON documents from a file, however, it also
      supports converting BSON Data Types represented using the MongoDB
      Extended Json (strict mode) into MySQL values.

      The options dictionary supports the following options:

      - schema: string - name of target schema.
      - collection: string - name of collection where the data will be
        imported.
      - table: string - name of table where the data will be imported.
      - tableColumn: string (default: "doc") - name of column in target table
        where the imported JSON documents will be stored.
      - convertBsonTypes: bool (default: false) - enables the BSON data type
        conversion.
      - convertBsonOid: bool (default: the value of convertBsonTypes) - enables
        conversion of the BSON ObjectId values.
      - extractOidTime: string (default: empty) - creates a new field based on
        the ObjectID timestamp. Only valid if convertBsonOid is enabled.

      The following options are valid only when convertBsonTypes is enabled.
      They are all boolean flags. ignoreRegexOptions is enabled by default,
      rest are disabled by default.

      - ignoreDate: disables conversion of BSON Date values
      - ignoreTimestamp: disables conversion of BSON Timestamp values
      - ignoreRegex: disables conversion of BSON Regex values.
      - ignoreBinary: disables conversion of BSON BinData values.
      - decimalAsDouble: causes BSON Decimal values to be imported as double
        values.
      - ignoreRegexOptions: causes regex options to be ignored when processing
        a Regex BSON value. This option is only valid if ignoreRegex is
        disabled.

      If the schema is not provided, an active schema on the global session, if
      set, will be used.

      The collection and the table options cannot be combined. If they are not
      provided, the basename of the file without extension will be used as
      target collection name.

      If the target collection or table does not exist, they are created,
      otherwise the data is inserted into the existing collection or table.

      The tableColumn implies the use of the table option and cannot be
      combined with the collection option.

      BSON Data Type Processing.

      If only convertBsonOid is enabled, no conversion will be done on the rest
      of the BSON Data Types.

      To use extractOidTime, it should be set to a name which will be used to
      insert an additional field into the main document. The value of the new
      field will be the timestamp obtained from the ObjectID value. Note that
      this will be done only for an ObjectID value associated to the '_id'
      field of the main document.

      NumberLong and NumberInt values will be converted to integer values.

      NumberDecimal values are imported as strings, unless decimalAsDouble is
      enabled.

      Regex values will be converted to strings containing the regular
      expression. The regular expression options are ignored unless
      ignoreRegexOptions is disabled. When ignoreRegexOptions is disabled the
      regular expression will be converted to the form: /<regex>/<options>.

EXCEPTIONS
      Throws ArgumentError when:

      - Option name is invalid
      - Required options are not set and cannot be deduced
      - Shell is not connected to MySQL Server using X Protocol
      - Schema is not provided and there is no active schema on the global
        session
      - Both collection and table are specified

      Throws LogicError when:

      - Path to JSON document does not exists or is not a file

      Throws RuntimeError when:

      - The schema does not exists
      - MySQL Server returns an error

      Throws InvalidJson when:

      - JSON document is ill-formed

#@<OUT> util configure_oci help
NAME
      configure_oci - Wizard to create a valid configuration for the OCI SDK.

SYNTAX
      util.configure_oci([profile])

WHERE
      profile: Parameter to specify the name profile to be configured.

DESCRIPTION
      If the profile name is not specified 'DEFAULT' will be used.

      To properly create OCI configuration to use the OCI SDK the following
      information will be required:

      - User OCID
      - Tenancy OCID
      - Tenancy region
      - A valid API key

      For details about where to find the user and tenancy details go to
      https://docs.us-phoenix-1.oraclecloud.com/Content/API/Concepts/apisigningkey.htm#Other

#@<OUT> oci help
The MySQL Shell offers support for the Oracle Cloud Infrastructure (OCI).

After starting the MySQL Shell with the --oci option an interactive wizard will
help to create the correct OCI configuration file, load the OCI Python SDK and
switch the shell to Python mode.

The MySQL Shell can then be used to access the OCI APIs and manage the OCI
account.

The following Python objects are automatically initialized.

- config an OCI configuration object with data loaded from ~/.oci/config
- identity an OCI identity client object, offering APIs for managing users,
  groups, compartments and policies.
- compute an OCI compute client object, offering APIs for Networking Service,
  Compute Service, and Block Volume Service.

For more information about the OCI Python SDK please read the documentation at
https://oracle-cloud-infrastructure-python-sdk.readthedocs.io/en/latest/

EXAMPLES
identity.get_user(config['user']).data
      Fetches information about the OCI user account specified in the config
      object.

identity.list_compartments(config['tenancy']).data
      Fetches the list of top level compartments available in the tenancy that
      was specified in the config object.

compartment = identity.list_compartments(config['tenancy']).data[0]
images = compute.list_images(compartment.id).data
for image in images:
  print(image.display_name)
      Assignes the first compartment of the tenancy to the compartment
      variable, featches the available OS images for the compartment and prints
      a list of their names.

#@<OUT> util import_table help
NAME
      import_table - Import table dump stored in filename to target table using
                     LOAD DATA LOCAL INFILE calls in parallel connections.

SYNTAX
      util.import_table(filename[, options])

WHERE
      filename: Path to file with user data
      options: Dictionary with import options

DESCRIPTION
      Scheme part of filename contains infomation about the transport backend.
      Supported transport backends are: file://, http://, https://, oci+os://.
      If scheme part of filename is omitted, then file:// transport backend
      will be chosen.

      Supported filename formats:

      - [file://]/path/to/file - Read import data from local file
      - http[s]://host.domain[:port]/path/to/file - Read import data from file
        provided in URL
      - oci+os://region/namespace/bucket/object - Read import data from object
        stored in OCI (Oracle Cloud Infrastructure) Object Storage. Variables
        needed to sign requests will be obtained from profile configured in OCI
        configuration file. Profile name and configuration file path are
        specified in oci.profile and oci.configFile shell options. ociProfile
        and ociConfigFile options will override, respectively, oci.profile and
        oci.configFile shell options.

      Options dictionary:

      - schema: string (default: current shell active schema) - Name of target
        schema
      - table: string (default: filename without extension) - Name of target
        table
      - columns: string array (default: empty array) - This option takes a
        array of column names as its value. The order of the column names
        indicates how to match data file columns with table columns.
      - fieldsTerminatedBy: string (default: "\t"), fieldsEnclosedBy: char
        (default: ''), fieldsEscapedBy: char (default: '\') - These options
        have the same meaning as the corresponding clauses for LOAD DATA
        INFILE. For more information use \? LOAD DATA, (a session is required).
      - fieldsOptionallyEnclosed: bool (default: false) - Set to true if the
        input values are not necessarily enclosed within quotation marks
        specified by fieldsEnclosedBy option. Set to false if all fields are
        quoted by character specified by fieldsEnclosedBy option.
      - linesTerminatedBy: string (default: "\n") - This option has the same
        meaning as the corresponding clause for LOAD DATA INFILE. For example,
        to import Windows files that have lines terminated with carriage
        return/linefeed pairs, use --lines-terminated-by="\r\n". (You might
        have to double the backslashes, depending on the escaping conventions
        of your command interpreter.) See Section 13.2.7, "LOAD DATA INFILE
        Syntax".
      - replaceDuplicates: bool (default: false) - If true, input rows that
        have the same value for a primary key or unique index as an existing
        row will be replaced, otherwise input rows will be skipped.
      - threads: int (default: 8) - Use N threads to sent file chunks to the
        server.
      - bytesPerChunk: string (minimum: "131072", default: "50M") - Send
        bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call.
        Unit suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes
        (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000 bytes),
        bytesPerChunk="2k" - ~2 kilobyte data chunk will send to the MySQL
        Server.
      - maxRate: string (default: "0") - Limit data send throughput to maxRate
        in bytes per second per thread. maxRate="0" - no limit. Unit suffixes,
        k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n * 1'000'000
        bytes), G - for Gigabytes (n * 1'000'000'000 bytes), maxRate="2k" -
        limit to 2 kilobytes per second.
      - showProgress: bool (default: true if stdout is a tty, false otherwise)
        - Enable or disable import progress information.
      - skipRows: int (default: 0) - Skip first n rows of the data in the file.
        You can use this option to skip an initial header line containing
        column names.
      - dialect: enum (default: "default") - Setup fields and lines options
        that matches specific data file format. Can be used as base dialect and
        customized with fieldsTerminatedBy, fieldsEnclosedBy,
        fieldsOptionallyEnclosed, fieldsEscapedBy and linesTerminatedBy
        options. Must be one of the following values: default, csv, tsv, json
        or csv-unix.
      - decodeColumns: map (default: not set) - a map between columns names to
        decode methods (UNHEX or FROM_BASE64) to be applied on the loaded data.
        Requires 'columns' to be set.
      - characterSet: string (default: not set) - Interpret the information in
        the input file using this character set encoding. characterSet set to
        "binary" specifies "no conversion". If not set, the server will use the
        character set indicated by the character_set_database system variable
        to interpret the information in the file.
      - ociConfigFile: string (default: not set) - Override oci.configFile
        shell option. Available only if oci+os:// transport protocol is in use.
      - ociProfile: string (default: not set) - Override oci.profile shell
        option. Available only if oci+os:// transport protocol is in use.

      dialect predefines following set of options fieldsTerminatedBy (FT),
      fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy
      (FESC) and linesTerminatedBy (LT) in following manner:

      - default: no quoting, tab-separated, lf line endings. (LT=<LF>,
        FESC='\', FT=<TAB>, FE=<empty>, FOE=false)
      - csv: optionally quoted, comma-separated, crlf line endings.
        (LT=<CR><LF>, FESC='\', FT=",", FE='"', FOE=true)
      - tsv: optionally quoted, tab-separated, crlf line endings. (LT=<CR><LF>,
        FESC='\', FT=<TAB>, FE='"', FOE=true)
      - json: one JSON document per line. (LT=<LF>, FESC=<empty>, FT=<LF>,
        FE=<empty>, FOE=false)
      - csv-unix: fully quoted, comma-separated, lf line endings. (LT=<LF>,
        FESC='\', FT=",", FE='"', FOE=false)

      Example input data for dialects:

      - default:
      1<TAB>20.1000<TAB>foo said: "Where is my bar?"<LF>
      2<TAB>-12.5000<TAB>baz said: "Where is my \<TAB> char?"<LF>
      - csv:
      1,20.1000,"foo said: \"Where is my bar?\""<CR><LF>
      2,-12.5000,"baz said: \"Where is my <TAB> char?\""<CR><LF>
      - tsv:
      1<TAB>20.1000<TAB>"foo said: \"Where is my bar?\""<CR><LF>
      2<TAB>-12.5000<TAB>"baz said: \"Where is my <TAB> char?\""<CR><LF>
      - json:
      {"id_int": 1, "value_float": 20.1000, "text_text": "foo said: \"Where is
      my bar?\""}<LF>
      {"id_int": 2, "value_float": -12.5000, "text_text": "baz said: \"Where is
      my \u000b char?\""}<LF>
      - csv-unix:
      "1","20.1000","foo said: \"Where is my bar?\""<LF>
      "2","-12.5000","baz said: \"Where is my <TAB> char?\""<LF>

      If the schema is not provided, an active schema on the global session, if
      set, will be used.

      If the input values are not necessarily enclosed within fieldsEnclosedBy,
      set fieldsOptionallyEnclosed to true.

      If you specify one separator that is the same as or a prefix of another,
      LOAD DATA INFILE cannot interpret the input properly.

      Connection options set in the global session, such as compression,
      ssl-mode, etc. are used in parallel connections.

      Each parallel connection sets the following session variables:

      - SET unique_checks = 0
      - SET foreign_key_checks = 0
      - SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED

#@<OUT> util load_dump help
NAME
      load_dump - Loads database dumps created by MySQL Shell.

SYNTAX
      util.load_dump(url[, options])

WHERE
      url: URL or path to the dump directory
      options: Dictionary with load options

DESCRIPTION
      url can be one of:

      - /path/to/file - Path to a locally or remotely (e.g. in OCI Object
        Storage) accessible file or directory
      - file:///path/to/file - Path to a locally accessible file or directory
      - http[s]://host.domain[:port]/path/to/file - Location of a remote file
        accessible through HTTP(s) (import_table() only)

      If the osBucketName option is given, the path argument must specify a
      plain path in that OCI (Oracle Cloud Infrastructure) Object Storage
      bucket.

      load_dump() will load a dump from the specified path. It transparently
      handles compressed files and directly streams data when loading from
      remote storage (currently HTTP and OCI Object Storage). If the
      'waitDumpTimeout' option is set, it will load a dump on-the-fly, loading
      table data chunks as the dumper produces them.

      Table data will be loaded in parallel using the configured number of
      threads (4 by default). Multiple threads per table can be used if the
      dump was created with table chunking enabled. Data loads are scheduled
      across threads in a way that tries to maximize parallelism, while also
      minimizing lock contention from concurrent loads to the same table. If
      there are more tables than threads, different tables will be loaded per
      thread, larger tables first. If there are more threads than tables, then
      chunks from larger tables will be proportionally assigned more threads.

      LOAD DATA LOCAL INFILE is used to load table data and thus, the
      'local_infile' MySQL global setting must be enabled.

      Resuming

      The load command will store progress information into a file for each
      step of the loading process, including successfully completed and
      interrupted/failed ones. If that file already exists, its contents will
      be used to skip steps that have already been completed and retry those
      that failed or didn't start yet.

      When resuming, table chunks that have started being loaded but didn't
      finish are loaded again. Duplicate rows are discarded by the server.
      Tables that do not have unique keys are truncated before the load is
      resumed.

      IMPORTANT: Resuming assumes that no changes have been made to the
      partially loaded data between the failure and the retry. Resuming after
      external changes has undefined behavior and may lead to data loss.

      The progress state file has a default name of
      load-progress.<server_uuid>.json and is written to the same location as
      the dump. If 'progressFile' is specified, progress will be written to a
      local file at the given path. Setting it to '' will disable progress
      tracking and resuming.

      If the 'resetProgress' option is enabled, progress information from
      previous load attempts of the dump to the destination server is discarded
      and the load is restarted. You may use this option to retry loading the
      whole dump from the beginning. However, changes made to the database are
      not reverted, so previously loaded objects should be manually dropped
      first.

      Options dictionary:

      - analyzeTables: "off", "on", "histogram" (default: off) - If 'on',
        executes ANALYZE TABLE for all tables, once loaded. If set to
        'histogram', only tables that have histogram information stored in the
        dump will be analyzed. This option can be used even if all 'load'
        options are disabled.
      - defaultCharacterSet: string (default taken from dump) - Specifies the
        character set to be used for loading the dump. By default, the same
        character set used for dumping will be used (utf8mb4 if not set at
        dump).
      - deferTableIndexes: bool (default: true) - Defer all but PRIMARY index
        creation for table until data has already been loaded, which should
        improve performance.
      - dryRun: bool (default: false) - Scans the dump and prints everything
        that would be performed, without actually doing so.
      - excludeSchemas: array of strings (default not set) - Skip loading
        specified schemas from the dump.
      - excludeTables: array of strings (default not set) - Skip loading
        specified tables from the dump. Strings are in format schema.table or
        `schema`.`table`.
      - ignoreExistingObjects: bool (default false) - Load the dump even if it
        contains objects that already exist in the target database.
      - ignoreVersion: bool (default false) - Load the dump even if the major
        version number of the server where it was created is different from
        where it will be loaded.
      - includeSchemas: array of strings (default not set) - Loads only the
        specified schemas from the dump. By default, all schemas are included.
      - includeTables: array of strings (default not set) - Loads only the
        specified tables from the dump. Strings are in format schema.table or
        `schema`.`table`. By default, all tables from all schemas are included.
      - loadData: bool (default: true) - Loads table data from the dump.
      - loadDdl: bool (default: true) - Executes DDL/SQL scripts in the dump.
      - loadUsers: bool (default: false) - Executes SQL scripts for user
        accounts, roles and grants contained in the dump. Note: statements for
        the current user will be skipped.
      - progressFile: path (default: <server_uuid>.progress) - Stores load
        progress information in the given local file path.
      - resetProgress: bool (default: false) - Discards progress information of
        previous load attempts to the destination server and loads the whole
        dump again.
      - showProgress: bool (default: true if stdout is a tty, false otherwise)
        - Enable or disable import progress information.
      - skipBinlog: bool (default: false) - Disables the binary log for the
        MySQL sessions used by the loader (set sql_log_bin=0).
      - threads: int (default: 4) - Number of threads to use to import table
        data.
      - waitDumpTimeout: int (default: 0) - Loads a dump while it's still being
        created. Once all available tables are processed the command will
        either wait for more data, the dump is marked as completed or the given
        timeout passes. <= 0 disables waiting.
      - osBucketName: string (default: not set) - Use specified OCI bucket for
        the location of the dump.
      - osNamespace: string (default: not set) - Specify the OCI namespace
        (tenancy name) where the OCI bucket is located.
      - ociConfigFile: string (default: not set) - Use the specified OCI
        configuration file instead of the one in the default location.
      - ociProfile: string (default: not set) - Use the specified OCI profile
        instead of the default one.

      Connection options set in the global session, such as compression,
      ssl-mode, etc. are inherited by load sessions.

      Examples:
      util.load_dump("sakila_dump")

      util.load_dump("mysql/sales", {
          "osBucketName": "mybucket",    // OCI Object Storage bucket
          "waitDumpTimeout": 1800        // wait for new data for up to 30mins
      })

//@<OUT> Shell Help
MySQL Shell <<<__mysh_full_version>>>

Copyright (c) 2016, <<<__package_year>>>, Oracle and/or its affiliates.
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Usage: mysqlsh [OPTIONS] [URI]
       mysqlsh [OPTIONS] [URI] -f <path> [<script-args>...]
       mysqlsh [OPTIONS] [URI] --cluster|--replicaset
       mysqlsh [OPTIONS] [URI] -- <object> <method> [<method-args>...]
       mysqlsh [OPTIONS] [URI] --dba enableXProtocol

  -?, --help                       Display this help and exit.
  --                               Triggers API Command Line integration, which
                                   allows execution of methods of the Shell
                                   global objects from command line using
                                   syntax:
                                     -- <object> <method> [arguments]
                                   For more details execute '\? cmdline' inside
                                   of the Shell.
  -e, --execute=<cmd>              Execute command and quit.
  -c, --pyc=<cmd>                  Execute Python command and quit. Any options
                                   specified after this are used as arguments
                                   of the processed command.
  -f, --file=<file>                Specify a file to process in batch mode. Any
                                   options specified after this are used as
                                   arguments of the processed file.
  --ssh=<value>                    Make SSH tunnel connection usingUniform
                                   Resource Identifier. Format:
                                   [user[:pass]@]host[:port]
  --ssh-identity-file=<file>       File from which the private key for public
                                   key authentication is read.
  --ssh-config-file=<file>         Specify a custom path for SSH configuration.
?{__os_type == 'windows'}
  --plugin-authentication-kerberos-client-mode=<mode>
                                   Allows defining the kerberos client mode
                                   (SSPI, GSSAPI) when using kerberos
                                   authentication.
?{}
  --oci-config-file=<file>         Allows defining the OCI configuration file
                                   for OCI authentication.
  --authentication-oci-client-config-profile=<name>
                                   Allows defining the OCI profile used from
                                   the configuration for client side OCI
                                   authentication.
  --authentication-openid-connect-client-id-token-file=<path>
                                   Allows defining the file path to an OpenId
                                   Connect authorization token file when using
                                   OpenId Connect authentication
  --uri=<value>                    Connect to Uniform Resource Identifier.
                                   Format: [user[:pass]@]host[:port][/db]
  -h, --host=<name>                Connect to host.
  -P, --port=<#>                   Port number to use for connection.
  --connect-timeout=<ms>           Connection timeout in milliseconds.
?{__os_type != 'windows'}
  -S, --socket[=<sock>]            Socket name to use. If no value is provided
                                   will use default UNIX socket path.
?{}
?{__os_type == 'windows'}
  -S, --socket=<sock>              Pipe name to use (only classic sessions).
?{}
  -u, --user=<name>                User for the connection to the server.
  --password=[<pass>]              Password to use when connecting to server.
                                   If password is empty, connection will be
                                   made without using a password.
  -p, --password                   Request password prompt to set the password
  --password1[=<pass>]             Password for first factor authentication
                                   plugin.
  --password2[=<pass>]             Password for second factor authentication
                                   plugin.
  --password3[=<pass>]             Password for third factor authentication
                                   plugin.
  -C, --compress[=<value>]         Use compression in client/server protocol.
                                   Valid values: 'REQUIRED', 'PREFFERED',
                                   'DISABLED', 'True', 'False', '1', and '0'.
                                   Boolean values map to REQUIRED and DISABLED
                                   respectively. By default compression is set
                                   to DISABLED in classic protocol and
                                   PREFERRED in X protocol connections.
  --compression-algorithms=<list>  Use compression algorithm in server/client
                                   protocol. Expects comma separated list of
                                   algorithms. Supported algorithms include
                                   'zstd', 'zlib', 'lz4' (X protocol only), and
                                   'uncompressed', which if appears in a list,
                                   causes connection to succeed even if
                                   compression negotiation fails.
  --compression-level=<int>        Use this compression level in the
  --zstd-compression-level=<int>   client/server protocol. Supported by X
                                   protocol and zstd compression in classic
                                   protocol.
  -D, --schema=<name>              Schema to use.
  --database=<name>                Same as --schema.
  --register-factor=<name>         Specifies authentication factor, for which
                                   registration needs to be done.
  --plugin-authentication-webauthn-client-preserve-privacy[=<value>]
                                   Allows selection of discoverable credential
                                   to be used for signing challenge. If not
                                   enabled, challenge is signed by all
                                   credentials for given relying party.
  --mx, --mysqlx                   Uses connection data to create an X protocol
                                   session.
  --mc, --mysql                    Uses connection data to create a classic
                                   session.
  --redirect-primary               Ensure that the target server is part of an
                                   InnoDB cluster or ReplicaSet and if it is
                                   not a primary, find the primary and connect
                                   to it.
  --redirect-secondary             Ensure that the target server is part of an
                                   InnoDB Cluster or ReplicaSet and if it is
                                   not a secondary, find a secondary and
                                   connect to it.
  --cluster                        Ensure that the target server is part of an
                                   InnoDB Cluster and if so, set the 'cluster'
                                   global variable. Also sets 'clusterset' if
                                   the cluster is part of a ClusterSet.
  --replicaset                     Ensure that the target server is part of an
                                   InnoDB ReplicaSet and if so, set the 'rs'
                                   global variable.
  --sql                            Start in SQL mode, auto-detecting the
                                   protocol to use if it is not specified as
                                   part of the connection information.
  --sqlc                           Start in SQL mode using a classic session.
  --sqlx                           Start in SQL mode using an X protocol
                                   session.
  --js, --javascript               Start in JavaScript mode.
  --py, --python                   Start in Python mode.
  --pym <module>                   Run Python library module as a script.
                                   Remaining args are forwarded to it.
  --json[=<format>]                Produce output in JSON format. Allowed
                                   values: raw, pretty, and off. If no format
                                   is specified pretty format is produced.
  --table                          Produce output in table format (default for
                                   interactive mode). This option can be used
                                   to force that format when running in batch
                                   mode.
  --tabbed                         Produce output in tab separated format
                                   (default for batch mode). This option can be
                                   used to force that format when running in
                                   interactive mode.
  -E, --vertical                   Print the output of a query (rows)
                                   vertically.
  --result-format=<value>          Determines format of results. Allowed
                                   values: [table, tabbed, vertical, json,
                                   ndjson, json/raw, json/array, json/pretty].
  --get-server-public-key          Request public key from the server required
                                   for RSA key pair-based password exchange.
                                   Use when connecting to MySQL 8.0 servers
                                   with classic sessions with SSL mode DISABLED.
  --server-public-key-path=<p>     The path name to a file containing a
                                   client-side copy of the public key required
                                   by the server for RSA key pair-based
                                   password exchange. Use when connecting to
                                   MySQL 8.0 servers with classic sessions with
                                   SSL mode DISABLED.
  -i, --interactive[=full]         To use in batch mode. It forces emulation of
                                   interactive mode processing. Each line on
                                   the batch is processed as if it were in
                                   interactive mode.
  --force                          In SQL batch mode, forces processing to
                                   continue if an error is found.
  --log-file=<path>                Override location of the Shell log file.
  --log-level=<value>              Set logging level. The log level value must
                                   be an integer between 1 and 8 or any of
                                   [none, internal, error, warning, info,
                                   debug, debug2, debug3] respectively.
  --dba-log-sql[={0|1|2}]          Log SQL statements executed by AdminAPI
                                   operations: 0 - logging disabled; 1 - log
                                   statements other than SELECT and SHOW; 2 -
                                   log all statements. Option takes precedence
                                   over --log-sql in Dba.* context if enabled.
  --log-sql=off|error|on|all|unfiltered
                                   Log SQL statements: off - none of SQL
                                   statements will be logged; (default) error -
                                   SQL statement with error message will be
                                   logged only when error occurs; on - All SQL
                                   statements will be logged except these which
                                   match any of logSql.ignorePattern and
                                   logSql.ignorePatternUnsafe glob pattern; all
                                   - All SQL statements will be logged except
                                   these which match any of
                                   logSql.ignorePatternUnsafe glob pattern;
                                   unfiltered - All SQL statements will be
                                   logged.
  --syslog                         Log filtered interactive commands to the
                                   system log. Filtering of commands depends on
                                   the patterns supplied via histignore option.
  --verbose[={0|1|2|3|4}]          Enable diagnostic message output to the
                                   console: 0 - display no messages; 1 -
                                   display error, warning and informational
                                   messages; 2, 3, 4 - include higher levels of
                                   debug messages. If level is not given, 1 is
                                   assumed.
  --passwords-from-stdin           Read passwords from stdin instead of the
                                   console.
  --show-warnings={true|false}     Automatically display SQL warnings on SQL
                                   mode if available.
  --column-type-info               Display column type information in SQL mode.
                                   Please be aware that output may depend on
                                   the protocol you are using to connect to the
                                   server, e.g. DbType field is approximated
                                   when using X protocol.
  --histignore=<filters>           Shell's history ignore list.
  --pager=<value>                  Pager used to display text output of
                                   statements executed in SQL mode as well as
                                   some other selected commands. Pager can be
                                   manually enabled in scripting modes. If you
                                   don't supply an option, the default pager is
                                   taken from your ENV variable PAGER. This
                                   option only works in interactive mode. This
                                   option is disabled by default.
  --mysql-plugin-dir[=<path>]      Directory for client-side authentication
                                   plugins.
  --name-cache                     Enable database name caching for
                                   autocompletion and DevAPI (default).
  -A, --no-name-cache              Disable automatic database name caching for
                                   autocompletion and DevAPI. Use \rehash to
                                   load DB names manually.
  --nw, --no-wizard                Disables wizard mode.
  --no-password                    Sets empty password and disables prompt for
                                   password.
  -V, --version                    Prints the version of MySQL Shell.
  --ssl-key=<file_name>            The path to the SSL private key file in PEM
                                   format.
  --ssl-cert=<file_name>           The path to the SSL public key certificate
                                   file in PEM format.
  --ssl-ca=<file_name>             The path to the certificate authority file
                                   in PEM format.
  --ssl-capath=<dir_name>          The path to the directory that contains the
                                   certificate authority files in PEM format.
  --ssl-cipher=<cipher_list>       The list of permissible encryption ciphers
                                   for connections that use TLS protocols up
                                   through TLSv1.2.
  --ssl-crl=<file_name>            The path to the file containing certificate
                                   revocation lists in PEM format.
  --ssl-crlpath=<dir_name>         The path to the directory that contains
                                   certificate revocation-list files in PEM
                                   format.
  --ssl-mode=<mode>                SSL mode to use. Allowed values:
                                   DISABLED,PREFERRED, REQUIRED, VERIFY_CA,
                                   VERIFY_IDENTITY.
  --tls-version=<version>          TLS version to use. Allowed values: TLSv1.2,
                                   TLSv1.3.
  --tls-ciphersuites=<name>        TLS v1.3 cipher to use.
  --auth-method=<method>           Authentication method to use. In case of
                                   classic session, this is the name of the
                                   authentication plugin to use, i.e.
                                   caching_sha2_password. In case of X protocol
                                   session, it should be one of: AUTO,
                                   FROM_CAPABILITIES, FALLBACK, MYSQL41, PLAIN,
                                   SHA256_MEMORY.
  --disable-plugins                Diable loading user plugins.
  --dba=enableXProtocol            Enable the X protocol in the target server.
                                   Requires a connection using classic session.
                                   Deprecated.
  --quiet-start[={1|2}]            Avoids printing information when the shell
                                   is started. A value of 1 will prevent
                                   printing the shell version information. A
                                   value of 2 will prevent printing any
                                   information unless it is an error. If no
                                   value is specified uses 1 as default.
  --credential-store-helper=<h>    Specifies the helper which is going to be
                                   used to store/retrieve the passwords.
  --save-passwords=<value>         Controls automatic storage of passwords.
                                   Allowed values are: always, prompt, never.

The following options may be given as the first argument:
--print-defaults        Print the program argument list and exit.
--no-defaults           Don't read default options from any option file,
                        except for login file.
--defaults-file=#       Only read default options from the given file #.
--defaults-extra-file=# Read this file after the global files are read.
--defaults-group-suffix=#
                        Also read groups with concat(group, suffix)
--login-path=#          Read this path from the login file.
The following groups are read: mysqlsh client

Usage examples:
$ mysqlsh --login-path=server1 --sql
$ mysqlsh root@localhost/schema
$ mysqlsh mysqlx://root@some.server:3307/world_x
$ mysqlsh --uri root@localhost --py -f sample.py sample param
$ mysqlsh root@targethost:33070 -s world_x -f sample.js
$ mysqlsh -- util check-for-server-upgrade root@localhost --output-format=JSON

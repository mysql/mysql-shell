//@<OUT> Shell Help {__os_type != 'windows'}
MySQL Shell <<<__mysh_full_version>>>

Copyright (c) 2016, <<<__package_year>>>, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Usage: mysqlsh [OPTIONS] [URI]
       mysqlsh [OPTIONS] [URI] -f <path> [script args...]
       mysqlsh [OPTIONS] [URI] --dba [command]
       mysqlsh [OPTIONS] [URI] --cluster
       mysqlsh [OPTIONS] [URI] -- <object> <method> [method args...]
       mysqlsh [OPTIONS] [URI] --import file|- [collection] | [table [, column]

  -?, --help                    Display this help and exit.
  --                            Triggers API Command Line integration, which
                                allows execution of methods of the Shell global
                                objects from command line using syntax:
                                  -- <object> <method> [arguments]
                                For more details execute '\? cmdline' inside of
                                the Shell.
  -e, --execute=<cmd>           Execute command and quit.
  -f, --file=file               Process file.
  --uri=value                   Connect to Uniform Resource Identifier. Format:
                                [user[:pass]@]host[:port][/db]
  -h, --host=name               Connect to host.
  -P, --port=#                  Port number to use for connection.
  --connect-timeout=#           Connection timeout in milliseconds.
  -S, --socket[=sock]           Socket name to use. If no value is provided
                                will use default UNIX socket path.
  -u, --user=name               User for the connection to the server.
  --password=[pass]             Password to use when connecting to server. If
                                password is empty, connection will be made
                                without using a password.
  -p, --password                Request password prompt to set the password
  -C, --compress                Use compression in client/server protocol.
  --import file collection      Import JSON documents from file to collection
  --import file table [column]  or table in MySQL Server. Set file to - if you
                                want to read the data from stdin. Requires a
                                default schema on the connection options.
  -D, --schema=name             Schema to use.
  --database=name               see above
  --recreate-schema             Drop and recreate the specified schema. Schema
                                will be deleted if it exists!
  --mx, --mysqlx                Uses connection data to create Creating an X
                                protocol session.
  --mc, --mysql                 Uses connection data to create a Classic
                                Session.
  --redirect-primary            Connect to the primary of the group. For use
                                with InnoDB clusters.
  --redirect-secondary          Connect to a secondary of the group. For use
                                with InnoDB clusters.
  --cluster                     Enable cluster management, setting the cluster
                                global variable.
  --sql                         Start in SQL mode.
  --sqlc                        Start in SQL mode using a classic session.
  --sqlx                        Start in SQL mode using Creating an X protocol
                                session.
  --js, --javascript            Start in JavaScript mode.
  --py, --python                Start in Python mode.
  --json[=format]               Produce output in JSON format, allowed values:
                                raw, pretty, and off. If no format is specified
                                pretty format is produced.
  --table                       Produce output in table format (default for
                                interactive mode). This option can be used to
                                force that format when running in batch mode.
  --tabbed                      Produce output in tab separated format (default
                                for batch mode). This option can be used to
                                force that format when running in interactive
                                mode.
  -E, --vertical                Print the output of a query (rows) vertically.
  --result-format=value         Determines format of results. Valid values:
                                [table, tabbed, vertical, json, ndjson,
                                json/raw, json/array, json/pretty].
  --get-server-public-key       Request public key from the server required for
                                RSA key pair-based password exchange. Use when
                                connecting to MySQL 8.0 servers with classic
                                MySQL sessions with SSL mode DISABLED.
  --server-public-key-path=path The path name to a file containing a
                                client-side copy of the public key required by
                                the server for RSA key pair-based password
                                exchange. Use when connecting to MySQL 8.0
                                servers with classic MySQL sessions with SSL
                                mode DISABLED.
  -i, --interactive[=full]      To use in batch mode. It forces emulation of
                                interactive mode processing. Each line on the
                                batch is processed as if it were in interactive
                                mode.
  --force                       To use in SQL batch mode, forces processing to
                                continue if an error is found.
  --log-level=value             The log level value must be an integer between
                                1 and 8 or any of [none, internal, error,
                                warning, info, debug, debug2, debug3]
                                respectively.
  --dba-log-sql[=0|1|2]         Log SQL statements executed by AdminAPI
                                operations: 0 - logging disabled; 1 - log
                                statements other than SELECT and SHOW; 2 - log
                                all statements.
  --verbose[=level]             Verbose output level. Enable diagnostic message
                                output. If level is given, it can go up to 4
                                for maximum verbosity, otherwise 1 is assumed.
  --passwords-from-stdin        Read passwords from stdin instead of the tty.
  --show-warnings=<true|false>  Automatically display SQL warnings on SQL mode
                                if available.
  --column-type-info            Display column type information in SQL mode.
                                Please be aware that output may depend on the
                                protocol you are using to connect to the
                                server, e.g. DbType field is approximated when
                                using X protocol.
  --histignore=filters          Shell's history ignore list.
  --pager=value                 Pager used to display text output of statements
                                executed in SQL mode as well as some other
                                selected commands. Pager can be manually
                                enabled in scripting modes. If you don't supply
                                an option, the default pager is taken from your
                                ENV variable PAGER. This option only works in
                                interactive mode. This option is disabled by
                                default.
  --name-cache                  Enable database name caching for autocompletion
                                and DevAPI (default).
  -A, --no-name-cache           Disable automatic database name caching for
                                autocompletion and DevAPI. Use \rehash to load
                                DB names manually.
  --nw, --no-wizard             Disables wizard mode.
  --no-password                 Sets empty password and disables prompt for
                                password.
  -V, --version                 Prints the version of MySQL Shell.
  --ssl-key=name                X509 key in PEM format.
  --ssl-cert=name               X509 cert in PEM format.
  --ssl-ca=name                 CA file in PEM format.
  --ssl-capath=dir              CA directory.
  --ssl-cipher=name             SSL Cipher to use.
  --ssl-crl=name                Certificate revocation list.
  --ssl-crlpath=dir             Certificate revocation list path.
  --ssl-mode=mode               SSL mode to use, allowed values:
                                DISABLED,PREFERRED, REQUIRED, VERIFY_CA,
                                VERIFY_IDENTITY.
  --tls-version=version         TLS version to use, permitted values are:
                                TLSv1, TLSv1.1.
  --auth-method=method          Authentication method to use.
  --dba=enableXProtocol         Enable the X Protocol in the server connected
                                to. Must be used with --mysql.
  --quiet-start[=1]             Avoids printing information when the shell is
                                started. A value of 1 will prevent printing the
                                shell version information. A value of 2 will
                                prevent printing any information unless it is
                                an error. If no value is specified uses 1 as
                                default.
  --debug=#                     Debug options for DBUG package.
?{__with_oci==1}
  --oci[=profile]               Starts the shell ready to work with OCI. A
                                wizard to configure the given profile will be
                                launched if the profile is not configured. If
                                no profile is specified the 'DEFAULT' profile
                                will be used.
?{}
  --credential-store-helper=val Specifies the helper which is going to be used
                                to store/retrieve the passwords.
  --save-passwords=value        Controls automatic storage of passwords.
                                Allowed values are: always, prompt, never.

Usage examples:
$ mysqlsh root@localhost/schema
$ mysqlsh mysqlx://root@some.server:3307/world_x
$ mysqlsh --uri root@localhost --py -f sample.py sample param
$ mysqlsh root@targethost:33070 -s world_x -f sample.js
$ mysqlsh -- util check-for-server-upgrade root@localhost --output-format=JSON
$ mysqlsh mysqlx://user@host/db --import ~/products.json shop

//@<OUT> Shell Help {__os_type == 'windows'}
MySQL Shell <<<__mysh_full_version>>>

Copyright (c) 2016, <<<__package_year>>>, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Usage: mysqlsh [OPTIONS] [URI]
       mysqlsh [OPTIONS] [URI] -f <path> [script args...]
       mysqlsh [OPTIONS] [URI] --dba [command]
       mysqlsh [OPTIONS] [URI] --cluster
       mysqlsh [OPTIONS] [URI] -- <object> <method> [method args...]
       mysqlsh [OPTIONS] [URI] --import file|- [collection] | [table [, column]

  -?, --help                    Display this help and exit.
  --                            Triggers API Command Line integration, which
                                allows execution of methods of the Shell global
                                objects from command line using syntax:
                                  -- <object> <method> [arguments]
                                For more details execute '\? cmdline' inside of
                                the Shell.
  -e, --execute=<cmd>           Execute command and quit.
  -f, --file=file               Process file.
  --uri=value                   Connect to Uniform Resource Identifier. Format:
                                [user[:pass]@]host[:port][/db]
  -h, --host=name               Connect to host.
  -P, --port=#                  Port number to use for connection.
  --connect-timeout=#           Connection timeout in milliseconds.
  -S, --socket=sock             Pipe name to use (only classic sessions).
  -u, --user=name               User for the connection to the server.
  --password=[pass]             Password to use when connecting to server. If
                                password is empty, connection will be made
                                without using a password.
  -p, --password                Request password prompt to set the password
  -C, --compress                Use compression in client/server protocol.
  --import file collection      Import JSON documents from file to collection
  --import file table [column]  or table in MySQL Server. Set file to - if you
                                want to read the data from stdin. Requires a
                                default schema on the connection options.
  -D, --schema=name             Schema to use.
  --database=name               see above
  --recreate-schema             Drop and recreate the specified schema. Schema
                                will be deleted if it exists!
  --mx, --mysqlx                Uses connection data to create Creating an X
                                protocol session.
  --mc, --mysql                 Uses connection data to create a Classic
                                Session.
  --redirect-primary            Connect to the primary of the group. For use
                                with InnoDB clusters.
  --redirect-secondary          Connect to a secondary of the group. For use
                                with InnoDB clusters.
  --cluster                     Enable cluster management, setting the cluster
                                global variable.
  --sql                         Start in SQL mode.
  --sqlc                        Start in SQL mode using a classic session.
  --sqlx                        Start in SQL mode using Creating an X protocol
                                session.
  --js, --javascript            Start in JavaScript mode.
  --py, --python                Start in Python mode.
  --json[=format]               Produce output in JSON format, allowed values:
                                raw, pretty, and off. If no format is specified
                                pretty format is produced.
  --table                       Produce output in table format (default for
                                interactive mode). This option can be used to
                                force that format when running in batch mode.
  --tabbed                      Produce output in tab separated format (default
                                for batch mode). This option can be used to
                                force that format when running in interactive
                                mode.
  -E, --vertical                Print the output of a query (rows) vertically.
  --result-format=value         Determines format of results. Valid values:
                                [table, tabbed, vertical, json, ndjson,
                                json/raw, json/array, json/pretty].
  --get-server-public-key       Request public key from the server required for
                                RSA key pair-based password exchange. Use when
                                connecting to MySQL 8.0 servers with classic
                                MySQL sessions with SSL mode DISABLED.
  --server-public-key-path=path The path name to a file containing a
                                client-side copy of the public key required by
                                the server for RSA key pair-based password
                                exchange. Use when connecting to MySQL 8.0
                                servers with classic MySQL sessions with SSL
                                mode DISABLED.
  -i, --interactive[=full]      To use in batch mode. It forces emulation of
                                interactive mode processing. Each line on the
                                batch is processed as if it were in interactive
                                mode.
  --force                       To use in SQL batch mode, forces processing to
                                continue if an error is found.
  --log-level=value             The log level value must be an integer between
                                1 and 8 or any of [none, internal, error,
                                warning, info, debug, debug2, debug3]
                                respectively.
  --dba-log-sql[=0|1|2]         Log SQL statements executed by AdminAPI
                                operations: 0 - logging disabled; 1 - log
                                statements other than SELECT and SHOW; 2 - log
                                all statements.
  --verbose[=level]             Verbose output level. Enable diagnostic message
                                output. If level is given, it can go up to 4
                                for maximum verbosity, otherwise 1 is assumed.
  --passwords-from-stdin        Read passwords from stdin instead of the tty.
  --show-warnings=<true|false>  Automatically display SQL warnings on SQL mode
                                if available.
  --column-type-info            Display column type information in SQL mode.
                                Please be aware that output may depend on the
                                protocol you are using to connect to the
                                server, e.g. DbType field is approximated when
                                using X protocol.
  --histignore=filters          Shell's history ignore list.
  --pager=value                 Pager used to display text output of statements
                                executed in SQL mode as well as some other
                                selected commands. Pager can be manually
                                enabled in scripting modes. If you don't supply
                                an option, the default pager is taken from your
                                ENV variable PAGER. This option only works in
                                interactive mode. This option is disabled by
                                default.
  --name-cache                  Enable database name caching for autocompletion
                                and DevAPI (default).
  -A, --no-name-cache           Disable automatic database name caching for
                                autocompletion and DevAPI. Use \rehash to load
                                DB names manually.
  --nw, --no-wizard             Disables wizard mode.
  --no-password                 Sets empty password and disables prompt for
                                password.
  -V, --version                 Prints the version of MySQL Shell.
  --ssl-key=name                X509 key in PEM format.
  --ssl-cert=name               X509 cert in PEM format.
  --ssl-ca=name                 CA file in PEM format.
  --ssl-capath=dir              CA directory.
  --ssl-cipher=name             SSL Cipher to use.
  --ssl-crl=name                Certificate revocation list.
  --ssl-crlpath=dir             Certificate revocation list path.
  --ssl-mode=mode               SSL mode to use, allowed values:
                                DISABLED,PREFERRED, REQUIRED, VERIFY_CA,
                                VERIFY_IDENTITY.
  --tls-version=version         TLS version to use, permitted values are:
                                TLSv1, TLSv1.1.
  --auth-method=method          Authentication method to use.
  --dba=enableXProtocol         Enable the X Protocol in the server connected
                                to. Must be used with --mysql.
  --quiet-start[=1]             Avoids printing information when the shell is
                                started. A value of 1 will prevent printing the
                                shell version information. A value of 2 will
                                prevent printing any information unless it is
                                an error. If no value is specified uses 1 as
                                default.
  --debug=#                     Debug options for DBUG package.
?{__with_oci==1}
  --oci[=profile]               Starts the shell ready to work with OCI. A
                                wizard to configure the given profile will be
                                launched if the profile is not configured. If
                                no profile is specified the 'DEFAULT' profile
                                will be used.
?{}
  --credential-store-helper=val Specifies the helper which is going to be used
                                to store/retrieve the passwords.
  --save-passwords=value        Controls automatic storage of passwords.
                                Allowed values are: always, prompt, never.

Usage examples:
$ mysqlsh root@localhost/schema
$ mysqlsh mysqlx://root@some.server:3307/world_x
$ mysqlsh --uri root@localhost --py -f sample.py sample param
$ mysqlsh root@targethost:33070 -s world_x -f sample.js
$ mysqlsh -- util check-for-server-upgrade root@localhost --output-format=JSON
$ mysqlsh mysqlx://user@host/db --import ~/products.json shop

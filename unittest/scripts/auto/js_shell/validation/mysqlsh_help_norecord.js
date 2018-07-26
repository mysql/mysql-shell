//@<OUT> Shell Help
MySQL Shell <<<__mysh_full_version>>>

Copyright (c) 2016, <<<__current_year>>>, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Usage: mysqlsh [OPTIONS] [URI]
       mysqlsh [OPTIONS] [URI] -f <path> [script args...]
       mysqlsh [OPTIONS] [URI] --dba [command]
       mysqlsh [OPTIONS] [URI] --cluster

  -?, --help                    Display this help and exit.
  -e, --execute=<cmd>           Execute command and quit.
  -f, --file=file               Process file.
  --uri=value                   Connect to Uniform Resource Identifier. Format:
                                [user[:pass]@]host[:port][/db]
  -h, --host=name               Connect to host.
  -P, --port=#                  Port number to use for connection.
  -S, --socket=sock             Socket name to use in UNIX, pipe name to use in
                                Windows (only classic sessions).
  -u, --dbuser=name             User for the connection to the server.
  --user=name                   see above
  -p, --password[=name]         Password to use when connecting to server.
  --dbpassword[=name]           see above
  -p                            Request password prompt to set the password
  -D, --schema=name             Schema to use.
  --database=name               see above
  --recreate-schema             Drop and recreate the specified schema. Schema
                                will be deleted if it exists!
  -mx, --mysqlx                 Uses connection data to create Creating an X
                                protocol session.
  -mc, --mysql                  Uses connection data to create a Classic
                                Session.
  -ma                           Uses the connection data to create the session
                                with automatic protocol detection.
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
  --json[=format]               Produce output in JSON format, allowed
                                values:raw, pretty. If no format is specified
                                pretty format is produced.
  --table                       Produce output in table format (default for
                                interactive mode). This option can be used to
                                force that format when running in batch mode.
  --tabbed                      Produce output in tab separated format (default
                                for batch mode). This option can be used to
                                force that format when running in interactive
                                mode.
  -E, --vertical                Print the output of a query (rows) vertically.
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
  --passwords-from-stdin        Read passwords from stdin instead of the tty.
  --show-warnings               Automatically display SQL warnings on SQL mode
                                if available.
  --histignore=filters          Shell's history ignore list.
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
  --credential-store-helper     Specifies the helper which is going to be used
                                to store/retrieve the passwords.
  --save-passwords              Controls automatic storage of passwords.

Usage examples:
$ mysqlsh root@localhost/schema
$ mysqlsh mysqlx://root@some.server:3307/world_x
$ mysqlsh --uri root@localhost --py -f sample.py sample param
$ mysqlsh root@targethost:33070 -s world_x -f sample.js


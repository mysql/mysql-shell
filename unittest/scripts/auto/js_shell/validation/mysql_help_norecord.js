//@<OUT> mysql help
NAME
      mysql - Encloses the functions and classes available to interact with a
              MySQL Server using the traditional MySQL Protocol.

DESCRIPTION
      Use this module to create a session using the traditional MySQL Protocol,
      for example for MySQL Servers where the X Protocol is not available.

      Note that the API interface on this module is very limited, even you can
      load schemas, tables and views as objects there are no operations
      available on them.

      The purpose of this module is to allow SQL Execution on MySQL Servers
      where the X Protocol is not enabled.

      To use the properties and functions available on this module you first
      need to import it.

      When running the shell in interactive mode, this module is automatically
      imported.

FUNCTIONS
      getClassicSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      getSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      help([member])
            Provides help about this module and it's members

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.

//@<OUT> getClassicSession help
NAME
      getClassicSession - Opens a classic MySQL protocol session to a MySQL
                          server.

SYNTAX
      mysql.getClassicSession(connectionData[, password])

WHERE
      connectionData: The connection data for the session
      password: Password for the session

RETURNS
      A ClassicSession

DESCRIPTION
      A ClassicSession object uses the traditional MySQL Protocol to allow
      executing operations on the connected MySQL Server.

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

//@<OUT> getSession help
NAME
      getSession - Opens a classic MySQL protocol session to a MySQL server.

SYNTAX
      mysql.getSession(connectionData[, password])

WHERE
      connectionData: The connection data for the session
      password: Password for the session

RETURNS
      A ClassicSession

DESCRIPTION
      A ClassicSession object uses the traditional MySQL Protocol to allow
      executing operations on the connected MySQL Server.

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

//@<OUT> mysql help on help
NAME
      help - Provides help about this module and it's members

SYNTAX
      mysql.help([member])

WHERE
      member: If specified, provides detailed information on the given member.


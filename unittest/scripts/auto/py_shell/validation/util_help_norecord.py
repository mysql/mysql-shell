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

      help([member])
            Provides help about this object and it's members

      import_json(file, options)
            Import JSON documents from file to collection or table in MySQL
            Server using X Protocol session.


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

      - outputFormat - value can be either TEXT (default) or JSON.
      - targetVersion - version to which upgrade will be checked
        (default=8.0.13)
      - password - password for connection.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      SSL Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: the SSL mode to be used in the connection.
      - ssl-ca: the path to the X509 certificate authority in PEM format.
      - ssl-capath: the path to the directory that contains the X509
        certificates authorities in PEM format.
      - ssl-cert: The path to the X509 certificate in PEM format.
      - ssl-key: The path to the X509 key in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: SSL Cipher to use.
      - tls-version: List of protocols permitted for secure connections
      - auth-method: Authentication method
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

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on a TCP connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.


#@<OUT> util import_json help
NAME
      import_json - Import JSON documents from file to collection or table in
                    MySQL Server using X Protocol session.

SYNTAX
      util.import_json(file, options)

WHERE
      file: Path to JSON documents file
      options: Dictionary with options

DESCRIPTION
      Options dictionary:

      - schema: string - name of target schema.
      - collection: string - name of collection where the data will be
        imported.
      - table: string - name of table where the data will be imported.
      - tableColumn: string (default: "doc") - name of column in target table
        where the imported JSON documents will be stored.
      - convertBsonOid: bool (default: false) - enable BSON ObjectId type
        conversion in strict representation of MongoDB Extended JSON

      If the schema is not provided, an active schema on the global session, if
      set, will be used.

      The collection and the table options cannot be combined. If they are not
      provided, the basename of the file without extension will be used as
      target collection name.

      If the target collection or table does not exist, they are created,
      otherwise the data is inserted into the existing collection or table.

      The tableColumn imply use of the table and cannot be combined with the
      collection.

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


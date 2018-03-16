//@ mysqlx module: exports
|Exported Items: 5|
|getSession: object|
|expr: object|
|dateValue: object|
|help: object|
|Type: <mysqlx.Type>|

//@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|

//@<OUT> Help on getSession
Creates a Session instance using the provided connection data.

SYNTAX

  <mysqlx>.getSession(connectionData[, password])

WHERE

  connectionData: The connection data for the session
  password: Password for the session

RETURNS

  A Session

DESCRIPTION

A Session object uses the X Protocol to allow executing operations on the
connected MySQL Server.

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

The following options are valid for use either in a URI or in a dictionary:

 - ssl-mode: the SSL mode to be used in the connection.
 - ssl-ca: the path to the X509 certificate authority in PEM format.
 - ssl-capath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - ssl-cert: The path to the X509 certificate in PEM format.
 - ssl-key: The path to the X509 key in PEM format.
 - ssl-crl: The path to file that contains certificate revocation lists.
 - ssl-crlpath: The path of directory that contains certificate revocation list
   files.
 - ssl-cipher: SSL Cipher to use.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method
 - get-server-public-key: Request public key from the server required for RSA
   key pair-based password exchange. Use when connecting to MySQL 8.0 servers
   with classic MySQL sessions with SSL mode DISABLED.
 - server-public-key-path: The path name to a file containing a client-side
   copy of the public key required by the server for RSA key pair-based
   password exchange. Use when connecting to MySQL 8.0 servers with classic
   MySQL sessions with SSL mode DISABLED.

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

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

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

For additional information on connection data use \? connection.

//@ mysqlx module: dateValue() diffrent parameters
|2025-10-15 00:00:00|
|2017-12-10 10:10:10|
|2017-12-10 10:10:10.500000|
|2017-12-10 10:10:10.599000|

//@ mysqlx module: Bug #26429377
||Invalid number of arguments in mysqlx.dateValue, expected 3 to 7 but got 0 (ArgumentError)
  
//@ mysqlx module: Bug #26429377 - 4/5 arguments
||mysqlx.dateValue: 3,6 or 7 arguments expected (ArgumentError)

//@ mysqlx module: Bug #26429426
||mysqlx.dateValue: Valid day range is 1-31 (ArgumentError)

//@ month validation
||mysqlx.dateValue: Valid month range is 1-12 (ArgumentError)

//@ year validation
||mysqlx.dateValue: Valid year range is 0-9999 (ArgumentError)

//@ hour validation
||mysqlx.dateValue: Valid hour range is 0-23 (ArgumentError)

//@ minute validation
||mysqlx.dateValue: Valid minute range is 0-59 (ArgumentError)

//@ second validation
||mysqlx.dateValue: Valid second range is 0-59 (ArgumentError)

//@ usecond validation
||mysqlx.dateValue: Valid second range is 0-999999 (ArgumentError)

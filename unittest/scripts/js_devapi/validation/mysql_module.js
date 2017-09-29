//@ mysql module: exports
|Exported Items: 2|
|getClassicSession: object|
|help: object|

//@<OUT> Help on getClassicSession
Creates a ClassicSession instance using the provided connection data.

SYNTAX

  <mysql>.getClassicSession(connectionData[, password])

WHERE

  connectionData: The connection data for the session
  password: Password for the session

RETURNS

 A ClassicSession

DESCRIPTION

A ClassicSession object uses the traditional MySQL Protocol to allow executing
operations on the connected MySQL Server.

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]host[:port][/schema][?option=value&option=value...]

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
 - ssl-ciphers: List of permitted ciphers to use for connection encryption.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method

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

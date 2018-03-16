//@<OUT> Object Help
Gives access to general purpose functions and properties.

The following properties are currently supported.

 - options Dictionary of active shell options.


The following functions are currently supported.

 - connect          Establishes the shell global session.
 - getSession
 - help             Provides help about this class and it's members
 - log              Logs an entry to the shell's log file.
 - parseUri         Utility function to parse a URI string.
 - prompt           Utility function to prompt data from the user.
 - reconnect
 - setCurrentSchema
 - setSession

//@<OUT> Options
Dictionary of active shell options.

DESCRIPTION

The options dictionary may contain the following attributes:

 - batchContinueOnError: read-only, boolean value to indicate if the execution
   of an SQL script in batch mode shall continue if errors occur
 - interactive: read-only, boolean value that indicates if the shell is running
   in interactive mode
 - outputFormat: controls the type of output produced for SQL results.
 - sandboxDir: default path where the new sandbox instances for InnoDB cluster
   will be deployed
 - showWarnings: boolean value to indicate whether warnings shall be included
   when printing an SQL result
 - useWizards: read-only, boolean value to indicate if the Shell is using the
   interactive wrappers (wizard mode)
 - history.maxSize: number of entries to keep in command history
 - history.autoSave: true to save command history when exiting the shell
 - history.sql.ignorePattern: colon separated list of glob patterns to filter
   out of the command history in SQL mode

The outputFormat option supports the following values:

 - table: displays the output in table format (default)
 - json: displays the output in JSON format
 - json/raw: displays the output in a JSON format but in a single line
 - vertical: displays the outputs vertically, one line per column value


//@<OUT> Connect
Establishes the shell global session.

SYNTAX

  <Shell>.connect(connectionData[, password])

WHERE

  connectionData: the connection data to be used to establish the session.
  password: The password to be used when establishing the session.

DESCRIPTION

This function will establish the global session with the received connection
data.

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

If no scheme is provided, a first attempt will be made to establish a
NodeSession, and if it detects the used port is for the mysql protocol, it will
attempt a ClassicSession

The password may be included on the connectionData, the optional parameter
should be used only if the connectionData does not contain it already. If both
are specified the password parameter will override the password defined on the
connectionData.


//@<OUT> Help


SYNTAX

  <Shell>.help()


//@<OUT> Parse Uri
Utility function to parse a URI string.

SYNTAX

  <Shell>.parseUri(uri)

WHERE

  uri: a URI string.

RETURNS

  A dictionary containing all the elements contained on the given URI string.

DESCRIPTION

Parses a URI string and returns a dictionary containing an item for each found
element.

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

For more details about how a URI is created as well as the returned dictionary,
use \? connection

//@<OUT> Prompt
Utility function to prompt data from the user.

SYNTAX

  <Shell>.prompt(message[, options])

WHERE

  message: a string with the message to be shown to the user.
  options: Dictionary with options that change the function behavior.

RETURNS

  A string value containing the input from the user.

DESCRIPTION

This function allows creating scripts that require interaction with the user to
gather data.

Calling this function with no options will show the given message to the user
and wait for the input. The information entered by the user will be the
returned value

The options dictionary may contain the following options:

 - defaultValue: a string value to be returned if the provides no data.
 - type: a string value to define the prompt type.

The type option supports the following values:

 - password: the user input will not be echoed on the screen.

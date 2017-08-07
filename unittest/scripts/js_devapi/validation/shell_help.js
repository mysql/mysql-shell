//@<OUT> Object Help
Gives access to general purpose functions and properties.

The following properties are currently supported.

 - options Dictionary of active shell options.


The following functions are currently supported.

 - connect          Establishes the shell global session.
 - getSession
 - help             Provides help about this class and it's members
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



//@<OUT> Custom Prompt
Callback to modify the default shell prompt.

DESCRIPTION

This property can be used to customize the shell prompt, i.e. to make it
include more information than the default implementation.

This property acts as a place holder for a function that would create the
desired prompt.

To modify the default shell prompt, follow the next steps:

 - Define a function that receives no parameters and returns a string which is
   the desired prompt.
 - Assign the function to this property.

The new prompt function will be automatically used to create the shell prompt.

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

The connectionData parameter may be any of:

 - A URI string
 - A dictionary with the connection data

A basic URI string has the following format:

[scheme://][user[:password]@]host[:port][?key=value&key=value...]

The scheme can be any of the following:

 - mysql: For TCP connections using the Classic protocol.
 - mysqlx: For TCP connections using the X protocol.

When using a URI the supported keys include:

 - sslCa: the path to the X509 certificate authority in PEM format.
 - sslCaPath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - sslCert: The path to the X509 certificate in PEM format.
 - sslKey: The path to the X509 key in PEM format.
 - sslCrl: The path to file that contains certificate revocation lists.
 - sslCrlPath: The path of directory that contains certificate revocation list
   files.
 - sslCiphers: List of permitted ciphers to use for connection encryption.
 - sslTlsVersion: List of protocols permitted for secure connections

The connection data dictionary may contain the following attributes:

 - user/dbUser: Username
 - password/dbPassword: Username password
 - host: Hostname or IP address
 - port: Port number
 - The ssl options described above

The password may be included on the connectionData, the optional parameter
should be used only if the connectionData does not contain it already. If both
are specified the password parameter will override the password defined on the
connectionData.

The type of session will be determined by the given scheme:

 - If mysqlx scheme, a NodeSession will be created
 - If mysql scheme, a ClassicSession will be created
 - If 'scheme' is not provided, the shell will first attempt establish a
   NodeSession and if it detects the used port is for the mysql protocol, it
   will attempt a ClassicSession


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

[scheme://][user[:password]@]host[:port][?key=value&key=value...]

The scheme can be any of the following:

 - mysql: For TCP connections using the Classic protocol.
 - mysqlx: For TCP connections using the X protocol.

When using a URI the supported keys include:

 - sslCa: the path to the X509 certificate authority in PEM format.
 - sslCaPath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - sslCert: The path to the X509 certificate in PEM format.
 - sslKey: The path to the X509 key in PEM format.
 - sslCrl: The path to file that contains certificate revocation lists.
 - sslCrlPath: The path of directory that contains certificate revocation list
   files.
 - sslCiphers: List of permitted ciphers to use for connection encryption.
 - sslTlsVersion: List of protocols permitted for secure connections



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

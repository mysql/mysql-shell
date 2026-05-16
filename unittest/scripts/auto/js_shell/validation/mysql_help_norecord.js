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

CONSTANTS
 - ErrorCode MySQL server error codes.

FUNCTIONS
      getClassicSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      getSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      help([member])
            Provides help about this module and it's members

      makeAccount(user, host)
            Joins user and host into an quoted account string.

      parseDataType(typeDefinition)
            Parse a MySQL data type definition.

      parseDdlScript(script[, options])
            Parse DDL statements and return the detected object changes.

      parseStatementAst(statements[, options])
            Parse MySQL statements and return its AST representation.

      quoteIdentifier(s)
            Quote a string as a MySQL identifier, escaping characters when
            needed.

      quoteString(s)
            Quote a string as a MySQL string literal, escaping characters when
            needed.

      splitAccount(account)
            Splits account string into user and host.

      splitScript(script[, options])
            Split a SQL script into individual statements.

      tokenizeStatement(statement[, options])
            Lexes a MySQL statement into a list of tokens.

      unquoteIdentifier(s)
            Unquote a MySQL identifier.

      unquoteString(s)
            Unquote a MySQL string literal.

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.
 - ShellResult    Encapsulates custom query result and metadata.

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

      Connection Data

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

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

      Connection Data

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      For additional information on connection data use \? connection.

//@<OUT> mysql help on help
NAME
      help - Provides help about this module and it's members

SYNTAX
      mysql.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> parseDataType
NAME
      parseDataType - Parse a MySQL data type definition.

SYNTAX
      mysql.parseDataType(typeDefinition)

WHERE
      typeDefinition: MySQL data type definition

RETURNS
      Dictionary with the parsed type, precision, scale, length, explicit
      parameters, and flags. Absent precision, scale, and length values are
      null.

//@<OUT> parseDdlScript
NAME
      parseDdlScript - Parse DDL statements and return the detected object
                       changes.

SYNTAX
      mysql.parseDdlScript(script[, options])

WHERE
      script: SQL script with DDL statements
      options: Dictionary of parser options

RETURNS
      Dictionary with parsed DDL changes

DESCRIPTION
      Supported options are:

      - schema: default schema for unqualified object names
      - ignoreErrors: if true, ignore statements that cannot be parsed
      - version: MySQL server version to parse against; defaults to Shell
        version
      - ansiQuotes: if true, treat `"` as an identifier quote. Default is
        false.
      - noBackslashEscapes: if true, backslashes do not escape characters in
        strings.   Default is false.

//@<OUT> parseStatementAst
NAME
      parseStatementAst - Parse MySQL statements and return its AST
                          representation.

SYNTAX
      mysql.parseStatementAst(statements[, options])

WHERE
      statements: SQL statements to be parsed
      options: Dictionary of parser options

RETURNS
      AST encoded as a JSON structure

DESCRIPTION
      Supported options are:

      - ansiQuotes: if true, treat `"` as an identifier quote
      - noBackslashEscapes: if true, backslashes do not escape characters in
        strings

//@<OUT> quoteIdentifier
NAME
      quoteIdentifier - Quote a string as a MySQL identifier, escaping
                        characters when needed.

SYNTAX
      mysql.quoteIdentifier(s)

WHERE
      s: the identifier name to be quoted

RETURNS
      Quoted identifier

//@<OUT> quoteString
NAME
      quoteString - Quote a string as a MySQL string literal, escaping
                    characters when needed.

SYNTAX
      mysql.quoteString(s)

WHERE
      s: the string value to be quoted

RETURNS
      Quoted string

//@<OUT> splitScript
NAME
      splitScript - Split a SQL script into individual statements.

SYNTAX
      mysql.splitScript(script[, options])

WHERE
      script: A SQL script as a string containing multiple statements
      options: Dictionary of parser options

RETURNS
      A list of statements

DESCRIPTION
      The default statement delimiter is `;` but it can be changed with the
      DELIMITER keyword, which must be followed by the delimiter character(s)
      and a newline.

      Supported options are:

      - ansiQuotes: if true, treat `"` as an identifier quote
      - noBackslashEscapes: if true, backslashes do not escape characters in
        strings

//@<OUT> unquoteIdentifier
NAME
      unquoteIdentifier - Unquote a MySQL identifier.

SYNTAX
      mysql.unquoteIdentifier(s)

WHERE
      s: String to unquote

RETURNS
      Unquoted string.

DESCRIPTION
      An exception is thrown if the input string is not quoted with backticks.

//@<OUT> unquoteString
NAME
      unquoteString - Unquote a MySQL string literal.

SYNTAX
      mysql.unquoteString(s)

WHERE
      s: String to unquote

RETURNS
      Unquoted string

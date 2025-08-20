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

      parseStatementAst(statements)
            Parse MySQL statements and return its AST representation.

      quoteIdentifier(s)
            Quote a string as a MySQL identifier, escaping characters when
            needed.

      splitAccount(account)
            Splits account string into user and host.

      splitScript(script)
            Split a SQL script into individual statements.

      tokenizeStatement(statement)
            Lexes a MySQL statement into a list of tokens.

      unquoteIdentifier(s)
            Unquote a MySQL identifier.

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

//@<OUT> parseStatementAst
NAME
      parseStatementAst - Parse MySQL statements and return its AST
                          representation.

SYNTAX
      mysql.parseStatementAst(statements)

WHERE
      statements: SQL statements to be parsed

RETURNS
      AST encoded as a JSON structure

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

//@<OUT> splitScript
NAME
      splitScript - Split a SQL script into individual statements.

SYNTAX
      mysql.splitScript(script)

WHERE
      script: A SQL script as a string containing multiple statements

RETURNS
      A list of statements

DESCRIPTION
      The default statement delimiter is `;` but it can be changed with the
      DELIMITER keyword, which must be followed by the delimiter character(s)
      and a newline.

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

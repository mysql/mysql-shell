#@<OUT> mysql
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
      get_classic_session(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      get_session(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      help([member])
            Provides help about this module and it's members

      make_account(user, host)
            Joins user and host into an quoted account string.

      parse_statement_ast(statements)
            Parse MySQL statements and return its AST representation.

      quote_identifier(s)
            Quote a string as a MySQL identifier, escaping characters when
            needed.

      split_account(account)
            Splits account string into user and host.

      split_script(script)
            Split a SQL script into individual statements.

      tokenize_statement(statement)
            Lexes a MySQL statement into a list of tokens.

      unquote_identifier(s)
            Unquote a MySQL identifier.

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.
 - ShellResult    Encapsulates custom query result and metadata.

#@<OUT> mysql.get_classic_session
NAME
      get_classic_session - Opens a classic MySQL protocol session to a MySQL
                            server.

SYNTAX
      mysql.get_classic_session(connectionData[, password])

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

#@<OUT> mysql.get_session
NAME
      get_session - Opens a classic MySQL protocol session to a MySQL server.

SYNTAX
      mysql.get_session(connectionData[, password])

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

#@<OUT> mysql.help
NAME
      help - Provides help about this module and it's members

SYNTAX
      mysql.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

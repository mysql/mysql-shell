// Set the shell.options["pager"] to a valid external command and perform an action which should use the Pager. The external command must receive the output of performed action.The command should have a functionality that help us to identify that the Pager is printing the output instead of Shell printing the output.

//@ disable the pager
||

//@<OUT> invoke \help mysql
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

      parseStatementAst(sql)
            Parse MySQL statements and return its AST representation.

      quoteIdentifier(s)
            Quote a string as a MySQL identifier, escaping characters when
            needed.

      splitScript(script)
            Split a SQL script into individual statements.

      unquoteIdentifier(s)
            Unquote a MySQL identifier.

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ invoke \help mysql, there should be no output here
||

//@<OUT> check if pager got all the output from \help mysql
Running with arguments: <<<__pager.cmd>>>

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

      parseStatementAst(sql)
            Parse MySQL statements and return its AST representation.

      quoteIdentifier(s)
            Quote a string as a MySQL identifier, escaping characters when
            needed.

      splitScript(script)
            Split a SQL script into individual statements.

      unquoteIdentifier(s)
            Unquote a MySQL identifier.

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.

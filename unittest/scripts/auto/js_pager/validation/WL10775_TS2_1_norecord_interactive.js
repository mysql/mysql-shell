// Call the help command for different functions/methods/objects, verify that the output is forwarded to the pager specified by shell.options["pager"] option.

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

//@ invoke \help ClassicSession, there should be no output here
||

//@<OUT> check if pager got all the output from \help ClassicSession
Running with arguments: <<<__pager.cmd>>>

NAME
      ClassicSession - Enables interaction with a MySQL Server using the MySQL
                       Protocol.

DESCRIPTION
      Provides facilities to execute queries.

PROPERTIES
      connectionId
            Retrieves the connection id for the current session.

      sshUri
            Retrieves the SSH URI for the current session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the internal connection to the MySQL Server held on this
            session object.

      commit()
            Commits all the operations executed after a call to
            startTransaction().

      getClientData(key)
            Returns value associated with the session for the given key.

      getConnectionId()
            Retrieves the connection id for the current session.

      getSqlMode()
            Retrieves the SQL_MODE for the current session.

      getSshUri()
            Retrieves the SSH URI for the current session.

      getUri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      isOpen()
            Returns true if session is known to be open.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      runSql(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

      setClientData(key, value)
            Associates a value with the session for the given key.

      setOptionTrackerFeatureId(feature_id)
            Defines the id of an option to be reported as used by the option
            tracker.

      setQueryAttributes()
            Defines query attributes that apply to the next statement sent to
            the server for execution.

      startTransaction()
            Starts a transaction context on the server.

//@ invoke \help ClassicSession.rollback, there should be no output here
||

//@<OUT> check if pager got all the output from \help ClassicSession.rollback
Running with arguments: <<<__pager.cmd>>>

NAME
      rollback - Discards all the operations executed after a call to
                 startTransaction().

SYNTAX
      <ClassicSession>.rollback()

DESCRIPTION
      All the operations executed after calling startTransaction() will be
      discarded when this function is called.

      The server autocommit mode will return back to it's state before calling
      startTransaction().



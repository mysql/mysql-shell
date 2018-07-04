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

FUNCTIONS
      getClassicSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      getSession(connectionData[, password])
            Opens a classic MySQL protocol session to a MySQL server.

      help([member])
            Provides help about this module and it's members

CLASSES
 - ClassicResult  Allows browsing through the result information after
                  performing an operation on the database through the MySQL
                  Protocol.
 - ClassicSession Enables interaction with a MySQL Server using the MySQL
                  Protocol.

//@ invoke \help ClassicSession, there should be no output here
||

//@<OUT> check if pager got all the output from \help ClassicSession
Running with arguments: <<<__pager.cmd>>>

NAME
      ClassicSession - Enables interaction with a MySQL Server using the MySQL
                       Protocol.

DESCRIPTION
      Provides facilities to execute queries and retrieve database objects.

PROPERTIES
      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the internal connection to the MySQL Server held on this
            session object.

      commit()
            Commits all the operations executed after a call to
            startTransaction().

      getUri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      isOpen()
            Returns true if session is known to be open.

      query(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      runSql(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

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

RETURNS
       A ClassicResult object.

DESCRIPTION
      All the operations executed after calling startTransaction() will be
      discarded when this function is called.

      The server autocommit mode will return back to it's state before calling
      startTransaction().

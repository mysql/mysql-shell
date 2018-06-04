//@<OUT> ClassicSession help
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

      help()
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

//@<OUT> Help on uri
NAME
      uri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.uri

//@<OUT> Help on close
NAME
      close - Closes the internal connection to the MySQL Server held on this
              session object.

SYNTAX
      <ClassicSession>.close()

//@<OUT> Help on commit
NAME
      commit - Commits all the operations executed after a call to
               startTransaction().

SYNTAX
      <ClassicSession>.commit()

RETURNS
       A ClassicResult object.

DESCRIPTION
      All the operations executed after calling startTransaction() will take
      place when this function is called.

      The server autocommit mode will return back to it's state before calling
      startTransaction().

//@<OUT> Help on getUri
NAME
      getUri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.getUri()

RETURNS
      A string representing the connection data.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <ClassicSession>.help()

//@<OUT> Help on isOpen
NAME
      isOpen - Returns true if session is known to be open.

SYNTAX
      <ClassicSession>.isOpen()

RETURNS
       A boolean value indicating if the session is still open.

DESCRIPTION
      Returns true if the session is still open and false otherwise. Note: may
      return true if connection is lost.

//@<OUT> Help on query
NAME
      query - Executes a query and returns the corresponding ClassicResult
              object.

SYNTAX
      <ClassicSession>.query(query[, args])

WHERE
      query: the SQL query string to execute, with optional ? placeholders
      args: List of literals to use when replacing ? placeholders in the query
            string.

RETURNS
       A ClassicResult object.

//@<OUT> Help on rollback
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

//@<OUT> Help on runSql
NAME
      runSql - Executes a query and returns the corresponding ClassicResult
               object.

SYNTAX
      <ClassicSession>.runSql(query[, args])

WHERE
      query: the SQL query to execute against the database.
      args: List of literals to use when replacing ? placeholders in the query
            string.

RETURNS
       A ClassicResult object.

//@<OUT> Help on startTransaction
NAME
      startTransaction - Starts a transaction context on the server.

SYNTAX
      <ClassicSession>.startTransaction()

RETURNS
       A ClassicResult object.

DESCRIPTION
      Calling this function will turn off the autocommit mode on the server.

      All the operations executed after calling this function will take place
      only when commit() is called.

      All the operations executed after calling this function, will be
      discarded is rollback() is called.

      When commit() or rollback() are called, the server autocommit mode will
      return back to it's state before calling startTransaction().


#@ __global__
||

#@<OUT> session
NAME
      ClassicSession - Enables interaction with a MySQL Server using the MySQL
                       Protocol.

DESCRIPTION
      Provides facilities to execute queries.

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

      get_uri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      is_open()
            Returns true if session is known to be open.

      query(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

            ATTENTION: This function will be removed in a future release, use
                       the run_sql function instead.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      run_sql(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

      start_transaction()
            Starts a transaction context on the server.

#@<OUT> session.close
NAME
      close - Closes the internal connection to the MySQL Server held on this
              session object.

SYNTAX
      <ClassicSession>.close()

#@<OUT> session.commit
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

#@<OUT> session.get_uri
NAME
      get_uri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.get_uri()

RETURNS
      A string representing the connection data.

#@<OUT> session.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <ClassicSession>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> session.is_open
NAME
      is_open - Returns true if session is known to be open.

SYNTAX
      <ClassicSession>.is_open()

RETURNS
      A boolean value indicating if the session is still open.

DESCRIPTION
      Returns true if the session is still open and false otherwise. Note: may
      return true if connection is lost.

#@<OUT> session.query
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

DESCRIPTION
      ATTENTION: This function will be removed in a future release, use the
                 run_sql function instead.

#@<OUT> session.rollback
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

#@<OUT> session.run_sql
NAME
      run_sql - Executes a query and returns the corresponding ClassicResult
                object.

SYNTAX
      <ClassicSession>.run_sql(query[, args])

WHERE
      query: the SQL query to execute against the database.
      args: List of literals to use when replacing ? placeholders in the query
            string.

RETURNS
      A ClassicResult object.

EXCEPTIONS
      LogicError if there's no open session.

      ArgumentError if the parameters are invalid.

#@<OUT> session.start_transaction
NAME
      start_transaction - Starts a transaction context on the server.

SYNTAX
      <ClassicSession>.start_transaction()

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

#@<OUT> session.uri
NAME
      uri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.uri


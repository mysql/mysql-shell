//@<OUT> ClassicSession help
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

      setQueryAttributes()
            Defines query attributes that apply to the next statement sent to
            the server for execution.

      startTransaction()
            Starts a transaction context on the server.

//@<OUT> Help on uri
NAME
      uri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.uri

//@<OUT> Help on getSqlMode
NAME
      getSqlMode - Retrieves the SQL_MODE for the current session.

SYNTAX
      <ClassicSession>.getSqlMode()

RETURNS
      Value of the SQL_MODE session variable.

DESCRIPTION
      Queries the value of the SQL_MODE session variable. If session tracking
      of SQL_MODE is enabled, it will fetch its cached value.

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
      <ClassicSession>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on isOpen
NAME
      isOpen - Returns true if session is known to be open.

SYNTAX
      <ClassicSession>.isOpen()

RETURNS
      A boolean value indicating if the session is still open.

DESCRIPTION
      Returns true if the session is still open and false otherwise.

      NOTE: This function may return true if connection is lost.

//@<OUT> Help on rollback
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

EXCEPTIONS
      LogicError if there's no open session.

      ArgumentError if the parameters are invalid.

//@<OUT> Help on startTransaction
NAME
      startTransaction - Starts a transaction context on the server.

SYNTAX
      <ClassicSession>.startTransaction()

DESCRIPTION
      Calling this function will turn off the autocommit mode on the server.

      All the operations executed after calling this function will take place
      only when commit() is called.

      All the operations executed after calling this function, will be
      discarded if rollback() is called.

      When commit() or rollback() are called, the server autocommit mode will
      return back to it's state before calling startTransaction().

//@<OUT> Help on setQueryAttributes
NAME
      setQueryAttributes - Defines query attributes that apply to the next
                           statement sent to the server for execution.

SYNTAX
      <ClassicSession>.setQueryAttributes()

DESCRIPTION
      It is possible to define up to 32 pairs of attribute and values for the
      next executed SQL statement.

      To access query attributes within SQL statements for which attributes
      have been defined, the query_attributes component must be installed, this
      component implements a mysql_query_attribute_string() loadable function
      that takes an attribute name argument and returns the attribute value as
      a string, or NULL if the attribute does not exist.

      To install the query_attributes component execute the following
      statement:
         session.runSql('INSTALL COMPONENT
      "file://component_query_attributes"');


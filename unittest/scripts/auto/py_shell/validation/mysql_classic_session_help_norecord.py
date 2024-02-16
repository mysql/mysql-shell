#@ __global__
||

#@<OUT> session
NAME
      ClassicSession - Enables interaction with a MySQL Server using the MySQL
                       Protocol.

DESCRIPTION
      Provides facilities to execute queries.

PROPERTIES
      ssh_uri
            Retrieves the SSH URI for the current session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the internal connection to the MySQL Server held on this
            session object.

      commit()
            Commits all the operations executed after a call to
            start_transaction().

      get_ssh_uri()
            Retrieves the SSH URI for the current session.

      get_uri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      is_open()
            Returns true if session is known to be open.

      rollback()
            Discards all the operations executed after a call to
            start_transaction().

      run_sql(query[, args])
            Executes a query and returns the corresponding ClassicResult
            object.

      set_query_attributes()
            Defines query attributes that apply to the next statement sent to
            the server for execution.

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
               start_transaction().

SYNTAX
      <ClassicSession>.commit()

DESCRIPTION
      All the operations executed after calling start_transaction() will take
      place when this function is called.

      The server autocommit mode will return back to it's state before calling
      start_transaction().

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
      Returns true if the session is still open and false otherwise.

      NOTE: This function may return true if connection is lost.

#@<OUT> session.rollback
NAME
      rollback - Discards all the operations executed after a call to
                 start_transaction().

SYNTAX
      <ClassicSession>.rollback()

DESCRIPTION
      All the operations executed after calling start_transaction() will be
      discarded when this function is called.

      The server autocommit mode will return back to it's state before calling
      start_transaction().

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

DESCRIPTION
      Calling this function will turn off the autocommit mode on the server.

      All the operations executed after calling this function will take place
      only when commit() is called.

      All the operations executed after calling this function, will be
      discarded if rollback() is called.

      When commit() or rollback() are called, the server autocommit mode will
      return back to it's state before calling start_transaction().

#@<OUT> session.uri
NAME
      uri - Retrieves the URI for the current session.

SYNTAX
      <ClassicSession>.uri

#@<OUT> Help on set_query_attributes
NAME
      set_query_attributes - Defines query attributes that apply to the next
                             statement sent to the server for execution.

SYNTAX
      <ClassicSession>.set_query_attributes()

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
         session.run_sql('INSTALL COMPONENT
      "file://component_query_attributes"');


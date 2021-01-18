#@ __global__
||

#@<OUT> session
NAME
      Session - Enables interaction with a MySQL Server using the X Protocol.

DESCRIPTION
      Document Store functionality can be used through this object, in addition
      to SQL.

      This class allows performing database operations such as:

      - Schema management operations.
      - Access to relational tables.
      - Access to Document Store collections.
      - Enabling/disabling warning generation.
      - Retrieval of connection information.

PROPERTIES
      current_schema
            Retrieves the active schema on the session.

      default_schema
            Retrieves the Schema configured as default for the session.

      ssh_uri
            Retrieves the SSH URI for the current session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the session.

      commit()
            Commits all the operations executed after a call to
            start_transaction().

      create_schema(name)
            Creates a schema on the database and returns the corresponding
            object.

      drop_schema(name)
            Drops the schema with the specified name.

      get_current_schema()
            Retrieves the active schema on the session.

      get_default_schema()
            Retrieves the Schema configured as default for the session.

      get_schema(name)
            Retrieves a Schema object from the current session through it's
            name.

      get_schemas()
            Retrieves the Schemas available on the session.

      get_ssh_uri()
            Retrieves the SSH URI for the current session.

      get_uri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      is_open()
            Returns true if session is known to be open.

      quote_name(id)
            Escapes the passed identifier.

      release_savepoint(name)
            Removes a savepoint defined on a transaction.

      rollback()
            Discards all the operations executed after a call to
            start_transaction().

      rollback_to(name)
            Rolls back the transaction to the named savepoint without
            terminating the transaction.

      run_sql(query[, args])
            Executes a query and returns the corresponding SqlResult object.

      set_current_schema(name)
            Sets the current schema for this session, and returns the schema
            object for it.

      set_fetch_warnings(enable)
            Enables or disables warning generation.

      set_savepoint([name])
            Creates or replaces a transaction savepoint with the given name.

      sql(statement)
            Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

      start_transaction()
            Starts a transaction context on the server.

#@<OUT> session.close
NAME
      close - Closes the session.

SYNTAX
      <Session>.close()

DESCRIPTION
      After closing the session it is still possible to make read only
      operations to gather metadata info, like get_table(name) or
      get_schemas().

#@<OUT> session.commit
NAME
      commit - Commits all the operations executed after a call to
               start_transaction().

SYNTAX
      <Session>.commit()

RETURNS
      A SqlResult object.

DESCRIPTION
      All the operations executed after calling start_transaction() will take
      place when this function is called.

      The server autocommit mode will return back to it's state before calling
      start_transaction().

#@<OUT> session.create_schema
NAME
      create_schema - Creates a schema on the database and returns the
                      corresponding object.

SYNTAX
      <Session>.create_schema(name)

WHERE
      name: A string value indicating the schema name.

RETURNS
      The created schema object.

EXCEPTIONS
      A MySQL error is thrown if fails creating the schema.

#@<OUT> session.current_schema
NAME
      current_schema - Retrieves the active schema on the session.

SYNTAX
      <Session>.current_schema

#@<OUT> session.default_schema
NAME
      default_schema - Retrieves the Schema configured as default for the
                       session.

SYNTAX
      <Session>.default_schema

#@<OUT> session.drop_schema
NAME
      drop_schema - Drops the schema with the specified name.

SYNTAX
      <Session>.drop_schema(name)

WHERE
      name: The name of the schema to be dropped.

RETURNS
      Nothing.

#@<OUT> session.get_current_schema
NAME
      get_current_schema - Retrieves the active schema on the session.

SYNTAX
      <Session>.get_current_schema()

RETURNS
      A Schema object if a schema is active on the session.

#@<OUT> session.get_default_schema
NAME
      get_default_schema - Retrieves the Schema configured as default for the
                           session.

SYNTAX
      <Session>.get_default_schema()

RETURNS
      A Schema object or Null

#@<OUT> session.get_schema
NAME
      get_schema - Retrieves a Schema object from the current session through
                   it's name.

SYNTAX
      <Session>.get_schema(name)

WHERE
      name: The name of the Schema object to be retrieved.

RETURNS
      The Schema object with the given name.

EXCEPTIONS
      RuntimeError If the given name is not a valid schema.

#@<OUT> session.get_schemas
NAME
      get_schemas - Retrieves the Schemas available on the session.

SYNTAX
      <Session>.get_schemas()

RETURNS
      A List containing the Schema objects available on the session.

#@<OUT> session.get_uri
NAME
      get_uri - Retrieves the URI for the current session.

SYNTAX
      <Session>.get_uri()

RETURNS
      A string representing the connection data.

#@<OUT> session.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Session>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> session.is_open
NAME
      is_open - Returns true if session is known to be open.

SYNTAX
      <Session>.is_open()

RETURNS
      A boolean value indicating if the session is still open.

DESCRIPTION
      Returns true if the session is still open and false otherwise.

      NOTE: This function may return true if connection is lost.

#@<OUT> session.quote_name
NAME
      quote_name - Escapes the passed identifier.

SYNTAX
      <Session>.quote_name(id)

WHERE
      id: The identifier to be quoted.

RETURNS
      A String containing the escaped identifier.

#@<OUT> session.release_savepoint
NAME
      release_savepoint - Removes a savepoint defined on a transaction.

SYNTAX
      <Session>.release_savepoint(name)

WHERE
      name: string with the name of the savepoint to be removed.

DESCRIPTION
      Removes a named savepoint from the set of savepoints defined on the
      current transaction. This does not affect the operations executed on the
      transaction since no commit or rollback occurs.

      It is an error trying to remove a savepoint that does not exist.

#@<OUT> session.rollback
NAME
      rollback - Discards all the operations executed after a call to
                 start_transaction().

SYNTAX
      <Session>.rollback()

RETURNS
      A SqlResult object.

DESCRIPTION
      All the operations executed after calling start_transaction() will be
      discarded when this function is called.

      The server autocommit mode will return back to it's state before calling
      start_transaction().

#@<OUT> session.rollback_to
NAME
      rollback_to - Rolls back the transaction to the named savepoint without
                    terminating the transaction.

SYNTAX
      <Session>.rollback_to(name)

WHERE
      name: string with the name of the savepoint for the rollback operation.

DESCRIPTION
      Modifications that the current transaction made to rows after the
      savepoint was defined will be rolled back.

      The given savepoint will not be removed, but any savepoint defined after
      the given savepoint was defined will be removed.

      It is an error calling this operation with an unexisting savepoint.

#@<OUT> session.run_sql
NAME
      run_sql - Executes a query and returns the corresponding SqlResult
                object.

SYNTAX
      <Session>.run_sql(query[, args])

WHERE
      query: the SQL query to execute against the database.
      args: List of literals to use when replacing ? placeholders in the query
            string.

RETURNS
      An SqlResult object.

EXCEPTIONS
      LogicError if there's no open session.

      ArgumentError if the parameters are invalid.

#@<OUT> session.set_current_schema
NAME
      set_current_schema - Sets the current schema for this session, and
                           returns the schema object for it.

SYNTAX
      <Session>.set_current_schema(name)

WHERE
      name: the name of the new schema to switch to.

RETURNS
      the Schema object for the new schema.

DESCRIPTION
      At the database level, this is equivalent at issuing the following SQL
      query:

        use <new-default-schema>;

#@<OUT> session.set_fetch_warnings
NAME
      set_fetch_warnings - Enables or disables warning generation.

SYNTAX
      <Session>.set_fetch_warnings(enable)

WHERE
      enable: Boolean value to enable or disable the warnings.

DESCRIPTION
      Warnings are generated sometimes when database operations are executed,
      such as SQL statements.

      On a Node session the warning generation is disabled by default. This
      function can be used to enable or disable the warning generation based on
      the received parameter.

      When warning generation is enabled, the warnings will be available
      through the result object returned on the executed operation.

#@<OUT> session.set_savepoint
NAME
      set_savepoint - Creates or replaces a transaction savepoint with the
                      given name.

SYNTAX
      <Session>.set_savepoint([name])

WHERE
      name: String with the name to be assigned to the transaction save point.

RETURNS
      The name of the transaction savepoint.

DESCRIPTION
      When working with transactions, using savepoints allows rolling back
      operations executed after the savepoint without terminating the
      transaction.

      Use this function to set a savepoint within a transaction.

      If this function is called with the same name of another savepoint set
      previously, the original savepoint will be deleted and a new one will be
      created.

      If the name is not provided an auto-generated name as 'TXSP#' will be
      assigned, where # is a consecutive number that guarantees uniqueness of
      the savepoint at Session level.

#@<OUT> session.sql
NAME
      sql - Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

SYNTAX
      Session.sql(statement)
             [.bind(data)]
             [.execute()]

DESCRIPTION
      This method creates an SqlExecute object which is a SQL execution
      handler.

      The SqlExecute class has functions that allow defining the way the
      statement will be executed and allows doing parameter binding.

      The received SQL is set on the execution handler.

      sql(statement)
            This function is called automatically when Session.sql(sql) is
            called.

            Parameter binding is supported and can be done by using the \b ?
            placeholder instead of passing values directly on the SQL
            statement.

            Parameters are bound in positional order.

            The actual execution of the SQL statement will occur when the
            execute() function is called.

            After this function invocation, the following functions can be
            invoked:

            - bind(Value data)
            - execute().

      bind(data)
            This method can be invoked any number of times, each time the
            received parameters will be added to an internal binding list.

            This function can be invoked after:

            - sql(String statement)
            - bind(Value data)

            After this function invocation, the following functions can be
            invoked:

            - bind(Value data)
            - execute().

      execute()
            This function can be invoked after:

            - sql(String statement)
            - bind(Value data)

#@<OUT> session.start_transaction
NAME
      start_transaction - Starts a transaction context on the server.

SYNTAX
      <Session>.start_transaction()

RETURNS
      A SqlResult object.

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
      <Session>.uri


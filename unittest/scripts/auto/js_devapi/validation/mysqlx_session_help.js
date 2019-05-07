//@ Initialization
||

//@<OUT> Session.help
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
      currentSchema
            Retrieves the active schema on the session.

      defaultSchema
            Retrieves the Schema configured as default for the session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the session.

      commit()
            Commits all the operations executed after a call to
            startTransaction().

      createSchema(name)
            Creates a schema on the database and returns the corresponding
            object.

      dropSchema(name)
            Drops the schema with the specified name.

      getCurrentSchema()
            Retrieves the active schema on the session.

      getDefaultSchema()
            Retrieves the Schema configured as default for the session.

      getSchema(name)
            Retrieves a Schema object from the current session through it's
            name.

      getSchemas()
            Retrieves the Schemas available on the session.

      getUri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      isOpen()
            Returns true if session is known to be open.

      quoteName(id)
            Escapes the passed identifier.

      releaseSavepoint(name)
            Removes a savepoint defined on a transaction.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      rollbackTo(name)
            Rolls back the transaction to the named savepoint without
            terminating the transaction.

      runSql(query[, args])
            Executes a query and returns the corresponding SqlResult object.

      setCurrentSchema(name)
            Sets the current schema for this session, and returns the schema
            object for it.

      setFetchWarnings(enable)
            Enables or disables warning generation.

      setSavepoint([name])
            Creates or replaces a transaction savepoint with the given name.

      sql(statement)
            Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

      startTransaction()
            Starts a transaction context on the server.

//@<OUT> Help on currentSchema
NAME
      currentSchema - Retrieves the active schema on the session.

SYNTAX
      <Session>.currentSchema

//@<OUT> Help on defaultSchema
NAME
      defaultSchema - Retrieves the Schema configured as default for the
                      session.

SYNTAX
      <Session>.defaultSchema

//@<OUT> Help on uri
NAME
      uri - Retrieves the URI for the current session.

SYNTAX
      <Session>.uri

//@<OUT> Help on close
NAME
      close - Closes the session.

SYNTAX
      <Session>.close()

DESCRIPTION
      After closing the session it is still possible to make read only
      operations to gather metadata info, like getTable(name) or getSchemas().

//@<OUT> Help on commit
NAME
      commit - Commits all the operations executed after a call to
               startTransaction().

SYNTAX
      <Session>.commit()

RETURNS
       A SqlResult object.

DESCRIPTION
      All the operations executed after calling startTransaction() will take
      place when this function is called.

      The server autocommit mode will return back to it's state before calling
      startTransaction().

//@<OUT> Help on createSchema
NAME
      createSchema - Creates a schema on the database and returns the
                     corresponding object.

SYNTAX
      <Session>.createSchema(name)

WHERE
      name: A string value indicating the schema name.

RETURNS
       The created schema object.

//@<OUT> Help on dropSchema
NAME
      dropSchema - Drops the schema with the specified name.

SYNTAX
      <Session>.dropSchema(name)

WHERE
      name: The name of the schema to be dropped.

RETURNS
       Nothing.

//@<OUT> Help on getCurrentSchema
NAME
      getCurrentSchema - Retrieves the active schema on the session.

SYNTAX
      <Session>.getCurrentSchema()

RETURNS
      A Schema object if a schema is active on the session.

//@<OUT> Help on getDefaultSchema
NAME
      getDefaultSchema - Retrieves the Schema configured as default for the
                         session.

SYNTAX
      <Session>.getDefaultSchema()

RETURNS
       A Schema object or Null

//@<OUT> Help on getSchema
NAME
      getSchema - Retrieves a Schema object from the current session through
                  it's name.

SYNTAX
      <Session>.getSchema(name)

WHERE
      name: The name of the Schema object to be retrieved.

RETURNS
       The Schema object with the given name.

//@<OUT> Help on getSchemas
NAME
      getSchemas - Retrieves the Schemas available on the session.

SYNTAX
      <Session>.getSchemas()

RETURNS
       A List containing the Schema objects available on the session.

//@<OUT> Help on getUri
NAME
      getUri - Retrieves the URI for the current session.

SYNTAX
      <Session>.getUri()

RETURNS
      A string representing the connection data.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Session>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on isOpen
NAME
      isOpen - Returns true if session is known to be open.

SYNTAX
      <Session>.isOpen()

RETURNS
       A boolean value indicating if the session is still open.

DESCRIPTION
      Returns true if the session is still open and false otherwise. Note: may
      return true if connection is lost.

//@<OUT> Help on quoteName
NAME
      quoteName - Escapes the passed identifier.

SYNTAX
      <Session>.quoteName(id)

WHERE
      id: The identifier to be quoted.

RETURNS
      A String containing the escaped identifier.

//@<OUT> Help on releaseSavePoint
NAME
      releaseSavepoint - Removes a savepoint defined on a transaction.

SYNTAX
      <Session>.releaseSavepoint(name)

WHERE
      name: string with the name of the savepoint to be removed.

DESCRIPTION
      Removes a named savepoint from the set of savepoints defined on the
      current transaction. This does not affect the operations executed on the
      transaction since no commit or rollback occurs.

      It is an error trying to remove a savepoint that does not exist.

//@<OUT> Help on rollBack
NAME
      rollback - Discards all the operations executed after a call to
                 startTransaction().

SYNTAX
      <Session>.rollback()

RETURNS
       A SqlResult object.

DESCRIPTION
      All the operations executed after calling startTransaction() will be
      discarded when this function is called.

      The server autocommit mode will return back to it's state before calling
      startTransaction().

//@<OUT> Help on rollBackTo
NAME
      rollbackTo - Rolls back the transaction to the named savepoint without
                   terminating the transaction.

SYNTAX
      <Session>.rollbackTo(name)

WHERE
      name: string with the name of the savepoint for the rollback operation.

DESCRIPTION
      Modifications that the current transaction made to rows after the
      savepoint was defined will be rolled back.

      The given savepoint will not be removed, but any savepoint defined after
      the given savepoint was defined will be removed.

      It is an error calling this operation with an unexisting savepoint.

//@<OUT> Help on runSql
NAME
      runSql - Executes a query and returns the corresponding SqlResult object.

SYNTAX
      <Session>.runSql(query[, args])

WHERE
      query: the SQL query to execute against the database.
      args: List of literals to use when replacing ? placeholders in the query
            string.

RETURNS
       An SqlResult object.

EXCEPTIONS
      LogicError if there's no open session.

      ArgumentError if the parameters are invalid.

//@<OUT> Help on setCurrentSchema
NAME
      setCurrentSchema - Sets the current schema for this session, and returns
                         the schema object for it.

SYNTAX
      <Session>.setCurrentSchema(name)

WHERE
      name: the name of the new schema to switch to.

RETURNS
      the Schema object for the new schema.

DESCRIPTION
      At the database level, this is equivalent at issuing the following SQL
      query:

        use <new-default-schema>;

//@<OUT> Help on setFetchWarnings
NAME
      setFetchWarnings - Enables or disables warning generation.

SYNTAX
      <Session>.setFetchWarnings(enable)

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

//@<OUT> Help on setSavePoint
NAME
      setSavepoint - Creates or replaces a transaction savepoint with the given
                     name.

SYNTAX
      <Session>.setSavepoint([name])

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

//@<OUT> Help on sql
NAME
      sql - Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

SYNTAX
      Session.sql(statement)
             [.bind(value, values)]
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

            - bind(Value value)
            - bind(List values)
            - execute().

      bind(value, values)
            This method can be invoked any number of times, each time the
            received parameter will be added to an internal binding list.

            This function can be invoked after:

            - sql(String statement)
            - bind(Value value)
            - bind(List values)

            After this function invocation, the following functions can be
            invoked:

            - bind(Value value)
            - bind(List values)
            - execute().

      execute()
            This function can be invoked after:

            - sql(String statement)
            - bind(Value value)
            - bind(List values)

//@<OUT> Help on startTransaction
NAME
      startTransaction - Starts a transaction context on the server.

SYNTAX
      <Session>.startTransaction()

RETURNS
       A SqlResult object.

DESCRIPTION
      Calling this function will turn off the autocommit mode on the server.

      All the operations executed after calling this function will take place
      only when commit() is called.

      All the operations executed after calling this function, will be
      discarded is rollback() is called.

      When commit() or rollback() are called, the server autocommit mode will
      return back to it's state before calling startTransaction().

//@ Finalization
||

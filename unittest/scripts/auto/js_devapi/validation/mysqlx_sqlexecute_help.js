//@ Initialization
||

//@<OUT> SqlExecute help
NAME
      SqlExecute - Handler for execution SQL statements, supports parameter
                   binding.

DESCRIPTION
      This object should only be created by calling the sql function at a
      Session instance.

FUNCTIONS
      bind(data)
            Registers a value or a list of values to be bound on the execution
            of the SQL statement.

      execute()
            Executes the sql statement.

      help([member])
            Provides help about this class and it's members

      sql(statement)
            Sets the sql statement to be executed by this handler.

//@<OUT> Help on bind
NAME
      bind - Registers a value or a list of values to be bound on the execution
             of the SQL statement.

SYNTAX
      <SqlExecute>.bind(data)

WHERE
      data: the value or list of values to be bound.

RETURNS
      This SqlExecute object.

DESCRIPTION
      This method can be invoked any number of times, each time the received
      parameters will be added to an internal binding list.

      This function can be invoked after:

      - sql(String statement)
      - bind(Value value)
      - bind(List values)

      After this function invocation, the following functions can be invoked:

      - bind(Value value)
      - bind(List values)
      - execute().

//@<OUT> Help on execute
NAME
      execute - Executes the sql statement.

SYNTAX
      <SqlExecute>.execute()

RETURNS
      A SqlResult object.

DESCRIPTION
      This function can be invoked after:

      - sql(String statement)
      - bind(Value value)
      - bind(List values)

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <SqlExecute>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on sql
NAME
      sql - Sets the sql statement to be executed by this handler.

SYNTAX
      <SqlExecute>.sql(statement)

WHERE
      statement: A string containing the SQL statement to be executed.

RETURNS
      This SqlExecute object.

DESCRIPTION
      This function is called automatically when Session.sql(sql) is called.

      Parameter binding is supported and can be done by using the \b ?
      placeholder instead of passing values directly on the SQL statement.

      Parameters are bound in positional order.

      The actual execution of the SQL statement will occur when the execute()
      function is called.

      After this function invocation, the following functions can be invoked:

      - bind(Value value)
      - bind(List values)
      - execute().

//@ Finalization
||

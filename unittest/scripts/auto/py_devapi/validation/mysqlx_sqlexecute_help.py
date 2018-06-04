#@ __global__
||

#@<OUT> sqlexecute
NAME
      SqlExecute - Handler for execution SQL statements, supports parameter
                   binding.

DESCRIPTION
      This object should only be created by calling the sql function at a
      Session instance.

FUNCTIONS
      bind(value, values)
            Registers a parameter to be bound on the execution of the SQL
            statement.

            Registers a list of parameter to be bound on the execution of the
            SQL statement.

      execute()
            Executes the sql statement.

      help()
            Provides help about this class and it's members

      sql(statement)
            Sets the sql statement to be executed by this handler.

#@<OUT> sqlexecute.bind
NAME
      bind - Registers a parameter to be bound on the execution of the SQL
             statement.

             Registers a list of parameter to be bound on the execution of the
             SQL statement.

SYNTAX
      <SqlExecute>.bind(value, values)

WHERE
      value: the value to be bound.
      values: the value list to be bound.

RETURNS
       This SqlExecute object.

DESCRIPTION
      This method can be invoked any number of times, each time the received
      parameter will be added to an internal binding list.

      This function can be invoked after:

      - sql(String statement)
      - bind(Value value)
      - bind(List values)

      After this function invocation, the following functions can be invoked:

      - bind(Value value)
      - bind(List values)
      - execute().

#@<OUT> sqlexecute.execute
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

#@<OUT> sqlexecute.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <SqlExecute>.help()

#@<OUT> sqlexecute.sql
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

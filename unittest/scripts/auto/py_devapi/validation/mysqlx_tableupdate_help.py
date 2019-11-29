#@ __global__
||

#@<OUT> tableupdate
NAME
      TableUpdate - Operation to add update records in a Table.

DESCRIPTION
      A TableUpdate object is used to update rows in a Table, is created
      through the update function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      execute()
            Executes the update operation with all the configured options.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be updated by the operation.

      order_by(...)
            Sets the order in which the records will be updated.

      set(attribute, value)
            Adds an update operation.

      update()
            Initializes the update operation.

      where(expression)
            Sets the search condition to filter the records to be updated.

#@<OUT> tableupdate.bind
NAME
      bind - Binds a value to a specific placeholder used on this operation.

SYNTAX
      <TableUpdate>.bind(name, value)

WHERE
      name: The name of the placeholder to which the value will be bound.
      value: The value to be bound on the placeholder.

RETURNS
      This TableUpdate object.

DESCRIPTION
      Binds the given value to the placeholder with the specified name.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

#@<OUT> tableupdate.execute
NAME
      execute - Executes the update operation with all the configured options.

SYNTAX
      <TableUpdate>.execute()

RETURNS
      A Result object.

#@<OUT> tableupdate.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableUpdate>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> tableupdate.limit
NAME
      limit - Sets the maximum number of rows to be updated by the operation.

SYNTAX
      <TableUpdate>.limit(numberOfRows)

WHERE
      numberOfRows: The maximum number of rows to be updated.

RETURNS
      This TableUpdate object.

DESCRIPTION
      If used, the operation will update only numberOfRows rows.

      This function can be called every time the statement is executed.

#@<OUT> tableupdate.order_by
NAME
      order_by - Sets the order in which the records will be updated.

SYNTAX
      <TableUpdate>.order_by(sortCriteriaList)
      <TableUpdate>.order_by(sortCriterion[, sortCriterion, ...])

RETURNS
      This TableUpdate object.

DESCRIPTION
      If used, the TableUpdate operation will update the records in the order
      established by the sort criteria.

      Every defined sort criterion follows the format:

      name [ ASC | DESC ]

      ASC is used by default if the sort order is not specified.

#@<OUT> tableupdate.set
NAME
      set - Adds an update operation.

SYNTAX
      <TableUpdate>.set(attribute, value)

WHERE
      attribute: Identifies the column to be updated by this operation.
      value: Defines the value to be set on the indicated column.

RETURNS
      This TableUpdate object.

DESCRIPTION
      Adds an operation into the update handler to update a column value in the
      records that were included on the selection filter and limit.

      Using Expressions As Values

      If a mysqlx.expr(...) object is defined as a value, it will be evaluated
      in the server, the resulting value will be set at the indicated column.

#@<OUT> tableupdate.where
NAME
      where - Sets the search condition to filter the records to be updated.

SYNTAX
      <TableUpdate>.where(expression)

WHERE
      expression: A condition to filter the records to be updated.

RETURNS
      This TableUpdate object.

DESCRIPTION
      If used, only those rows satisfying the expression will be updated

      The expression supports parameter binding.

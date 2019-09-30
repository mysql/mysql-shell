//@ Initialization
||

//@<OUT> TableUpdate help
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

      orderBy(...)
            Sets the order in which the records will be updated.

      set(attribute, value)
            Adds an update operation.

      update()
            Initializes the update operation.

      where([expression])
            Sets the search condition to filter the records to be updated.

//@<OUT> Help on bind
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
      Binds a value to a specific placeholder used on this operation.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

//@<OUT> Help on execute
NAME
      execute - Executes the update operation with all the configured options.

SYNTAX
      <TableUpdate>.execute()

RETURNS
      A Result object.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableUpdate>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on limit
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

//@<OUT> Help on orderBy
NAME
      orderBy - Sets the order in which the records will be updated.

SYNTAX
      <TableUpdate>.orderBy(sortCriteria)
      <TableUpdate>.orderBy(sortCriterion[, sortCriterion, ...])

RETURNS
      This TableUpdate object.

DESCRIPTION
      If used the records will be updated in the order established by the sort
      criteria.

      The elements of sortExprStr list are strings defining the column name on
      which the sorting will be based.

      The format is as follows: columnIdentifier [ ASC | DESC ]

      If no order criteria is specified, ASC will be used by default.

//@<OUT> Help on set
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
      Adds an operation into the update handler to update a column value in on
      the records that were included on the selection filter and limit.

//@<OUT> Help on where
NAME
      where - Sets the search condition to filter the records to be updated.

SYNTAX
      <TableUpdate>.where([expression])

WHERE
      expression: Condition to filter the records to be updated.

RETURNS
      This TableUpdate object.

DESCRIPTION
      If used, only those rows satisfying the expression will be updated

      The expression supports parameter binding.

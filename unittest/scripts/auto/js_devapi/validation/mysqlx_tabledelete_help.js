//@ Initialization
||

//@<OUT> TableDelete help
NAME
      TableDelete - Operation to delete data from a table.

DESCRIPTION
      A TableDelete represents an operation to remove records from a Table, it
      is created through the delete function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      delete()
            Initializes the deletion operation.

      execute()
            Executes the delete operation with all the configured options.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be deleted by the operation.

      orderBy(...)
            Sets the order in which the records will be deleted.

      where([expression])
            Sets the search condition to filter the records to be deleted from
            the Table.

//@<OUT> Help on bind
NAME
      bind - Binds a value to a specific placeholder used on this operation.

SYNTAX
      <TableDelete>.bind(name, value)

WHERE
      name: The name of the placeholder to which the value will be bound.
      value: The value to be bound on the placeholder.

RETURNS
       This TableDelete object.

DESCRIPTION
      Binds a value to a specific placeholder used on this operation.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

//@<OUT> Help on delete
NAME
      delete - Initializes the deletion operation.

SYNTAX
      <TableDelete>.delete()

RETURNS
       This TableDelete object.

//@<OUT> Help on execute
NAME
      execute - Executes the delete operation with all the configured options.

SYNTAX
      <TableDelete>.execute()

RETURNS
       A Result object.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableDelete>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on limit
NAME
      limit - Sets the maximum number of rows to be deleted by the operation.

SYNTAX
      <TableDelete>.limit(numberOfRows)

WHERE
      numberOfRows: The maximum number of rows to be deleted.

RETURNS
       This TableDelete object.

DESCRIPTION
      If used, the operation will delete only numberOfRows rows.

//@<OUT> Help on orderBy
NAME
      orderBy - Sets the order in which the records will be deleted.

SYNTAX
      <TableDelete>.orderBy(sortCriteria)
      <TableDelete>.orderBy(sortCriterion[, sortCriterion, ...])

RETURNS
       This TableDelete object.

DESCRIPTION
      If used the records will be deleted in the order established by the sort
      criteria.

      The elements of sortExprStr list are strings defining the column name on
      which the sorting will be based.

      The format is as follows: columnIdentifier [ ASC | DESC ]

      If no order criteria is specified, ASC will be used by default.

//@<OUT> Help on where
NAME
      where - Sets the search condition to filter the records to be deleted
              from the Table.

SYNTAX
      <TableDelete>.where([expression])

WHERE
      expression: Condition to filter the records to be deleted.

RETURNS
       This TableDelete object.

DESCRIPTION
      If used, only those rows satisfying the expression will be deleted

      The expression supports parameter binding.

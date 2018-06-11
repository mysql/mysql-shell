#@ __global__
||

#@<OUT> tabledelete
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

      order_by(...)
            Sets the order in which the records will be deleted.

      where([expression])
            Sets the search condition to filter the records to be deleted from
            the Table.

#@<OUT> tabledelete.execute
NAME
      execute - Executes the delete operation with all the configured options.

SYNTAX
      <TableDelete>.execute()

RETURNS
       A Result object.

#@<OUT> tabledelete.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableDelete>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> tabledelete.limit
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

#@<OUT> tabledelete.order_by
NAME
      order_by - Sets the order in which the records will be deleted.

SYNTAX
      <TableDelete>.order_by(sortCriteria)
      <TableDelete>.order_by(sortCriterion[, sortCriterion, ...])

RETURNS
       This TableDelete object.

DESCRIPTION
      If used the records will be deleted in the order established by the sort
      criteria.

      The elements of sortExprStr list are strings defining the column name on
      which the sorting will be based.

      The format is as follows: columnIdentifier [ ASC | DESC ]

      If no order criteria is specified, ASC will be used by default.

#@<OUT> tabledelete.where
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

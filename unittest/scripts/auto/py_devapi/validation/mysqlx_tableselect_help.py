#@ __global__
||

#@<OUT> tableselect
NAME
      TableSelect - Operation to retrieve rows from a table.

DESCRIPTION
      A TableSelect represents a query to retrieve rows from a Table. It is is
      created through the select function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      execute()
            Executes the select operation with all the configured options.

      group_by(...)
            Sets a grouping criteria for the retrieved rows.

      having(condition)
            Sets a condition for records to be considered in aggregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be returned on the select
            operation.

      lock_exclusive([lockContention])
            Instructs the server to acquire an exclusive lock on rows matched
            by this find operation.

      lock_shared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      offset(numberOfRows)
            Sets number of rows to skip on the resultset when a limit has been
            defined.

      order_by(...)
            Sets the order in which the records will be retrieved.

      select(...)
            Defines the columns to be retrieved from the table.

      where(expression)
            Sets the search condition to filter the records to be retrieved
            from the Table.

#@<OUT> tableselect.bind
NAME
      bind - Binds a value to a specific placeholder used on this operation.

SYNTAX
      <TableSelect>.bind(name, value)

WHERE
      name: The name of the placeholder to which the value will be bound.
      value: The value to be bound on the placeholder.

RETURNS
      This TableSelect object.

DESCRIPTION
      Binds the given value to the placeholder with the specified name.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute() method is called.

#@<OUT> tableselect.execute
NAME
      execute - Executes the select operation with all the configured options.

SYNTAX
      <TableSelect>.execute()

RETURNS
      A RowResult object that can be used to traverse the rows returned by this
      operation.

#@<OUT> tableselect.group_by
NAME
      group_by - Sets a grouping criteria for the retrieved rows.

SYNTAX
      <TableSelect>.group_by(columnList)
      <TableSelect>.group_by(column[, column, ...])

RETURNS
      This TableSelect object.

#@<OUT> tableselect.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableSelect>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> tableselect.limit
NAME
      limit - Sets the maximum number of rows to be returned on the select
              operation.

SYNTAX
      <TableSelect>.limit(numberOfRows)

WHERE
      numberOfRows: The maximum number of rows to be retrieved.

RETURNS
      This TableSelect object.

DESCRIPTION
      If used, the operation will return at most numberOfRows rows.

      This function can be called every time the statement is executed.

#@<OUT> tableselect.lock_exclusive
NAME
      lock_exclusive - Instructs the server to acquire an exclusive lock on
                       rows matched by this find operation.

SYNTAX
      <TableSelect>.lock_exclusive([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle rows that are already
                      locked.

RETURNS
      This TableSelect object.

DESCRIPTION
      When this function is called, the selected rows will be locked for read
      operations, they will not be retrievable by other session.

      The acquired locks will be released when the current transaction is
      committed or rolled back.

      The lockContention parameter defines the behavior of the operation if
      another session contains a lock to matching rows.

      The lockContention can be specified using the following constants:

      - mysqlx.LockContention.DEFAULT
      - mysqlx.LockContention.NOWAIT
      - mysqlx.LockContention.SKIP_LOCKED

      The lockContention can also be specified using the following string
      literals (no case sensitive):

      - 'DEFAULT'
      - 'NOWAIT'
      - 'SKIP_LOCKED'

      If no lockContention or the default is specified, the operation will
      block if another session already holds a lock on matching rows until the
      lock is released.

      If lockContention is set to NOWAIT and another session already holds a
      lock on matching rows, the operation will not block and an error will be
      generated.

      If lockContention is set to SKIP_LOCKED and another session already holds
      a lock on matching rows, the operation will not block and will return
      only those rows not having an exclusive lock.

      This operation only makes sense within a transaction.

#@<OUT> tableselect.lock_shared
NAME
      lock_shared - Instructs the server to acquire shared row locks in
                    documents matched by this find operation.

SYNTAX
      <TableSelect>.lock_shared([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle rows that are already
                      locked.

RETURNS
      This TableSelect object.

DESCRIPTION
      When this function is called, the selected rows will be locked for write
      operations, they may be retrieved on a different session, but no updates
      will be allowed.

      The acquired locks will be released when the current transaction is
      committed or rolled back.

      The lockContention parameter defines the behavior of the operation if
      another session contains an exclusive lock to matching rows.

      The lockContention can be specified using the following constants:

      - mysqlx.LockContention.DEFAULT
      - mysqlx.LockContention.NOWAIT
      - mysqlx.LockContention.SKIP_LOCKED

      The lockContention can also be specified using the following string
      literals (no case sensitive):

      - 'DEFAULT'
      - 'NOWAIT'
      - 'SKIP_LOCKED'

      If no lockContention or the default is specified, the operation will
      block if another session already holds an exclusive lock on matching rows
      until the lock is released.

      If lockContention is set to NOWAIT and another session already holds an
      exclusive lock on matching rows, the operation will not block and an
      error will be generated.

      If lockContention is set to SKIP_LOCKED and another session already holds
      an exclusive lock on matching rows, the operation will not block and will
      return only those rows not having an exclusive lock.

      This operation only makes sense within a transaction.

#@<OUT> tableselect.order_by
NAME
      order_by - Sets the order in which the records will be retrieved.

SYNTAX
      <TableSelect>.order_by(sortCriteriaList)
      <TableSelect>.order_by(sortCriterion[, sortCriterion, ...])

RETURNS
      This TableSelect object.

DESCRIPTION
      If used, the TableSelect operation will return the records sorted with
      the defined criteria.

      Every defined sort criterion follows the format:

      name [ ASC | DESC ]

      ASC is used by default if the sort order is not specified.

#@<OUT> tableselect.where
NAME
      where - Sets the search condition to filter the records to be retrieved
              from the Table.

SYNTAX
      <TableSelect>.where(expression)

WHERE
      expression: A condition to filter the records to be retrieved.

RETURNS
      This TableSelect object.

DESCRIPTION
      If used, only those rows satisfying the expression will be retrieved.

      The expression supports parameter binding.

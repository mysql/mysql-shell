//@ Initialization
||

//@<OUT> TableSelect help
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

      groupBy(...)
            Sets a grouping criteria for the retrieved rows.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be returned on the select
            operation.

      lockExclusive([lockContention])
            Instructs the server to acquire an exclusive lock on rows matched
            by this find operation.

      lockShared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      offset(numberOfRows)
            Sets number of rows to skip on the resultset when a limit has been
            defined.

      orderBy(...)
            Sets the order in which the records will be retrieved.

      select(...)
            Defines the columns to be retrieved from the table.

      where([expression])
            Sets the search condition to filter the records to be retrieved
            from the Table.

//@<OUT> Help on bind
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
      Binds a value to a specific placeholder used on this operation.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

//@<OUT> Help on execute
NAME
      execute - Executes the select operation with all the configured options.

SYNTAX
      <TableSelect>.execute()

RETURNS
       A RowResult object that can be used to traverse the rows returned by
      this operation.

//@<OUT> Help on groupBy
NAME
      groupBy - Sets a grouping criteria for the retrieved rows.

SYNTAX
      <TableSelect>.groupBy(searchExprStrList)
      <TableSelect>.groupBy(searchExprStr, searchExprStr, ...)

RETURNS
       This TableSelect object.

//@<OUT> Help on having
NAME
      having - Sets a condition for records to be considered in agregate
               function operations.

SYNTAX
      <TableSelect>.having(condition)

WHERE
      condition: A condition to be used with agregate functions.

RETURNS
       This TableSelect object.

DESCRIPTION
      If used the TableSelect operation will only consider the records matching
      the established criteria.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableSelect>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on limit
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

//@<OUT> Help on lockExclusive
NAME
      lockExclusive - Instructs the server to acquire an exclusive lock on rows
                      matched by this find operation.

SYNTAX
      <TableSelect>.lockExclusive([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle rows that are already
                      locked.

RETURNS
       This TableSelect object.

DESCRIPTION
      When this function is called, the selected rows will belocked for read
      operations, they will not be retrievable by other session.

      The acquired locks will be released when the current transaction is
      commited or rolled back.

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

//@<OUT> Help on lockShared
NAME
      lockShared - Instructs the server to acquire shared row locks in
                   documents matched by this find operation.

SYNTAX
      <TableSelect>.lockShared([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle rows that are already
                      locked.

RETURNS
       This TableSelect object.

DESCRIPTION
      When this function is called, the selected rows will belocked for write
      operations, they may be retrieved on a different session, but no updates
      will be allowed.

      The acquired locks will be released when the current transaction is
      commited or rolled back.

      The lockContention parameter defines the behavior of the operation if
      another session contains an exlusive lock to matching rows.

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

//@<OUT> Help on offset
NAME
      offset - Sets number of rows to skip on the resultset when a limit has
               been defined.

SYNTAX
      <TableSelect>.offset(numberOfRows)

WHERE
      numberOfRows: The number of rows to skip before start including them on
                    the RowResult.

RETURNS
       This CollectionFind object.

DESCRIPTION
      If used, the first numberOfRows records will not be included on the
      result.

//@<OUT> Help on orderBy
NAME
      orderBy - Sets the order in which the records will be retrieved.

SYNTAX
      <TableSelect>.orderBy(sortCriterion[, sortCriterion, ...])
      <TableSelect>.orderBy(sortCriteria)

RETURNS
       This TableSelect object.

DESCRIPTION
      If used the records will be sorted with the defined criteria.

      The elements of sortExprStr list are strings defining the column name on
      which the sorting will be based.

      The format is as follows: columnIdentifier [ ASC | DESC ]

      If no order criteria is specified, ascending will be used by default.

//@<OUT> Help on select
NAME
      select - Defines the columns to be retrieved from the table.

SYNTAX
      <TableSelect>.select()
      <TableSelect>.select(colDefArray)
      <TableSelect>.select(colDef, colDef, ...)

RETURNS
       This TableSelect object.

DESCRIPTION
      Defines the columns that will be retrieved from the Table.

      To define the column list either use a list object containing the column
      definition or pass each column definition on a separate parameter

      If the function is called without specifying any column definition, all
      the columns in the table will be retrieved.

//@<OUT> Help on where
NAME
      where - Sets the search condition to filter the records to be retrieved
              from the Table.

SYNTAX
      <TableSelect>.where([expression])

WHERE
      expression: Condition to filter the records to be retrieved.

RETURNS
       This TableSelect object.

DESCRIPTION
      If used, only those rows satisfying the expression will be retrieved

      The expression supports parameter binding.

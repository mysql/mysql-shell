//@ Initialization
||

//@<OUT> Table.help
NAME
      Table - Represents a Table on an Schema, retrieved with a session created
              using mysqlx module.

DESCRIPTION
      Represents a Table on an Schema, retrieved with a session created using
      mysqlx module.

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

FUNCTIONS
      delete()
            Creates a record deletion handler.

      existsInDatabase()
            Verifies if this object exists in the database.

      getName()
            Returns the name of this database object.

      getSchema()
            Returns the Schema object of this database object.

      getSession()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

      insert(...)
            Creates TableInsert object to insert new records into the table.

      isView()
            Indicates whether this Table object represents a View on the
            database.

      select(...)
            Creates a TableSelect object to retrieve rows from the table.

      update()
            Creates a record update handler.

//@<OUT> Help on name
NAME
      name - The name of this database object.

SYNTAX
      <Table>.name

//@<OUT> Help on schema
NAME
      schema - The Schema object of this database object.

SYNTAX
      <Table>.schema

//@<OUT> Help on session
NAME
      session - The Session object of this database object.

SYNTAX
      <Table>.session

//@<OUT> Help on delete
NAME
      delete - Creates a record deletion handler.

SYNTAX
      Table.delete()
           [.where([expression])]
           [.orderBy(...)]
           [.limit(numberOfRows)]
           [.bind(name, value)]
           [.execute()]

DESCRIPTION
      This function creates a TableDelete object which is a record deletion
      handler.

      The TableDelete class has several functions that allow specifying what
      should be deleted and how, if a searchCondition was specified, it will be
      set on the handler.

      The deletion is done when the execute function is called on the handler.

      delete()
            Initializes the deletion operation.

      where([expression])
            If used, only those rows satisfying the expression will be deleted

            The expression supports parameter binding.

      orderBy(...)
            This function has the following overloads:

            - orderBy(sortCriteria)
            - orderBy(sortCriterion[, sortCriterion, ...])

            If used the records will be deleted in the order established by the
            sort criteria.

            The elements of sortExprStr list are strings defining the column
            name on which the sorting will be based.

            The format is as follows: columnIdentifier [ ASC | DESC ]

            If no order criteria is specified, ASC will be used by default.

      limit(numberOfRows)
            If used, the operation will delete only numberOfRows rows.

            This function can be called every time the statement is executed.

      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

            An error will be raised if the placeholder indicated by name does
            not exist.

            This function must be called once for each used placeholder or an
            error will be raised when the execute method is called.

      execute()
            Executes the delete operation with all the configured options.

//@<OUT> Help on existsInDatabase
NAME
      existsInDatabase - Verifies if this object exists in the database.

SYNTAX
      <Table>.existsInDatabase()

RETURNS
       A boolean indicating if the object still exists on the database.

//@<OUT> Help on getName
NAME
      getName - Returns the name of this database object.

SYNTAX
      <Table>.getName()

//@<OUT> Help on getSchema
NAME
      getSchema - Returns the Schema object of this database object.

SYNTAX
      <Table>.getSchema()

RETURNS
       The Schema object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Schema: if the object was created/retrieved using a Schema instance.
      - ClassicSchema: if the object was created/retrieved using an
        ClassicSchema.
      - Null: if this database object is a Schema or ClassicSchema.

//@<OUT> Help on getSession
NAME
      getSession - Returns the Session object of this database object.

SYNTAX
      <Table>.getSession()

RETURNS
       The Session object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Session: if the object was created/retrieved using an Session instance.
      - ClassicSession: if the object was created/retrieved using an
        ClassicSession.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Table>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on insert
NAME
      insert - Creates TableInsert object to insert new records into the table.

SYNTAX
      Table.insert(...)
           [.values(value[, value, ...])]
           [.execute()]

DESCRIPTION
      The TableInsert class has other functions that allow specifying the way
      the insertion occurs.

      The insertion is done when the execute method is called on the handler.

      insert(...)
            This function has the following overloads:

            - insert()
            - insert(columnList)
            - insert(column[, column, ...])
            - insert({column:value[, column:value, ...]})

            An insert operation requires the values to be inserted, optionally
            the target colums can be defined.

            If this function is called without any parameter, no column names
            will be defined yet.

            The column definition can be done by three ways:

            - Passing to the function call an array with the columns names that
              will be used in the insertion.
            - Passing multiple parameters, each of them being a column name.
            - Passing a JSON document, using the column names as the document
              keys.

            If the columns are defined using either an array or multiple
            parameters, the values must match the defined column names in order
            and data type.

            If a JSON document was used, the operation is ready to be completed
            and it will insert the associated value into the corresponding
            column.

            If no columns are defined, insertion will suceed if the provided
            values match the database columns in number and data types.

      values(value[, value, ...])
            Each parameter represents the value for a column in the target
            table.

            If the columns were defined on the insert function, the number of
            values on this function must match the number of defined columns.

            If no column was defined, the number of parameters must match the
            number of columns on the target Table.

            This function is not available when the insert is called passing a
            JSON object with columns and values.

            Using Expressions As Values

            If a mysqlx.expr(...) object is defined as a value, it will be
            evaluated in the server, the resulting value will be inserted into
            the record.

      execute()
            Executes the insert operation.

//@<OUT> Help on isView
NAME
      isView - Indicates whether this Table object represents a View on the
               database.

SYNTAX
      <Table>.isView()

RETURNS
       True if the Table represents a View on the database, False if represents
      a Table.

//@<OUT> Help on select
NAME
      select - Creates a TableSelect object to retrieve rows from the table.

SYNTAX
      Table.select(...)
           [.where([expression])]
           [.groupBy(...)[.having(condition)]]
           [.orderBy(...)]
           [.limit(numberOfRows)[.offset(numberOfRows)]]
           [.lockShared([lockContention])]
           [.lockExclusive([lockContention])]
           [.bind(name, value)]
           [.execute()]

DESCRIPTION
      This function creates a TableSelect object which is a record selection
      handler.

      This handler will retrieve all the columns for each included record.

      The TableSelect class has several functions that allow specifying what
      records should be retrieved from the table, if a searchCondition was
      specified, it will be set on the handler.

      The selection will be returned when the execute function is called on the
      handler.

      This handler will retrieve only the columns specified on the columns list
      for each included record.

      Each column on the list should be a string identifying the column name,
      alias is supported.

      select(...)
            This function has the following overloads:

            - select()
            - select(colDefArray)
            - select(colDef, colDef, ...)

            Defines the columns that will be retrieved from the Table.

            To define the column list either use a list object containing the
            column definition or pass each column definition on a separate
            parameter

            If the function is called without specifying any column definition,
            all the columns in the table will be retrieved.

      where([expression])
            If used, only those rows satisfying the expression will be
            retrieved

            The expression supports parameter binding.

      groupBy(...)
            This function has the following overloads:

            - groupBy(searchExprStrList)
            - groupBy(searchExprStr, searchExprStr, ...)

            Sets a grouping criteria for the retrieved rows.

      having(condition)
            If used the TableSelect operation will only consider the records
            matching the established criteria.

      orderBy(...)
            This function has the following overloads:

            - orderBy(sortCriterion[, sortCriterion, ...])
            - orderBy(sortCriteria)

            If used the records will be sorted with the defined criteria.

            The elements of sortExprStr list are strings defining the column
            name on which the sorting will be based.

            The format is as follows: columnIdentifier [ ASC | DESC ]

            If no order criteria is specified, ascending will be used by
            default.

      limit(numberOfRows)
            If used, the operation will return at most numberOfRows rows.

            This function can be called every time the statement is executed.

      offset(numberOfRows)
            If used, the first numberOfRows records will not be included on the
            result.

      lockShared([lockContention])
            When this function is called, the selected rows will belocked for
            write operations, they may be retrieved on a different session, but
            no updates will be allowed.

            The acquired locks will be released when the current transaction is
            commited or rolled back.

            The lockContention parameter defines the behavior of the operation
            if another session contains an exlusive lock to matching rows.

            The lockContention can be specified using the following constants:

            - mysqlx.LockContention.DEFAULT
            - mysqlx.LockContention.NOWAIT
            - mysqlx.LockContention.SKIP_LOCKED

            The lockContention can also be specified using the following string
            literals (no case sensitive):

            - 'DEFAULT'
            - 'NOWAIT'
            - 'SKIP_LOCKED'

            If no lockContention or the default is specified, the operation
            will block if another session already holds an exclusive lock on
            matching rows until the lock is released.

            If lockContention is set to NOWAIT and another session already
            holds an exclusive lock on matching rows, the operation will not
            block and an error will be generated.

            If lockContention is set to SKIP_LOCKED and another session already
            holds an exclusive lock on matching rows, the operation will not
            block and will return only those rows not having an exclusive lock.

            This operation only makes sense within a transaction.

      lockExclusive([lockContention])
            When this function is called, the selected rows will belocked for
            read operations, they will not be retrievable by other session.

            The acquired locks will be released when the current transaction is
            commited or rolled back.

            The lockContention parameter defines the behavior of the operation
            if another session contains a lock to matching rows.

            The lockContention can be specified using the following constants:

            - mysqlx.LockContention.DEFAULT
            - mysqlx.LockContention.NOWAIT
            - mysqlx.LockContention.SKIP_LOCKED

            The lockContention can also be specified using the following string
            literals (no case sensitive):

            - 'DEFAULT'
            - 'NOWAIT'
            - 'SKIP_LOCKED'

            If no lockContention or the default is specified, the operation
            will block if another session already holds a lock on matching rows
            until the lock is released.

            If lockContention is set to NOWAIT and another session already
            holds a lock on matching rows, the operation will not block and an
            error will be generated.

            If lockContention is set to SKIP_LOCKED and another session already
            holds a lock on matching rows, the operation will not block and
            will return only those rows not having an exclusive lock.

            This operation only makes sense within a transaction.

      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

            An error will be raised if the placeholder indicated by name does
            not exist.

            This function must be called once for each used placeholder or an
            error will be raised when the execute method is called.

      execute()
            Executes the select operation with all the configured options.

//@<OUT> Help on update
NAME
      update - Creates a record update handler.

SYNTAX
      Table.update()
           .set(attribute, value)
           [.where([expression])]
           [.orderBy(...)]
           [.limit(numberOfRows)]
           [.bind(name, value)]
           [.execute()]

DESCRIPTION
      This function creates a TableUpdate object which is a record update
      handler.

      The TableUpdate class has several functions that allow specifying the way
      the update occurs, if a searchCondition was specified, it will be set on
      the handler.

      The update is done when the execute function is called on the handler.

      update()
            Initializes the update operation.

      set(attribute, value)
            Adds an opertion into the update handler to update a column value
            in on the records that were included on the selection filter and
            limit.

      where([expression])
            If used, only those rows satisfying the expression will be updated

            The expression supports parameter binding.

      orderBy(...)
            This function has the following overloads:

            - orderBy(sortCriteria)
            - orderBy(sortCriterion[, sortCriterion, ...])

            If used the records will be updated in the order established by the
            sort criteria.

            The elements of sortExprStr list are strings defining the column
            name on which the sorting will be based.

            The format is as follows: columnIdentifier [ ASC | DESC ]

            If no order criteria is specified, ASC will be used by default.

      limit(numberOfRows)
            If used, the operation will update only numberOfRows rows.

            This function can be called every time the statement is executed.

      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

            An error will be raised if the placeholder indicated by name does
            not exist.

            This function must be called once for each used placeholder or an
            error will be raised when the execute method is called.

      execute()
            Executes the update operation with all the configured options.


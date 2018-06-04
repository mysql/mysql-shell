#@ __global__
||

#@<OUT> tableinsert
NAME
      TableInsert - Operation to insert data into a table.

DESCRIPTION
      A TableInsert object is created through the insert function on the Table
      class.

FUNCTIONS
      execute()
            Executes the insert operation.

      help()
            Provides help about this class and it's members

      insert(...)
            Initializes the insertion operation.

      values(value[, value, ...])
            Adds a new row to the insert operation with the given values.

#@<OUT> tableinsert.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <TableInsert>.help()

#@<OUT> tableinsert.insert
NAME
      insert - Initializes the insertion operation.

SYNTAX
      <TableInsert>.insert()
      <TableInsert>.insert(columnList)
      <TableInsert>.insert(column[, column, ...])
      <TableInsert>.insert({column:value[, column:value, ...]})

RETURNS
       This TableInsert object.

DESCRIPTION
      An insert operation requires the values to be inserted, optionally the
      target colums can be defined.

      If this function is called without any parameter, no column names will be
      defined yet.

      The column definition can be done by three ways:

      - Passing to the function call an array with the columns names that will
        be used in the insertion.
      - Passing multiple parameters, each of them being a column name.
      - Passing a JSON document, using the column names as the document keys.

      If the columns are defined using either an array or multiple parameters,
      the values must match the defined column names in order and data type.

      If a JSON document was used, the operation is ready to be completed and
      it will insert the associated value into the corresponding column.

      If no columns are defined, insertion will suceed if the provided values
      match the database columns in number and data types.

#@<OUT> tableinsert.values
NAME
      values - Adds a new row to the insert operation with the given values.

SYNTAX
      <TableInsert>.values(value[, value, ...])

RETURNS
       This TableInsert object.

DESCRIPTION
      Each parameter represents the value for a column in the target table.

      If the columns were defined on the insert function, the number of values
      on this function must match the number of defined columns.

      If no column was defined, the number of parameters must match the number
      of columns on the target Table.

      This function is not available when the insert is called passing a JSON
      object with columns and values.

      Using Expressions As Values

      If a mysqlx.expr(...) object is defined as a value, it will be evaluated
      in the server, the resulting value will be inserted into the record.

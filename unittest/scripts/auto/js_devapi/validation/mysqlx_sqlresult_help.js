//@ Initialization
||

//@<OUT> SqlResult help
NAME
      SqlResult - Allows browsing through the result information after
                  performing an operation on the database done through
                  Session.sql

DESCRIPTION
      Allows browsing through the result information after performing an
      operation on the database done through Session.sql

PROPERTIES
      affectedItemsCount
            Same as getAffectedItemsCount

      autoIncrementValue
            Same as getAutoIncrementValue

      columnCount
            Same as getColumnCount

      columnNames
            Same as getColumnNames

      columns
            Same as getColumns

      executionTime
            Same as getExecutionTime

      warnings
            Same as getWarnings

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      fetchAll()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetchOne()
            Retrieves the next Row on the RowResult.

      fetchOneObject()
            Retrieves the next Row on the result and returns it as an object.

      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getAutoIncrementValue()
            Returns the identifier for the last record inserted.

      getColumnCount()
            Retrieves the number of columns on the current result.

      getColumnNames()
            Gets the columns on the current result.

      getColumns()
            Gets the column metadata for the columns on the active result.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getWarnings()
            Retrieves the warnings generated by the executed operation.

      getWarningsCount()
            The number of warnings produced by the last statement execution.

      hasData()
            Returns true if the last statement execution has a result set.

      help([member])
            Provides help about this class and it's members

      nextResult()
            Prepares the SqlResult to start reading data from the next Result
            (if many results were returned).

//@<OUT> Help on affectedItemsCount
NAME
      affectedItemsCount - Same as getAffectedItemsCount

SYNTAX
      <SqlResult>.affectedItemsCount

//@<OUT> Help on autoIncrementValue
NAME
      autoIncrementValue - Same as getAutoIncrementValue

SYNTAX
      <SqlResult>.autoIncrementValue

//@<OUT> Help on columnCount
NAME
      columnCount - Same as getColumnCount

SYNTAX
      <SqlResult>.columnCount

//@<OUT> Help on columnNames
NAME
      columnNames - Same as getColumnNames

SYNTAX
      <SqlResult>.columnNames

//@<OUT> Help on columns
NAME
      columns - Same as getColumns

SYNTAX
      <SqlResult>.columns

//@<OUT> Help on executionTime
NAME
      executionTime - Same as getExecutionTime

SYNTAX
      <SqlResult>.executionTime

//@<OUT> Help on warnings
NAME
      warnings - Same as getWarnings

SYNTAX
      <SqlResult>.warnings

//@<OUT> Help on warningsCount
NAME
      warningsCount - Same as getWarningsCount

SYNTAX
      <SqlResult>.warningsCount

//@<OUT> Help on fetchAll
NAME
      fetchAll - Returns a list of DbDoc objects which contains an element for
                 every unread document.

SYNTAX
      <SqlResult>.fetchAll()

RETURNS
      A List of DbDoc objects.

//@<OUT> Help on fetchOne
NAME
      fetchOne - Retrieves the next Row on the RowResult.

SYNTAX
      <SqlResult>.fetchOne()

RETURNS
      A Row object representing the next record on the result.

//@<OUT> Help on fetchOneObject
NAME
      fetchOneObject - Retrieves the next Row on the result and returns it as
                       an object.

SYNTAX
      <SqlResult>.fetchOneObject()

RETURNS
      A dictionary containing the row information.

DESCRIPTION
      The column names will be used as keys in the returned dictionary and the
      column data will be used as the key values.

      If a column is a valid identifier it will be accessible as an object
      attribute as <dict>.<column>.

      If a column is not a valid identifier, it will be accessible as a
      dictionary key as <dict>[<column>].

//@<OUT> Help on getAffectedItemsCount
NAME
      getAffectedItemsCount - The the number of affected items for the last
                              operation.

SYNTAX
      <SqlResult>.getAffectedItemsCount()

RETURNS
      the number of affected items.

DESCRIPTION
      Returns the number of records affected by the executed operation.

//@<OUT> Help on getAutoIncrementValue
NAME
      getAutoIncrementValue - Returns the identifier for the last record
                              inserted.

SYNTAX
      <SqlResult>.getAutoIncrementValue()

DESCRIPTION
      Note that this value will only be set if the executed statement inserted
      a record in the database and an ID was automatically generated.

//@<OUT> Help on getColumnCount
NAME
      getColumnCount - Retrieves the number of columns on the current result.

SYNTAX
      <SqlResult>.getColumnCount()

RETURNS
      the number of columns on the current result.

//@<OUT> Help on getColumnNames
NAME
      getColumnNames - Gets the columns on the current result.

SYNTAX
      <SqlResult>.getColumnNames()

RETURNS
      A list with the names of the columns returned on the active result.

//@<OUT> Help on getColumns
NAME
      getColumns - Gets the column metadata for the columns on the active
                   result.

SYNTAX
      <SqlResult>.getColumns()

RETURNS
      a list of Column objects containing information about the columns
      included on the active result.

//@<OUT> Help on getExecutionTime
NAME
      getExecutionTime - Retrieves a string value indicating the execution time
                         of the executed operation.

SYNTAX
      <SqlResult>.getExecutionTime()

//@<OUT> Help on getWarnings
NAME
      getWarnings - Retrieves the warnings generated by the executed operation.

SYNTAX
      <SqlResult>.getWarnings()

RETURNS
      A list containing a warning object for each generated warning.

DESCRIPTION
      This is the same value than C API mysql_warning_count, see
      https://dev.mysql.com/doc/c-api/en/mysql-warning-count.html

      Each warning object contains a key/value pair describing the information
      related to a specific warning.

      This information includes: Level, Code and Message.

//@<OUT> Help on getWarningsCount
NAME
      getWarningsCount - The number of warnings produced by the last statement
                         execution.

SYNTAX
      <SqlResult>.getWarningsCount()

RETURNS
      the number of warnings.

DESCRIPTION
      This is the same value than C API mysql_warning_count, see
      https://dev.mysql.com/doc/c-api/en/mysql-warning-count.html

      See getWarnings() for more details.

//@<OUT> Help on hasData
NAME
      hasData - Returns true if the last statement execution has a result set.

SYNTAX
      <SqlResult>.hasData()

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <SqlResult>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

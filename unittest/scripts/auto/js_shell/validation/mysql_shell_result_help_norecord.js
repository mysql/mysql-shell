//@<OUT> ShellResult help
NAME
      ShellResult - Encapsulates custom query result and metadata.

DESCRIPTION
      This class allows access to a custom result set created through
      shell.createResult.

PROPERTIES
      affectedItemsCount
            The the number of affected items for the last operation.

      autoIncrementValue
            Returns the last insert id auto generated (from an insert
            operation).

      columnCount
            Retrieves the number of columns on the current result.

      columnNames
            Gets the columns on the current result.

      columns
            Gets the column metadata for the columns on the current result.

      executionTime
            Retrieves a string value indicating the execution time of the
            operation that generated this result.

      info
            Retrieves a string providing additional information about the
            result.

      warnings
            Retrieves the warnings generated by operation that generated this
            result.

      warningsCount
            The number of warnings produced by the operation that generated
            this result.

FUNCTIONS
      fetchAll()
            Returns a list of Row objects which contains an element for every
            record left on the result.

      fetchOne()
            Retrieves the next Row on the ShellResult.

      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getAutoIncrementValue()
            Returns the last insert id auto generated (from an insert
            operation).

      getColumnCount()
            Retrieves the number of columns on the current result.

      getColumnNames()
            Gets the columns on the current result.

      getColumns()
            Gets the column metadata for the columns on the current result.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            operation that generated this result.

      getInfo()
            Retrieves a string providing additional information about the
            result.

      getWarnings()
            Retrieves the warnings generated by operation that generated this
            result.

      getWarningsCount()
            The number of warnings produced by the operation that generated
            this result.

      hasData()
            Returns true if there are records in the result.

      help([member])
            Provides help about this class and it's members

      nextResult()
            Prepares the ShellResult to start reading data from the next Result
            (if many results were returned).

//@<OUT> Help on affectedItemsCount
NAME
      affectedItemsCount - The the number of affected items for the last
                           operation.

SYNTAX
      <ShellResult>.affectedItemsCount

//@<OUT> Help on columnCount
NAME
      columnCount - Retrieves the number of columns on the current result.

SYNTAX
      <ShellResult>.columnCount

//@<OUT> Help on columnNames
NAME
      columnNames - Gets the columns on the current result.

SYNTAX
      <ShellResult>.columnNames

//@<OUT> Help on columns
NAME
      columns - Gets the column metadata for the columns on the current result.

SYNTAX
      <ShellResult>.columns

//@<OUT> Help on executionTime
NAME
      executionTime - Retrieves a string value indicating the execution time of
                      the operation that generated this result.

SYNTAX
      <ShellResult>.executionTime

//@<OUT> Help on info
NAME
      info - Retrieves a string providing additional information about the
             result.

SYNTAX
      <ShellResult>.info

//@<OUT> Help on warningsCount
NAME
      warningsCount - The number of warnings produced by the operation that
                      generated this result.

SYNTAX
      <ShellResult>.warningsCount

//@<OUT> Help on warnings
NAME
      warnings - Retrieves the warnings generated by operation that generated
                 this result.

SYNTAX
      <ShellResult>.warnings

//@<OUT> Help on fetchAll
NAME
      fetchAll - Returns a list of Row objects which contains an element for
                 every record left on the result.

SYNTAX
      <ShellResult>.fetchAll()

RETURNS
      A List of Row objects.

DESCRIPTION
      This function will return a Row for every record on the resultset unless
      fetchOne is called before, in such case it will return a Row for each of
      the remaining records on the resultset.

//@<OUT> Help on fetchOne
NAME
      fetchOne - Retrieves the next Row on the ShellResult.

SYNTAX
      <ShellResult>.fetchOne()

RETURNS
      A Row object representing the next record in the result.

//@<OUT> Help on getAffectedItemsCount
NAME
      getAffectedItemsCount - The the number of affected items for the last
                              operation.

SYNTAX
      <ShellResult>.getAffectedItemsCount()

RETURNS
      the number of affected items.

//@<OUT> Help on getColumnCount
NAME
      getColumnCount - Retrieves the number of columns on the current result.

SYNTAX
      <ShellResult>.getColumnCount()

RETURNS
      the number of columns on the current result.

//@<OUT> Help on getColumnNames
NAME
      getColumnNames - Gets the columns on the current result.

SYNTAX
      <ShellResult>.getColumnNames()

RETURNS
      A list with the names of the columns returned on the active result.

//@<OUT> Help on getColumns
NAME
      getColumns - Gets the column metadata for the columns on the current
                   result.

SYNTAX
      <ShellResult>.getColumns()

RETURNS
      a list of column metadata objects containing information about the
      columns included on the current result.

//@<OUT> Help on getExecutionTime
NAME
      getExecutionTime - Retrieves a string value indicating the execution time
                         of the operation that generated this result.

SYNTAX
      <ShellResult>.getExecutionTime()

//@<OUT> Help on getInfo
NAME
      getInfo - Retrieves a string providing additional information about the
                result.

SYNTAX
      <ShellResult>.getInfo()

RETURNS
      a string with the additional information.

//@<OUT> Help on getWarningsCount
NAME
      getWarningsCount - The number of warnings produced by the operation that
                         generated this result.

SYNTAX
      <ShellResult>.getWarningsCount()

RETURNS
      the number of warnings.

DESCRIPTION
      See getWarnings() for more details.

//@<OUT> Help on getWarnings
NAME
      getWarnings - Retrieves the warnings generated by operation that
                    generated this result.

SYNTAX
      <ShellResult>.getWarnings()

RETURNS
      a list containing a warning object for each generated warning.

DESCRIPTION
      Each warning object contains a key/value pair describing the information
      related to a specific warning.

      This information includes: level, code and message.

//@<OUT> Help on hasData
NAME
      hasData - Returns true if there are records in the result.

SYNTAX
      <ShellResult>.hasData()

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <ShellResult>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on nextResult
NAME
      nextResult - Prepares the ShellResult to start reading data from the next
                   Result (if many results were returned).

SYNTAX
      <ShellResult>.nextResult()

RETURNS
      A boolean value indicating whether there is another result or not.


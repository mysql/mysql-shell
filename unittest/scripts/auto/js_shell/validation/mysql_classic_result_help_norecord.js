//@<OUT> ClassicResult help
NAME
      ClassicResult - Allows browsing through the result information after
                      performing an operation on the database through the MySQL
                      Protocol.

DESCRIPTION
      This class allows access to the result set from the classic MySQL data
      model to be retrieved from Dev API queries.

PROPERTIES
      affectedItemsCount
            Same as getAffectedItemsCount

      autoIncrementValue
            Returns the last insert id auto generated (from an insert
            operation).

      columnCount
            Retrieves the number of columns on the current result.

      columnNames
            Gets the columns on the current result.

      columns
            Gets the column metadata for the columns on the active result.

      executionTime
            Retrieves a string value indicating the execution time of the
            executed operation.

      info
            Retrieves a string providing information about the most recently
            executed statement.

      statementId
            Retrieves the statement id of the query that produced this result.

      warnings
            Retrieves the warnings generated by the executed operation.

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      fetchAll()
            Returns a list of Row objects which contains an element for every
            record left on the result.

      fetchOne()
            Retrieves the next Row on the ClassicResult.

      fetchOneObject()
            Retrieves the next Row on the result and returns it as an object.

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
            Gets the column metadata for the columns on the active result.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getInfo()
            Retrieves a string providing information about the most recently
            executed statement.

      getStatementId()
            Retrieves the statement id of the query that produced this result.

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
      <ClassicResult>.affectedItemsCount

//@<OUT> Help on autoIncrementValue
NAME
      autoIncrementValue - Returns the last insert id auto generated (from an
                           insert operation).

SYNTAX
      <ClassicResult>.autoIncrementValue

//@<OUT> Help on columnCount
NAME
      columnCount - Retrieves the number of columns on the current result.

SYNTAX
      <ClassicResult>.columnCount

//@<OUT> Help on columnNames
NAME
      columnNames - Gets the columns on the current result.

SYNTAX
      <ClassicResult>.columnNames

//@<OUT> Help on columns
NAME
      columns - Gets the column metadata for the columns on the active result.

SYNTAX
      <ClassicResult>.columns

//@<OUT> Help on executionTime
NAME
      executionTime - Retrieves a string value indicating the execution time of
                      the executed operation.

SYNTAX
      <ClassicResult>.executionTime

//@<OUT> Help on info
NAME
      info - Retrieves a string providing information about the most recently
             executed statement.

SYNTAX
      <ClassicResult>.info

//@<OUT> Help on warningsCount
NAME
      warningsCount - Same as getWarningsCount

SYNTAX
      <ClassicResult>.warningsCount

//@<OUT> Help on warnings
NAME
      warnings - Retrieves the warnings generated by the executed operation.

SYNTAX
      <ClassicResult>.warnings

//@<OUT> Help on fetchAll
NAME
      fetchAll - Returns a list of Row objects which contains an element for
                 every record left on the result.

SYNTAX
      <ClassicResult>.fetchAll()

RETURNS
      A List of Row objects.

DESCRIPTION
      If this function is called right after executing a query, it will return
      a Row for every record on the resultset.

      If fetchOne is called before this function, when this function is called
      it will return a Row for each of the remaining records on the resultset.

//@<OUT> Help on fetchOne
NAME
      fetchOne - Retrieves the next Row on the ClassicResult.

SYNTAX
      <ClassicResult>.fetchOne()

RETURNS
      A Row object representing the next record in the result.

//@<OUT> Help on fetchOneObject
NAME
      fetchOneObject - Retrieves the next Row on the result and returns it as
                       an object.

SYNTAX
      <ClassicResult>.fetchOneObject()

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
      <ClassicResult>.getAffectedItemsCount()

RETURNS
      the number of affected items.

//@<OUT> Help on getAutoIncrementValue
NAME
      getAutoIncrementValue - Returns the last insert id auto generated (from
                              an insert operation).

SYNTAX
      <ClassicResult>.getAutoIncrementValue()

RETURNS
      the integer representing the last insert id.

//@<OUT> Help on getColumnCount
NAME
      getColumnCount - Retrieves the number of columns on the current result.

SYNTAX
      <ClassicResult>.getColumnCount()

RETURNS
      the number of columns on the current result.

//@<OUT> Help on getColumnNames
NAME
      getColumnNames - Gets the columns on the current result.

SYNTAX
      <ClassicResult>.getColumnNames()

RETURNS
      A list with the names of the columns returned on the active result.

//@<OUT> Help on getColumns
NAME
      getColumns - Gets the column metadata for the columns on the active
                   result.

SYNTAX
      <ClassicResult>.getColumns()

RETURNS
      a list of column metadata objects containing information about the
      columns included on the active result.

//@<OUT> Help on getExecutionTime
NAME
      getExecutionTime - Retrieves a string value indicating the execution time
                         of the executed operation.

SYNTAX
      <ClassicResult>.getExecutionTime()

//@<OUT> Help on getInfo
NAME
      getInfo - Retrieves a string providing information about the most
                recently executed statement.

SYNTAX
      <ClassicResult>.getInfo()

RETURNS
      a string with the execution information.

//@<OUT> Help on getWarningsCount
NAME
      getWarningsCount - The number of warnings produced by the last statement
                         execution.

SYNTAX
      <ClassicResult>.getWarningsCount()

RETURNS
      the number of warnings.

DESCRIPTION
      This is the same value than C API mysql_warning_count, see
      https://dev.mysql.com/doc/c-api/en/mysql-warning-count.html

      See getWarnings() for more details.

//@<OUT> Help on getWarnings
NAME
      getWarnings - Retrieves the warnings generated by the executed operation.

SYNTAX
      <ClassicResult>.getWarnings()

RETURNS
      a list containing a warning object for each generated warning.

DESCRIPTION
      Each warning object contains a key/value pair describing the information
      related to a specific warning.

      This information includes: level, code and message.

//@<OUT> Help on hasData
NAME
      hasData - Returns true if the last statement execution has a result set.

SYNTAX
      <ClassicResult>.hasData()

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <ClassicResult>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on nextResult
NAME
      nextResult - Prepares the SqlResult to start reading data from the next
                   Result (if many results were returned).

SYNTAX
      <ClassicResult>.nextResult()

RETURNS
      A boolean value indicating whether there is another esult or not.


#@ __global__
||

#@<OUT> rowresult
NAME
      RowResult - Allows traversing the Row objects returned by a Table.select
                  operation.

DESCRIPTION
      Allows traversing the Row objects returned by a Table.select operation.

PROPERTIES
      affected_items_count
            Same as get_affected_items_count

      column_count
            Same as get_column_count

      column_names
            Same as get_column_names

      columns
            Same as get_columns

      execution_time
            Same as get_execution_time

      warnings
            Same as get_warnings

      warnings_count
            Same as get_warnings_count

FUNCTIONS
      fetch_all()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetch_one()
            Retrieves the next Row on the RowResult.

      fetch_one_object()
            Retrieves the next Row on the result and returns it as an object.

      get_affected_items_count()
            The the number of affected items for the last operation.

      get_column_count()
            Retrieves the number of columns on the current result.

      get_column_names()
            Gets the columns on the current result.

      get_columns()
            Gets the column metadata for the columns on the active result.

      get_execution_time()
            Retrieves a string value indicating the execution time of the
            executed operation.

      get_warnings()
            Retrieves the warnings generated by the executed operation.

      get_warnings_count()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

#@<OUT> rowresult.affected_items_count
NAME
      affected_items_count - Same as get_affected_items_count

SYNTAX
      <RowResult>.affected_items_count

#@<OUT> rowresult.column_count
NAME
      column_count - Same as get_column_count

SYNTAX
      <RowResult>.column_count

#@<OUT> rowresult.column_names
NAME
      column_names - Same as get_column_names

SYNTAX
      <RowResult>.column_names

#@<OUT> rowresult.columns
NAME
      columns - Same as get_columns

SYNTAX
      <RowResult>.columns

#@<OUT> rowresult.execution_time
NAME
      execution_time - Same as get_execution_time

SYNTAX
      <RowResult>.execution_time

#@<OUT> rowresult.fetch_all
NAME
      fetch_all - Returns a list of DbDoc objects which contains an element for
                  every unread document.

SYNTAX
      <RowResult>.fetch_all()

RETURNS
      A List of DbDoc objects.

#@<OUT> rowresult.fetch_one
NAME
      fetch_one - Retrieves the next Row on the RowResult.

SYNTAX
      <RowResult>.fetch_one()

RETURNS
      A Row object representing the next record on the result.

#@<OUT> rowresult.fetch_one_object
NAME
      fetch_one_object - Retrieves the next Row on the result and returns it as
                         an object.

SYNTAX
      <RowResult>.fetch_one_object()

RETURNS
      A dictionary containing the row information.

DESCRIPTION
      The column names will be used as keys in the returned dictionary and the
      column data will be used as the key values.

      If a column is a valid identifier it will be accessible as an object
      attribute as <dict>.<column>.

      If a column is not a valid identifier, it will be accessible as a
      dictionary key as <dict>[<column>].

#@<OUT> rowresult.get_affected_items_count
NAME
      get_affected_items_count - The the number of affected items for the last
                                 operation.

SYNTAX
      <RowResult>.get_affected_items_count()

RETURNS
      the number of affected items.

DESCRIPTION
      Returns the number of records affected by the executed operation.

#@<OUT> rowresult.get_column_count
NAME
      get_column_count - Retrieves the number of columns on the current result.

SYNTAX
      <RowResult>.get_column_count()

RETURNS
      the number of columns on the current result.

#@<OUT> rowresult.get_column_names
NAME
      get_column_names - Gets the columns on the current result.

SYNTAX
      <RowResult>.get_column_names()

RETURNS
      A list with the names of the columns returned on the active result.

#@<OUT> rowresult.get_columns
NAME
      get_columns - Gets the column metadata for the columns on the active
                    result.

SYNTAX
      <RowResult>.get_columns()

RETURNS
      a list of Column objects containing information about the columns
      included on the active result.

#@<OUT> rowresult.get_execution_time
NAME
      get_execution_time - Retrieves a string value indicating the execution
                           time of the executed operation.

SYNTAX
      <RowResult>.get_execution_time()

#@<OUT> rowresult.get_warnings
NAME
      get_warnings - Retrieves the warnings generated by the executed
                     operation.

SYNTAX
      <RowResult>.get_warnings()

RETURNS
      A list containing a warning object for each generated warning.

DESCRIPTION
      This is the same value than C API mysql_warning_count, see
      https://dev.mysql.com/doc/c-api/en/mysql-warning-count.html

      Each warning object contains a key/value pair describing the information
      related to a specific warning.

      This information includes: Level, Code and Message.

#@<OUT> rowresult.get_warnings_count
NAME
      get_warnings_count - The number of warnings produced by the last
                           statement execution.

SYNTAX
      <RowResult>.get_warnings_count()

RETURNS
      the number of warnings.

DESCRIPTION
      This is the same value than C API mysql_warning_count, see
      https://dev.mysql.com/doc/c-api/en/mysql-warning-count.html

      See get_warnings() for more details.

#@<OUT> rowresult.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <RowResult>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> rowresult.warnings
NAME
      warnings - Same as get_warnings

SYNTAX
      <RowResult>.warnings

#@<OUT> rowresult.warnings_count
NAME
      warnings_count - Same as get_warnings_count

SYNTAX
      <RowResult>.warnings_count


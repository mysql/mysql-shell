shell.connect(__uripwd + '/mysql')
sqlresult = session.sql('select 1;').execute()
session.close()

#@ sqlresult
sqlresult.help()

#@ global ? for SqlResult[USE:sqlresult]
\? SqlResult

#@ global help for SqlResult[USE:sqlresult]
\help SqlResult

#@ sqlresult.affected_items_count
sqlresult.help('affected_items_count')

#@ global ? for affected_items_count[USE:sqlresult.affected_items_count]
\? SqlResult.affected_items_count

#@ global help for affected_items_count[USE:sqlresult.affected_items_count]
\help SqlResult.affected_items_count

#@ sqlresult.auto_increment_value
sqlresult.help('auto_increment_value')

#@ global ? for auto_increment_value[USE:sqlresult.auto_increment_value]
\? SqlResult.auto_increment_value

#@ global help for auto_increment_value[USE:sqlresult.auto_increment_value]
\help SqlResult.auto_increment_value

#@ sqlresult.column_count
sqlresult.help('column_count')

#@ global ? for column_count[USE:sqlresult.column_count]
\? SqlResult.column_count

#@ global help for column_count[USE:sqlresult.column_count]
\help SqlResult.column_count

#@ sqlresult.column_names
sqlresult.help('column_names')

#@ global ? for column_names[USE:sqlresult.column_names]
\? SqlResult.column_names

#@ global help for column_names[USE:sqlresult.column_names]
\help SqlResult.column_names

#@ sqlresult.columns
sqlresult.help('columns')

#@ global ? for columns[USE:sqlresult.columns]
\? SqlResult.columns

#@ global help for columns[USE:sqlresult.columns]
\help SqlResult.columns

#@ sqlresult.execution_time
sqlresult.help('execution_time')

#@ global ? for execution_time[USE:sqlresult.execution_time]
\? SqlResult.execution_time

#@ global help for execution_time[USE:sqlresult.execution_time]
\help SqlResult.execution_time

#@ sqlresult.fetch_all
sqlresult.help('fetch_all')

#@ global ? for fetch_all[USE:sqlresult.fetch_all]
\? SqlResult.fetch_all

#@ global help for fetch_all[USE:sqlresult.fetch_all]
\help SqlResult.fetch_all

#@ sqlresult.fetch_one
sqlresult.help('fetch_one')

#@ global ? for fetch_one[USE:sqlresult.fetch_one]
\? SqlResult.fetch_one

#@ global help for fetch_one[USE:sqlresult.fetch_one]
\help SqlResult.fetch_one

#@ sqlresult.fetch_one_object
sqlresult.help('fetch_one_object')

#@ global ? for fetch_one_object[USE:sqlresult.fetch_one_object]
\? SqlResult.fetch_one_object

#@ global help for fetch_one_object[USE:sqlresult.fetch_one_object]
\help SqlResult.fetch_one_object

#@ sqlresult.get_affected_items_count
sqlresult.help('get_affected_items_count')

#@ global ? for get_affected_items_count[USE:sqlresult.get_affected_items_count]
\? SqlResult.get_affected_items_count

#@ global help for get_affected_items_count[USE:sqlresult.get_affected_items_count]
\help SqlResult.get_affected_items_count

#@ sqlresult.get_auto_increment_value
sqlresult.help('get_auto_increment_value')

#@ global ? for get_auto_increment_value[USE:sqlresult.get_auto_increment_value]
\? SqlResult.get_auto_increment_value

#@ global help for get_auto_increment_value[USE:sqlresult.get_auto_increment_value]
\help SqlResult.get_auto_increment_value

#@ sqlresult.get_column_count
sqlresult.help('get_column_count')

#@ global ? for get_column_count[USE:sqlresult.get_column_count]
\? SqlResult.get_column_count

#@ global help for get_column_count[USE:sqlresult.get_column_count]
\help SqlResult.get_column_count

#@ sqlresult.get_column_names
sqlresult.help('get_column_names')

#@ global ? for get_column_names[USE:sqlresult.get_column_names]
\? SqlResult.get_column_names

#@ global help for get_column_names[USE:sqlresult.get_column_names]
\help SqlResult.get_column_names

#@ sqlresult.get_columns
sqlresult.help('get_columns')

#@ global ? for get_columns[USE:sqlresult.get_columns]
\? SqlResult.get_columns

#@ global help for get_columns[USE:sqlresult.get_columns]
\help SqlResult.get_columns

#@ sqlresult.get_execution_time
sqlresult.help('get_execution_time')

#@ global ? for get_execution_time[USE:sqlresult.get_execution_time]
\? SqlResult.get_execution_time

#@ global help for get_execution_time[USE:sqlresult.get_execution_time]
\help SqlResult.get_execution_time

#@ sqlresult.get_warnings
sqlresult.help('get_warnings')

#@ global ? for get_warnings[USE:sqlresult.get_warnings]
\? SqlResult.get_warnings

#@ global help for get_warnings[USE:sqlresult.get_warnings]
\help SqlResult.get_warnings

#@ sqlresult.get_warnings_count
sqlresult.help('get_warnings_count')

#@ global ? for get_warnings_count[USE:sqlresult.get_warnings_count]
\? SqlResult.get_warnings_count

#@ global help for get_warnings_count[USE:sqlresult.get_warnings_count]
\help SqlResult.get_warnings_count

#@ sqlresult.has_data
sqlresult.help('has_data')

#@ global ? for has_data[USE:sqlresult.has_data]
\? SqlResult.has_data

#@ global help for has_data[USE:sqlresult.has_data]
\help SqlResult.has_data

#@ sqlresult.help
sqlresult.help('help')

#@ global ? for help[USE:sqlresult.help]
\? SqlResult.help

#@ global help for help[USE:sqlresult.help]
\help SqlResult.help

#@ sqlresult.next_result
sqlresult.help('next_result')

#@ global ? for next_result[USE:sqlresult.next_result]
\? SqlResult.next_result

#@ global help for next_result[USE:sqlresult.next_result]
\help SqlResult.next_result

#@ sqlresult.warnings
sqlresult.help('warnings')

#@ global ? for warnings[USE:sqlresult.warnings]
\? SqlResult.warnings

#@ global help for warnings[USE:sqlresult.warnings]
\help SqlResult.warnings

#@ sqlresult.warnings_count
sqlresult.help('warnings_count')

#@ global ? for warnings_count[USE:sqlresult.warnings_count]
\? SqlResult.warnings_count

#@ global help for warnings_count[USE:sqlresult.warnings_count]
\help SqlResult.warnings_count

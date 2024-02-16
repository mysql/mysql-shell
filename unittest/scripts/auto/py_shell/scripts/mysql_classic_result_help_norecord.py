shell.connect(__mysqluripwd)
classicresult = session.run_sql('select 1')
session.close()

#@ classicresult
classicresult.help()

#@ global ? for ClassicResult[USE:classicresult]
\? ClassicResult

#@ global help for ClassicResult[USE:classicresult]
\help ClassicResult

#@ classicresult.affected_items_count
classicresult.help('affected_items_count')

#@ global ? for affected_items_count[USE:classicresult.affected_items_count]
\? ClassicResult.affected_items_count

#@ global help for affected_items_count[USE:classicresult.affected_items_count]
\help ClassicResult.affected_items_count

#@ classicresult.auto_increment_value
classicresult.help('auto_increment_value')

#@ global ? for auto_increment_value[USE:classicresult.auto_increment_value]
\? ClassicResult.auto_increment_value

#@ global help for auto_increment_value[USE:classicresult.auto_increment_value]
\help ClassicResult.auto_increment_value

#@ classicresult.column_count
classicresult.help('column_count')

#@ global ? for column_count[USE:classicresult.column_count]
\? ClassicResult.column_count

#@ global help for column_count[USE:classicresult.column_count]
\help ClassicResult.column_count

#@ classicresult.column_names
classicresult.help('column_names')

#@ global ? for column_names[USE:classicresult.column_names]
\? ClassicResult.column_names

#@ global help for column_names[USE:classicresult.column_names]
\help ClassicResult.column_names

#@ classicresult.columns
classicresult.help('columns')

#@ global ? for columns[USE:classicresult.columns]
\? ClassicResult.columns

#@ global help for columns[USE:classicresult.columns]
\help ClassicResult.columns

#@ classicresult.execution_time
classicresult.help('execution_time')

#@ global ? for execution_time[USE:classicresult.execution_time]
\? ClassicResult.execution_time

#@ global help for execution_time[USE:classicresult.execution_time]
\help ClassicResult.execution_time

#@ classicresult.fetch_all
classicresult.help('fetch_all')

#@ global ? for fetch_all[USE:classicresult.fetch_all]
\? ClassicResult.fetch_all

#@ global help for fetch_all[USE:classicresult.fetch_all]
\help ClassicResult.fetch_all

#@ classicresult.fetch_one
classicresult.help('fetch_one')

#@ global ? for fetch_one[USE:classicresult.fetch_one]
\? ClassicResult.fetch_one

#@ global help for fetch_one[USE:classicresult.fetch_one]
\help ClassicResult.fetch_one

#@ classicresult.fetch_one_object
classicresult.help('fetch_one_object')

#@ global ? for fetch_one_object[USE:classicresult.fetch_one_object]
\? ClassicResult.fetch_one_object

#@ global help for fetch_one_object[USE:classicresult.fetch_one_object]
\help ClassicResult.fetch_one_object

#@ classicresult.get_affected_items_count
classicresult.help('get_affected_items_count')

#@ global ? for get_affected_items_count[USE:classicresult.get_affected_items_count]
\? ClassicResult.get_affected_items_count

#@ global help for get_affected_items_count[USE:classicresult.get_affected_items_count]
\help ClassicResult.get_affected_items_count

#@ classicresult.get_auto_increment_value
classicresult.help('get_auto_increment_value')

#@ global ? for get_auto_increment_value[USE:classicresult.get_auto_increment_value]
\? ClassicResult.get_auto_increment_value

#@ global help for get_auto_increment_value[USE:classicresult.get_auto_increment_value]
\help ClassicResult.get_auto_increment_value

#@ classicresult.get_column_count
classicresult.help('get_column_count')

#@ global ? for get_column_count[USE:classicresult.get_column_count]
\? ClassicResult.get_column_count

#@ global help for get_column_count[USE:classicresult.get_column_count]
\help ClassicResult.get_column_count

#@ classicresult.get_column_names
classicresult.help('get_column_names')

#@ global ? for get_column_names[USE:classicresult.get_column_names]
\? ClassicResult.get_column_names

#@ global help for get_column_names[USE:classicresult.get_column_names]
\help ClassicResult.get_column_names

#@ classicresult.get_columns
classicresult.help('get_columns')

#@ global ? for get_columns[USE:classicresult.get_columns]
\? ClassicResult.get_columns

#@ global help for get_columns[USE:classicresult.get_columns]
\help ClassicResult.get_columns

#@ classicresult.get_execution_time
classicresult.help('get_execution_time')

#@ global ? for get_execution_time[USE:classicresult.get_execution_time]
\? ClassicResult.get_execution_time

#@ global help for get_execution_time[USE:classicresult.get_execution_time]
\help ClassicResult.get_execution_time

#@ classicresult.get_info
classicresult.help('get_info')

#@ global ? for get_info[USE:classicresult.get_info]
\? ClassicResult.get_info

#@ global help for get_info[USE:classicresult.get_info]
\help ClassicResult.get_info

#@ classicresult.get_warnings_count
classicresult.help('get_warnings_count')

#@ global ? for get_warnings_count[USE:classicresult.get_warnings_count]
\? ClassicResult.get_warnings_count

#@ global help for get_warnings_count[USE:classicresult.get_warnings_count]
\help ClassicResult.get_warnings_count

#@ classicresult.get_warnings
classicresult.help('get_warnings')

#@ global ? for get_warnings[USE:classicresult.get_warnings]
\? ClassicResult.get_warnings

#@ global help for get_warnings[USE:classicresult.get_warnings]
\help ClassicResult.get_warnings

#@ classicresult.has_data
classicresult.help('has_data')

#@ global ? for has_data[USE:classicresult.has_data]
\? ClassicResult.has_data

#@ global help for has_data[USE:classicresult.has_data]
\help ClassicResult.has_data

#@ classicresult.help
classicresult.help('help')

#@ global ? for help[USE:classicresult.help]
\? ClassicResult.help

#@ global help for help[USE:classicresult.help]
\help ClassicResult.help

#@ classicresult.info
classicresult.help('info')

#@ global ? for info[USE:classicresult.info]
\? ClassicResult.info

#@ global help for info[USE:classicresult.info]
\help ClassicResult.info

#@ classicresult.next_result
classicresult.help('next_result')

#@ global ? for next_result[USE:classicresult.next_result]
\? ClassicResult.next_result

#@ global help for next_result[USE:classicresult.next_result]
\help ClassicResult.next_result

#@ classicresult.warnings_count
classicresult.help('warnings_count')

#@ global ? for warnings_count[USE:classicresult.warnings_count]
\? ClassicResult.warnings_count

#@ global help for warnings_count[USE:classicresult.warnings_count]
\help ClassicResult.warnings_count

#@ classicresult.warnings
classicresult.help('warnings')

#@ global ? for warnings[USE:classicresult.warnings]
\? ClassicResult.warnings

#@ global help for warnings[USE:classicresult.warnings]
\help ClassicResult.warnings

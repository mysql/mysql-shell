shell.connect(__uripwd + '/mysql')
rowresult = db.user.select().execute()
session.close()

#@ rowresult
rowresult.help()

#@ global ? for RowResult[USE:rowresult]
\? RowResult

#@ global help for RowResult[USE:rowresult]
\help RowResult

#@ rowresult.affected_items_count
rowresult.help('affected_items_count')

#@ global ? for affected_items_count[USE:rowresult.affected_items_count]
\? RowResult.affected_items_count

#@ global help for affected_items_count[USE:rowresult.affected_items_count]
\help RowResult.affected_items_count

#@ rowresult.column_count
rowresult.help('column_count')

#@ global ? for column_count[USE:rowresult.column_count]
\? RowResult.column_count

#@ global help for column_count[USE:rowresult.column_count]
\help RowResult.column_count

#@ rowresult.column_names
rowresult.help('column_names')

#@ global ? for column_names[USE:rowresult.column_names]
\? RowResult.column_names

#@ global help for column_names[USE:rowresult.column_names]
\help RowResult.column_names

#@ rowresult.columns
rowresult.help('columns')

#@ global ? for columns[USE:rowresult.columns]
\? RowResult.columns

#@ global help for columns[USE:rowresult.columns]
\help RowResult.columns

#@ rowresult.execution_time
rowresult.help('execution_time')

#@ global ? for execution_time[USE:rowresult.execution_time]
\? RowResult.execution_time

#@ global help for execution_time[USE:rowresult.execution_time]
\help RowResult.execution_time

#@ rowresult.fetch_all
rowresult.help('fetch_all')

#@ global ? for fetch_all[USE:rowresult.fetch_all]
\? RowResult.fetch_all

#@ global help for fetch_all[USE:rowresult.fetch_all]
\help RowResult.fetch_all

#@ rowresult.fetch_one
rowresult.help('fetch_one')

#@ global ? for fetch_one[USE:rowresult.fetch_one]
\? RowResult.fetch_one

#@ global help for fetch_one[USE:rowresult.fetch_one]
\help RowResult.fetch_one

#@ rowresult.get_affected_items_count
rowresult.help('get_affected_items_count')

#@ global ? for get_affected_items_count[USE:rowresult.get_affected_items_count]
\? RowResult.get_affected_items_count

#@ global help for get_affected_items_count[USE:rowresult.get_affected_items_count]
\help RowResult.get_affected_items_count

#@ rowresult.get_column_count
rowresult.help('get_column_count')

#@ global ? for get_column_count[USE:rowresult.get_column_count]
\? RowResult.get_column_count

#@ global help for get_column_count[USE:rowresult.get_column_count]
\help RowResult.get_column_count

#@ rowresult.get_column_names
rowresult.help('get_column_names')

#@ global ? for get_column_names[USE:rowresult.get_column_names]
\? RowResult.get_column_names

#@ global help for get_column_names[USE:rowresult.get_column_names]
\help RowResult.get_column_names

#@ rowresult.get_columns
rowresult.help('get_columns')

#@ global ? for get_columns[USE:rowresult.get_columns]
\? RowResult.get_columns

#@ global help for get_columns[USE:rowresult.get_columns]
\help RowResult.get_columns

#@ rowresult.get_execution_time
rowresult.help('get_execution_time')

#@ global ? for get_execution_time[USE:rowresult.get_execution_time]
\? RowResult.get_execution_time

#@ global help for get_execution_time[USE:rowresult.get_execution_time]
\help RowResult.get_execution_time

#@ rowresult.get_warning_count
rowresult.help('get_warning_count')

#@ global ? for get_warning_count[USE:rowresult.get_warning_count]
\? RowResult.get_warning_count

#@ global help for get_warning_count[USE:rowresult.get_warning_count]
\help RowResult.get_warning_count

#@ rowresult.get_warnings
rowresult.help('get_warnings')

#@ global ? for get_warnings[USE:rowresult.get_warnings]
\? RowResult.get_warnings

#@ global help for get_warnings[USE:rowresult.get_warnings]
\help RowResult.get_warnings

#@ rowresult.get_warnings_count
rowresult.help('get_warnings_count')

#@ global ? for get_warnings_count[USE:rowresult.get_warnings_count]
\? RowResult.get_warnings_count

#@ global help for get_warnings_count[USE:rowresult.get_warnings_count]
\help RowResult.get_warnings_count

#@ rowresult.help
rowresult.help('help')

#@ global ? for help[USE:rowresult.help]
\? RowResult.help

#@ global help for help[USE:rowresult.help]
\help RowResult.help

#@ rowresult.warning_count
rowresult.help('warning_count')

#@ global ? for warning_count[USE:rowresult.warning_count]
\? RowResult.warning_count

#@ global help for warning_count[USE:rowresult.warning_count]
\help RowResult.warning_count

#@ rowresult.warnings
rowresult.help('warnings')

#@ global ? for warnings[USE:rowresult.warnings]
\? RowResult.warnings

#@ global help for warnings[USE:rowresult.warnings]
\help RowResult.warnings

#@ rowresult.warnings_count
rowresult.help('warnings_count')

#@ global ? for warnings_count[USE:rowresult.warnings_count]
\? RowResult.warnings_count

#@ global help for warnings_count[USE:rowresult.warnings_count]
\help RowResult.warnings_count

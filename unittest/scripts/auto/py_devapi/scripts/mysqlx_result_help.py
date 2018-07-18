shell.connect(__uripwd)
schema = session.create_schema('result_help')
coll = schema.create_collection('sample')
result = coll.add({'_id':'sample', 'name':'test'}).execute();
session.close()

#@ result
result.help()

#@ global ? for Result[USE:result]
\? Result

#@ global help for Result[USE:result]
\help Result

#@ result.affected_item_count
result.help('affected_item_count')

#@ global ? for affected_item_count[USE:result.affected_item_count]
\? Result.affected_item_count

#@ global help for affected_item_count[USE:result.affected_item_count]
\help Result.affected_item_count

#@ result.affected_items_count
result.help('affected_items_count')

#@ global ? for affected_items_count[USE:result.affected_items_count]
\? Result.affected_items_count

#@ global help for affected_items_count[USE:result.affected_items_count]
\help Result.affected_items_count

#@ result.auto_increment_value
result.help('auto_increment_value')

#@ global ? for auto_increment_value[USE:result.auto_increment_value]
\? Result.auto_increment_value

#@ global help for auto_increment_value[USE:result.auto_increment_value]
\help Result.auto_increment_value

#@ result.execution_time
result.help('execution_time')

#@ global ? for execution_time[USE:result.execution_time]
\? Result.execution_time

#@ global help for execution_time[USE:result.execution_time]
\help Result.execution_time

#@ result.generated_ids
result.help('generated_ids')

#@ global ? for generated_ids[USE:result.generated_ids]
\? Result.generated_ids

#@ global help for generated_ids[USE:result.generated_ids]
\help Result.generated_ids

#@ result.get_affected_item_count
result.help('get_affected_item_count')

#@ global ? for get_affected_item_count[USE:result.get_affected_item_count]
\? Result.get_affected_item_count

#@ global help for get_affected_item_count[USE:result.get_affected_item_count]
\help Result.get_affected_item_count

#@ result.get_affected_items_count
result.help('get_affected_items_count')

#@ global ? for get_affected_items_count[USE:result.get_affected_items_count]
\? Result.get_affected_items_count

#@ global help for get_affected_items_count[USE:result.get_affected_items_count]
\help Result.get_affected_items_count

#@ result.get_auto_increment_value
result.help('get_auto_increment_value')

#@ global ? for get_auto_increment_value[USE:result.get_auto_increment_value]
\? Result.get_auto_increment_value

#@ global help for get_auto_increment_value[USE:result.get_auto_increment_value]
\help Result.get_auto_increment_value

#@ result.get_execution_time
result.help('get_execution_time')

#@ global ? for get_execution_time[USE:result.get_execution_time]
\? Result.get_execution_time

#@ global help for get_execution_time[USE:result.get_execution_time]
\help Result.get_execution_time

#@ result.get_generated_ids
result.help('get_generated_ids')

#@ global ? for get_generated_ids[USE:result.get_generated_ids]
\? Result.get_generated_ids

#@ global help for get_generated_ids[USE:result.get_generated_ids]
\help Result.get_generated_ids

#@ result.get_warning_count
result.help('get_warning_count')

#@ global ? for get_warning_count[USE:result.get_warning_count]
\? Result.get_warning_count

#@ global help for get_warning_count[USE:result.get_warning_count]
\help Result.get_warning_count

#@ result.get_warnings
result.help('get_warnings')

#@ global ? for get_warnings[USE:result.get_warnings]
\? Result.get_warnings

#@ global help for get_warnings[USE:result.get_warnings]
\help Result.get_warnings

#@ result.get_warnings_count
result.help('get_warnings_count')

#@ global ? for get_warnings_count[USE:result.get_warnings_count]
\? Result.get_warnings_count

#@ global help for get_warnings_count[USE:result.get_warnings_count]
\help Result.get_warnings_count

#@ result.help
result.help('help')

#@ global ? for help[USE:result.help]
\? Result.help

#@ global help for help[USE:result.help]
\help Result.help

#@ result.warning_count
result.help('warning_count')

#@ global ? for warning_count[USE:result.warning_count]
\? Result.warning_count

#@ global help for warning_count[USE:result.warning_count]
\help Result.warning_count

#@ result.warnings
result.help('warnings')

#@ global ? for warnings[USE:result.warnings]
\? Result.warnings

#@ global help for warnings[USE:result.warnings]
\help Result.warnings

#@ result.warnings_count
result.help('warnings_count')

#@ global ? for warnings_count[USE:result.warnings_count]
\? Result.warnings_count

#@ global help for warnings_count[USE:result.warnings_count]
\help Result.warnings_count

shell.connect(__uripwd)
session.drop_schema('result_help');
session.close()

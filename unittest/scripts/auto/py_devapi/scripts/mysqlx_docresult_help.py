shell.connect(__uripwd)
schema = session.create_schema('docresult_help')
coll = schema.create_collection('sample')
docresult = coll.find().execute();
session.close()

#@ docresult
docresult.help()

#@ global ? for DocResult[USE:docresult]
\? DocResult

#@ global help for DocResult[USE:docresult]
\help DocResult

#@ docresult.execution_time
docresult.help('execution_time')

#@ global ? for execution_time[USE:docresult.execution_time]
\? DocResult.execution_time

#@ global help for execution_time[USE:docresult.execution_time]
\help DocResult.execution_time

#@ docresult.fetch_all
docresult.help('fetch_all')

#@ global ? for fetch_all[USE:docresult.fetch_all]
\? DocResult.fetch_all

#@ global help for fetch_all[USE:docresult.fetch_all]
\help DocResult.fetch_all

#@ docresult.fetch_one
docresult.help('fetch_one')

#@ global ? for fetch_one[USE:docresult.fetch_one]
\? DocResult.fetch_one

#@ global help for fetch_one[USE:docresult.fetch_one]
\help DocResult.fetch_one

#@ docresult.get_execution_time
docresult.help('get_execution_time')

#@ global ? for get_execution_time[USE:docresult.get_execution_time]
\? DocResult.get_execution_time

#@ global help for get_execution_time[USE:docresult.get_execution_time]
\help DocResult.get_execution_time

#@ docresult.get_warning_count
docresult.help('get_warning_count')

#@ global ? for get_warning_count[USE:docresult.get_warning_count]
\? DocResult.get_warning_count

#@ global help for get_warning_count[USE:docresult.get_warning_count]
\help DocResult.get_warning_count

#@ docresult.get_warnings
docresult.help('get_warnings')

#@ global ? for get_warnings[USE:docresult.get_warnings]
\? DocResult.get_warnings

#@ global help for get_warnings[USE:docresult.get_warnings]
\help DocResult.get_warnings

#@ docresult.help
docresult.help('help')

#@ global ? for help[USE:docresult.help]
\? DocResult.help

#@ global help for help[USE:docresult.help]
\help DocResult.help

#@ docresult.warning_count
docresult.help('warning_count')

#@ global ? for warning_count[USE:docresult.warning_count]
\? DocResult.warning_count

#@ global help for warning_count[USE:docresult.warning_count]
\help DocResult.warning_count

#@ docresult.warnings
docresult.help('warnings')

#@ global ? for warnings[USE:docresult.warnings]
\? DocResult.warnings

#@ global help for warnings[USE:docresult.warnings]
\help DocResult.warnings

shell.connect(__uripwd)
session.drop_schema('docresult_help');
session.close()

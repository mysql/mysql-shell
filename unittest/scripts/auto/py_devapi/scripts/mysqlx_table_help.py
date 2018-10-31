shell.connect(__uripwd + '/mysql')
table = db.user
session.close()

#@ table
table.help()

#@ global ? for Table[USE:table]
\? Table

#@ global help for Table[USE:table]
\help Table

#@ table.count
table.help('count')

#@ global help for count[USE:table.count]
\help Table.count

#@ table.delete
table.help('delete')

#@ global ? for delete[USE:table.delete]
\? Table.delete

#@ global help for delete[USE:table.delete]
\help Table.delete

#@ table.exists_in_database
table.help('exists_in_database')

#@ global ? for exists_in_database[USE:table.exists_in_database]
\? Table.exists_in_database

#@ global help for exists_in_database[USE:table.exists_in_database]
\help Table.exists_in_database

#@ table.get_name
table.help('get_name')

#@ global ? for get_name[USE:table.get_name]
\? Table.get_name

#@ global help for get_name[USE:table.get_name]
\help Table.get_name

#@ table.get_schema
table.help('get_schema')

#@ global ? for get_schema[USE:table.get_schema]
\? Table.get_schema

#@ global help for get_schema[USE:table.get_schema]
\help Table.get_schema

#@ table.get_session
table.help('get_session')

#@ global ? for get_session[USE:table.get_session]
\? Table.get_session

#@ global help for get_session[USE:table.get_session]
\help Table.get_session

#@ table.help
table.help('help')

#@ global ? for help[USE:table.help]
\? Table.help

#@ global help for help[USE:table.help]
\help Table.help

#@ table.insert
table.help('insert')

#@ global ? for insert[USE:table.insert]
\? Table.insert

#@ global help for insert[USE:table.insert]
\help Table.insert

#@ table.is_view
table.help('is_view')

#@ global ? for is_view[USE:table.is_view]
\? Table.is_view

#@ global help for is_view[USE:table.is_view]
\help Table.is_view

#@ table.name
table.help('name')

#@ global ? for name[USE:table.name]
\? Table.name

#@ global help for name[USE:table.name]
\help Table.name

#@ table.schema
table.help('schema')

#@ global ? for schema[USE:table.schema]
\? Table.schema

#@ global help for schema[USE:table.schema]
\help Table.schema

#@ table.select
table.help('select')

#@ global ? for select[USE:table.select]
\? Table.select

#@ global help for select[USE:table.select]
\help Table.select

#@ table.session
table.help('session')

#@ global ? for session[USE:table.session]
\? Table.session

#@ global help for session[USE:table.session]
\help Table.session

#@ table.update
table.help('update')

#@ global ? for update[USE:table.update]
\? Table.update

#@ global help for update[USE:table.update]
\help Table.update

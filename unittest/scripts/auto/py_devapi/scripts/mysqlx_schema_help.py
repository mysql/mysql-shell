shell.connect(__uripwd + '/mysql')
schema = db
session.close()

#@ schema
schema.help()

#@ global ? for Schema[USE:schema]
\? mysqlx.Schema

#@ global help for Schema[USE:schema]
\help mysqlx.Schema

#@ schema.create_collection
schema.help('create_collection')

#@ global ? for create_collection[USE:schema.create_collection]
\? Schema.create_collection

#@ global help for create_collection[USE:schema.create_collection]
\help Schema.create_collection

#@ schema.drop_collection
schema.help('drop_collection')

#@ global ? for drop_collection[USE:schema.drop_collection]
\? Schema.drop_collection

#@ global help for drop_collection[USE:schema.drop_collection]
\help Schema.drop_collection

#@ schema.modify_collection
schema.help('modify_collection')

#@ global ? for modify_collection[USE:schema.modify_collection]
\? Schema.modify_collection

#@ global help for modify_collection[USE:schema.modify_collection]
\help Schema.modify_collection

#@ schema.exists_in_database
schema.help('exists_in_database')

#@ global ? for exists_in_database[USE:schema.exists_in_database]
\? Schema.exists_in_database

#@ global help for exists_in_database[USE:schema.exists_in_database]
\help Schema.exists_in_database

#@ schema.get_collection
schema.help('get_collection')

#@ global ? for get_collection[USE:schema.get_collection]
\? Schema.get_collection

#@ global help for get_collection[USE:schema.get_collection]
\help Schema.get_collection

#@ schema.get_collection_as_table
schema.help('get_collection_as_table')

#@ global ? for get_collection_as_table[USE:schema.get_collection_as_table]
\? Schema.get_collection_as_table

#@ global help for get_collection_as_table[USE:schema.get_collection_as_table]
\help Schema.get_collection_as_table

#@ schema.get_collections
schema.help('get_collections')

#@ global ? for get_collections[USE:schema.get_collections]
\? Schema.get_collections

#@ global help for get_collections[USE:schema.get_collections]
\help Schema.get_collections

#@ schema.get_name
schema.help('get_name')

#@ global ? for get_name[USE:schema.get_name]
\? Schema.get_name

#@ global help for get_name[USE:schema.get_name]
\help Schema.get_name

#@ schema.get_schema
schema.help('get_schema')

#@ global ? for get_schema[USE:schema.get_schema]
\? Schema.get_schema

#@ global help for get_schema[USE:schema.get_schema]
\help Schema.get_schema

#@ schema.get_session
schema.help('get_session')

#@ global ? for get_session[USE:schema.get_session]
\? Schema.get_session

#@ global help for get_session[USE:schema.get_session]
\help Schema.get_session

#@ schema.get_table
schema.help('get_table')

#@ global ? for get_table[USE:schema.get_table]
\? Schema.get_table

#@ global help for get_table[USE:schema.get_table]
\help Schema.get_table

#@ schema.get_tables
schema.help('get_tables')

#@ global ? for get_tables[USE:schema.get_tables]
\? Schema.get_tables

#@ global help for get_tables[USE:schema.get_tables]
\help Schema.get_tables

#@ schema.help
schema.help('help')

#@ global ? for help[USE:schema.help]
\? Schema.help

#@ global help for help[USE:schema.help]
\help Schema.help

#@ schema.name
schema.help('name')

#@ global ? for name[USE:schema.name]
\? Schema.name

#@ global help for name[USE:schema.name]
\help Schema.name

#@ schema.schema
schema.help('schema')

#@ global ? for schema[USE:schema.schema]
\? Schema.schema

#@ global help for schema[USE:schema.schema]
\help Schema.schema

#@ schema.session
schema.help('session')

#@ global ? for session[USE:schema.session]
\? Schema.session

#@ global help for session[USE:schema.session]
\help Schema.session

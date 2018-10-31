shell.connect(__uripwd)
schema = session.create_schema('collection_help')
coll = schema.create_collection('sample')
session.close()

#@ coll
coll.help()

#@ global ? for Collection[USE:coll]
\? Collection

#@ global help for Collection[USE:coll]
\help Collection

#@ coll.add
coll.help('add')

#@ global ? for add[USE:coll.add]
\? Collection.add

#@ global help for add[USE:coll.add]
\help Collection.add

#@ coll.add_or_replace_one
coll.help('add_or_replace_one')

#@ global ? for add_or_replace_one[USE:coll.add_or_replace_one]
\? Collection.add_or_replace_one

#@ global help for add_or_replace_one[USE:coll.add_or_replace_one]
\help Collection.add_or_replace_one

#@coll.count
coll.help('count')

#@ global ? for count[USE:coll.count]
\? Collection.count

#@ global help for count[USE:coll.count]
\help Collection.count

#@ coll.create_index
coll.help('create_index')

#@ global ? for create_index[USE:coll.create_index]
\? Collection.create_index

#@ global help for create_index[USE:coll.create_index]
\help Collection.create_index

#@ coll.drop_index
coll.help('drop_index')

#@ global ? for drop_index[USE:coll.drop_index]
\? Collection.drop_index

#@ global help for drop_index[USE:coll.drop_index]
\help Collection.drop_index

#@ coll.exists_in_database
coll.help('exists_in_database')

#@ global ? for exists_in_database[USE:coll.exists_in_database]
\? Collection.exists_in_database

#@ global help for exists_in_database[USE:coll.exists_in_database]
\help Collection.exists_in_database

#@ coll.find
coll.help('find')

#@ global ? for find[USE:coll.find]
\? Collection.find

#@ global help for find[USE:coll.find]
\help Collection.find

#@ coll.get_name
coll.help('get_name')

#@ global ? for get_name[USE:coll.get_name]
\? Collection.get_name

#@ global help for get_name[USE:coll.get_name]
\help Collection.get_name

#@ coll.get_one
coll.help('get_one')

#@ global ? for get_one[USE:coll.get_one]
\? Collection.get_one

#@ global help for get_one[USE:coll.get_one]
\help Collection.get_one

#@ coll.get_schema
coll.help('get_schema')

#@ global ? for get_schema[USE:coll.get_schema]
\? Collection.get_schema

#@ global help for get_schema[USE:coll.get_schema]
\help Collection.get_schema

#@ coll.get_session
coll.help('get_session')

#@ global ? for get_session[USE:coll.get_session]
\? Collection.get_session

#@ global help for get_session[USE:coll.get_session]
\help Collection.get_session

#@ coll.help
coll.help('help')

#@ global ? for help[USE:coll.help]
\? Collection.help

#@ global help for help[USE:coll.help]
\help Collection.help

#@ coll.modify
coll.help('modify')

#@ global ? for modify[USE:coll.modify]
\? Collection.modify

#@ global help for modify[USE:coll.modify]
\help Collection.modify

#@ coll.name
coll.help('name')

#@ global ? for name[USE:coll.name]
\? Collection.name

#@ global help for name[USE:coll.name]
\help Collection.name

#@ coll.remove
coll.help('remove')

#@ global ? for remove[USE:coll.remove]
\? Collection.remove

#@ global help for remove[USE:coll.remove]
\help Collection.remove

#@ coll.remove_one
coll.help('remove_one')

#@ global ? for remove_one[USE:coll.remove_one]
\? Collection.remove_one

#@ global help for remove_one[USE:coll.remove_one]
\help Collection.remove_one

#@ coll.replace_one
coll.help('replace_one')

#@ global ? for replace_one[USE:coll.replace_one]
\? Collection.replace_one

#@ global help for replace_one[USE:coll.replace_one]
\help Collection.replace_one

#@ coll.schema
coll.help('schema')

#@ global ? for schema[USE:coll.schema]
\? Collection.schema

#@ global help for schema[USE:coll.schema]
\help Collection.schema

#@ coll.session
coll.help('session')

#@ global ? for session[USE:coll.session]
\? Collection.session

#@ global help for session[USE:coll.session]
\help Collection.session

shell.connect(__uripwd)
schema = session.drop_schema('collection_help')
session.close()

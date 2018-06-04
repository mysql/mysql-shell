shell.connect(__uripwd)
schema = session.create_schema('collectionfind_help')
coll = schema.create_collection('sample')
collfind = coll.find();
session.close()

#@ collfind
collfind.help()

#@ global ? for CollectionFind[USE:collfind]
\? CollectionFind

#@ global help for CollectionFind[USE:collfind]
\help CollectionFind

#@ collfind.bind
collfind.help('bind')

#@ global ? for bind[USE:collfind.bind]
\? CollectionFind.bind

#@ global help for bind[USE:collfind.bind]
\help CollectionFind.bind

#@ collfind.execute
collfind.help('execute')

#@ global ? for execute[USE:collfind.execute]
\? CollectionFind.execute

#@ global help for execute[USE:collfind.execute]
\help CollectionFind.execute

#@ collfind.fields
collfind.help('fields')

#@ global ? for fields[USE:collfind.fields]
\? CollectionFind.fields

#@ global help for fields[USE:collfind.fields]
\help CollectionFind.fields

#@ collfind.find
collfind.help('find')

#@ global ? for find[USE:collfind.find]
\? CollectionFind.find

#@ global help for find[USE:collfind.find]
\help CollectionFind.find

#@ collfind.group_by
collfind.help('group_by')

#@ global ? for group_by[USE:collfind.group_by]
\? CollectionFind.group_by

#@ global help for group_by[USE:collfind.group_by]
\help CollectionFind.group_by

#@ collfind.help
collfind.help('help')

#@ global ? for help[USE:collfind.help]
\? CollectionFind.help

#@ global help for help[USE:collfind.help]
\help CollectionFind.help

#@ collfind.limit
collfind.help('limit')

#@ global ? for limit[USE:collfind.limit]
\? CollectionFind.limit

#@ global help for limit[USE:collfind.limit]
\help CollectionFind.limit

#@ collfind.lock_exclusive
collfind.help('lock_exclusive')

#@ global ? for lock_exclusive[USE:collfind.lock_exclusive]
\? CollectionFind.lock_exclusive

#@ global help for lock_exclusive[USE:collfind.lock_exclusive]
\help CollectionFind.lock_exclusive

#@ collfind.lock_shared
collfind.help('lock_shared')

#@ global ? for lock_shared[USE:collfind.lock_shared]
\? CollectionFind.lock_shared

#@ global help for lock_shared[USE:collfind.lock_shared]
\help CollectionFind.lock_shared

#@ collfind.sort
collfind.help('sort')

#@ global ? for sort[USE:collfind.sort]
\? CollectionFind.sort

#@ global help for sort[USE:collfind.sort]
\help CollectionFind.sort

shell.connect(__uripwd)
session.drop_schema('collectionfind_help');
session.close()

shell.connect(__uripwd)
schema = session.create_schema('collectionremove_help')
coll = schema.create_collection('sample')
collremove = coll.remove('1=1');
session.close()

#@ collremove
collremove.help()

#@ global ? for CollectionRemove[USE:collremove]
\? CollectionRemove

#@ global help for CollectionRemove[USE:collremove]
\help CollectionRemove

#@ collremove.bind
collremove.help('bind')

#@ global ? for bind[USE:collremove.bind]
\? CollectionRemove.bind

#@ global help for bind[USE:collremove.bind]
\help CollectionRemove.bind

#@ collremove.execute
collremove.help('execute')

#@ global ? for execute[USE:collremove.execute]
\? CollectionRemove.execute

#@ global help for execute[USE:collremove.execute]
\help CollectionRemove.execute

#@ collremove.help
collremove.help('help')

#@ global ? for help[USE:collremove.help]
\? CollectionRemove.help

#@ global help for help[USE:collremove.help]
\help CollectionRemove.help

#@ collremove.limit
collremove.help('limit')

#@ global ? for limit[USE:collremove.limit]
\? CollectionRemove.limit

#@ global help for limit[USE:collremove.limit]
\help CollectionRemove.limit

#@ collremove.remove
collremove.help('remove')

#@ global ? for remove[USE:collremove.remove]
\? CollectionRemove.remove

#@ global help for remove[USE:collremove.remove]
\help CollectionRemove.remove

#@ collremove.sort
collremove.help('sort')

#@ global ? for sort[USE:collremove.sort]
\? CollectionRemove.sort

#@ global help for sort[USE:collremove.sort]
\help CollectionRemove.sort

shell.connect(__uripwd)
session.drop_schema('collectionremove_help');
session.close()

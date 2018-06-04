shell.connect(__uripwd)
schema = session.create_schema('collectionadd_help')
coll = schema.create_collection('sample')
colladd = coll.add({'name':'test'});
session.close()

#@ colladd
colladd.help()

#@ global ? for CollectionAdd[USE:colladd]
\? CollectionAdd

#@ global help for CollectionAdd[USE:colladd]
\help CollectionAdd

#@ colladd.add
colladd.help('add')

#@ global ? for add[USE:colladd.add]
\? CollectionAdd.add

#@ global help for add[USE:colladd.add]
\help CollectionAdd.add

#@ colladd.execute
colladd.help('execute')

#@ global ? for execute[USE:colladd.execute]
\? CollectionAdd.execute

#@ global help for execute[USE:colladd.execute]
\help CollectionAdd.execute

#@ colladd.help
colladd.help('help')

#@ global ? for help[USE:colladd.help]
\? CollectionAdd.help

#@ global help for help[USE:colladd.help]
\help CollectionAdd.help


shell.connect(__uripwd)
session.drop_schema('collectionadd_help');
session.close()

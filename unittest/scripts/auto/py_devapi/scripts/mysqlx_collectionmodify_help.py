shell.connect(__uripwd)
schema = session.create_schema('collectionmodify_help')
coll = schema.create_collection('sample')
collmodify = coll.modify('1=1');
session.close()

#@ collmodify
collmodify.help()

#@ global ? for CollectionModify[USE:collmodify]
\? CollectionModify

#@ global help for CollectionModify[USE:collmodify]
\help CollectionModify

#@ collfind.array_append
collmodify.help('array_append')

#@ global ? for array_append[USE:collfind.array_append]
\? CollectionModify.array_append

#@ global help for array_append[USE:collfind.array_append]
\help CollectionModify.array_append

#@ collfind.array_delete
collmodify.help('array_delete')

#@ global ? for array_delete[USE:collfind.array_delete]
\? CollectionModify.array_delete

#@ global help for array_delete[USE:collfind.array_delete]
\help CollectionModify.array_delete

#@ collfind.array_insert
collmodify.help('array_insert')

#@ global ? for array_insert[USE:collfind.array_insert]
\? CollectionModify.array_insert

#@ global help for array_insert[USE:collfind.array_insert]
\help CollectionModify.array_insert

#@ collfind.help
collmodify.help('help')

#@ global ? for help[USE:collfind.help]
\? CollectionModify.help

#@ global help for help[USE:collfind.help]
\help CollectionModify.help

#@ collfind.merge
collmodify.help('merge')

#@ global ? for merge[USE:collfind.merge]
\? CollectionModify.merge

#@ global help for merge[USE:collfind.merge]
\help CollectionModify.merge

#@ collfind.modify
collmodify.help('modify')

#@ global ? for modify[USE:collfind.modify]
\? CollectionModify.modify

#@ global help for modify[USE:collfind.modify]
\help CollectionModify.modify

#@ collfind.patch
collmodify.help('patch')

#@ global ? for patch[USE:collfind.patch]
\? CollectionModify.patch

#@ global help for patch[USE:collfind.patch]
\help CollectionModify.patch

#@ collfind.set
collmodify.help('set')

#@ global ? for set[USE:collfind.set]
\? CollectionModify.set

#@ global help for set[USE:collfind.set]
\help CollectionModify.set

#@ collfind.unset
collmodify.help('unset')

#@ global ? for unset[USE:collfind.unset]
\? CollectionModify.unset

#@ global help for unset[USE:collfind.unset]
\help CollectionModify.unset

shell.connect(__uripwd)
session.drop_schema('collectionmodify_help');
session.close()

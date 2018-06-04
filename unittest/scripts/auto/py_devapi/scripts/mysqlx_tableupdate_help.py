shell.connect(__uripwd + '/mysql')
tableupdate = db.user.update()
session.close()

#@ tableupdate
tableupdate.help()

#@ global ? for TableUpdate[USE:tableupdate]
\? TableUpdate

#@ global help for TableUpdate[USE:tableupdate]
\help TableUpdate

#@ tableupdate.bind
tableupdate.help('bind')

#@ global ? for bind[USE:tableupdate.bind]
\? TableUpdate.bind

#@ global help for bind[USE:tableupdate.bind]
\help TableUpdate.bind

#@ tableupdate.execute
tableupdate.help('execute')

#@ global ? for execute[USE:tableupdate.execute]
\? TableUpdate.execute

#@ global help for execute[USE:tableupdate.execute]
\help TableUpdate.execute

#@ tableupdate.help
tableupdate.help('help')

#@ global ? for help[USE:tableupdate.help]
\? TableUpdate.help

#@ global help for help[USE:tableupdate.help]
\help TableUpdate.help

#@ tableupdate.limit
tableupdate.help('limit')

#@ global ? for limit[USE:tableupdate.limit]
\? TableUpdate.limit

#@ global help for limit[USE:tableupdate.limit]
\help TableUpdate.limit

#@ tableupdate.order_by
tableupdate.help('order_by')

#@ global ? for order_by[USE:tableupdate.order_by]
\? TableUpdate.order_by

#@ global help for orderBy[USE:tableupdate.order_by]
\help TableUpdate.order_by

#@ tableupdate.set
tableupdate.help('set')

#@ global ? for set[USE:tableupdate.set]
\? TableUpdate.set

#@ global help for set[USE:tableupdate.set]
\help TableUpdate.set

#@ tableupdate.where
tableupdate.help('where')

#@ global ? for where[USE:tableupdate.where]
\? TableUpdate.where

#@ global help for where[USE:tableupdate.where]
\help TableUpdate.where

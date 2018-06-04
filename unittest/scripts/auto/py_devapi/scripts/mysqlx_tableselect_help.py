shell.connect(__uripwd + '/mysql')
tableselect = db.user.select()
session.close()

#@ tableselect
tableselect.help()

#@ global ? for TableSelect[USE:tableselect]
\? TableSelect

#@ global help for TableSelect[USE:tableselect]
\help TableSelect

#@ tableselect.bind
tableselect.help('bind')

#@ global ? for bind[USE:tableselect.bind]
\? TableSelect.bind

#@ global help for bind[USE:tableselect.bind]
\help TableSelect.bind

#@ tableselect.execute
tableselect.help('execute')

#@ global ? for execute[USE:tableselect.execute]
\? TableSelect.execute

#@ global help for execute[USE:tableselect.execute]
\help TableSelect.execute

#@ tableselect.group_by
tableselect.help('group_by')

#@ global ? for group_by[USE:tableselect.group_by]
\? TableSelect.group_by

#@ global help for group_by[USE:tableselect.group_by]
\help TableSelect.group_by

#@ tableselect.help
tableselect.help('help')

#@ global ? for help[USE:tableselect.help]
\? TableSelect.help

#@ global help for help[USE:tableselect.help]
\help TableSelect.help

#@ tableselect.limit
tableselect.help('limit')

#@ global ? for limit[USE:tableselect.limit]
\? TableSelect.limit

#@ global help for limit[USE:tableselect.limit]
\help TableSelect.limit

#@ tableselect.lock_exclusive
tableselect.help('lock_exclusive')

#@ global ? for lock_exclusive[USE:tableselect.lock_exclusive]
\? TableSelect.lock_exclusive

#@ global help for lock_exclusive[USE:tableselect.lock_exclusive]
\help TableSelect.lock_exclusive

#@ tableselect.lock_shared
tableselect.help('lock_shared')

#@ global ? for lock_shared[USE:tableselect.lock_shared]
\? TableSelect.lock_shared

#@ global help for lock_shared[USE:tableselect.lock_shared]
\help TableSelect.lock_shared

#@ tableselect.order_by
tableselect.help('order_by')

#@ global ? for order_by[USE:tableselect.order_by]
\? TableSelect.order_by

#@ global help for order_by[USE:tableselect.order_by]
\help TableSelect.order_by

#@ tableselect.where
tableselect.help('where')

#@ global ? for where[USE:tableselect.where]
\? TableSelect.where

#@ global help for where[USE:tableselect.where]
\help TableSelect.where

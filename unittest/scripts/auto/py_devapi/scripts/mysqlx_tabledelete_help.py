shell.connect(__uripwd + '/mysql')
tabledelete = db.user.delete()
session.close()

#@ tabledelete
tabledelete.help()

#@ global ? for TableDelete[USE:tabledelete]
\? TableDelete

#@ global help for TableDelete[USE:tabledelete]
\help TableDelete

#@ tabledelete.execute
tabledelete.help('execute')

#@ global ? for execute[USE:tabledelete.execute]
\? TableDelete.execute

#@ global help for execute[USE:tabledelete.execute]
\help TableDelete.execute

#@ tabledelete.help
tabledelete.help('help')

#@ global ? for help[USE:tabledelete.help]
\? TableDelete.help

#@ global help for help[USE:tabledelete.help]
\help TableDelete.help

#@ tabledelete.limit
tabledelete.help('limit')

#@ global ? for limit[USE:tabledelete.limit]
\? TableDelete.limit

#@ global help for limit[USE:tabledelete.limit]
\help TableDelete.limit

#@ tabledelete.order_by
tabledelete.help('order_by')

#@ global ? for order_by[USE:tabledelete.order_by]
\? TableDelete.order_by

#@ global help for order_by[USE:tabledelete.order_by]
\help TableDelete.order_by

#@ tabledelete.where
tabledelete.help('where')

#@ global ? for where[USE:tabledelete.where]
\? TableDelete.where

#@ global help for where[USE:tabledelete.where]
\help TableDelete.where

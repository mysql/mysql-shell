shell.connect(__uripwd + '/mysql')
tableinsert = db.user.insert()
session.close()

#@ tableinsert
tableinsert.help()

#@ global ? for TableInsert[USE:tableinsert]
\? TableInsert

#@ global help for TableInsert[USE:tableinsert]
\help TableInsert

#@ tableinsert.help
tableinsert.help('help')

#@ global ? for help[USE:tableinsert.help]
\? TableInsert.help

#@ global help for help[USE:tableinsert.help]
\help TableInsert.help

#@ tableinsert.insert
tableinsert.help('insert')

#@ global ? for insert[USE:tableinsert.insert]
\? TableInsert.insert

#@ global help for insert[USE:tableinsert.insert]
\help TableInsert.insert

#@ tableinsert.values
tableinsert.help('values')

#@ global ? for values[USE:tableinsert.values]
\? TableInsert.values

#@ global help for values[USE:tableinsert.values]
\help TableInsert.values
